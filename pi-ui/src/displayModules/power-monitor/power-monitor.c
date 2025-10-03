#include "power-monitor.h"
#include "../shared/module_interface.h"
#include "../../esp_compat.h"
#include "views/power_grid_view.h"
#include "views/starter_voltage_view.h"
// Use the unified detail screen template header (matches implementation under shared/detail_screen)
#include "../../screens/detail_screen/detail_screen.h"
// Include the shared current view manager
#include "../shared/current_view/current_view_manager.h"
#include "../shared/alerts_modal/alerts_modal.h"
#include "../shared/gauges/bar_graph_gauge.h"
#include "../../screens/home_screen/home_screen.h"
#include "../../state/device_state.h"
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include "../../data/config.h"
#include "../../data/mock_data/mock_data.h"
#include "../../data/lerp_data/lerp_data.h"
#include <stdlib.h>
#include <string.h>

// Forward declarations
static void power_monitor_cycle_view(void);
static void power_monitor_close_modal(void);
static void power_monitor_view_creator(lv_obj_t* container, void* user_data);
void power_monitor_create_current_view_in_container(lv_obj_t* container);
static void power_monitor_detail_touch_callback(void);
void power_monitor_create_current_view_content(lv_obj_t* container);
static void power_monitor_home_current_view_touch_cb(lv_event_t * e);
static void power_monitor_detail_current_view_touch_cb(lv_event_t * e);
static void power_monitor_create_detail_gauges(lv_obj_t* container);
static void power_monitor_create_view_containers(lv_obj_t* parent_container);
// Sensor label functions moved to detail_screen.c
static void power_monitor_update_detail_gauges(void);
static power_monitor_data_t* power_monitor_get_data_internal(void);
void power_monitor_render_current_view(lv_obj_t* container);
static void power_monitor_update_all_containers(void);
static void power_monitor_current_view_touch_cb(lv_event_t * e);
void power_monitor_cleanup_internal(void);
static void power_monitor_destroy_current_view(void);
static int power_monitor_get_view_index(void);
static void power_monitor_set_view_index(int index);

static const char *TAG = "power_monitor";

// Flag to indicate detail view needs refresh after view cycling
static bool s_detail_view_needs_refresh = false;

// View configuration
static const int s_total_views = 2;

// Crash handler for debugging
void crash_handler(int sig) {
	void *array[20];
	size_t size;

	ESP_LOGE(TAG, "=== CRASH DETECTED: Signal %d ===", sig);

	// Get void*'s for all entries on the stack
	size = backtrace(array, 20);

	// Print out all the frames to stderr
	ESP_LOGE(TAG, "Stack trace:");
	backtrace_symbols_fd(array, size, STDERR_FILENO);

	// Also log to ESP logger
	char **strings = backtrace_symbols(array, size);
	if (strings != NULL) {
		for (size_t i = 0; i < size; i++) {
			ESP_LOGE(TAG, "  %s", strings[i]);
		}
		free(strings);
	}

	ESP_LOGE(TAG, "=== END CRASH DUMP ===");
	exit(1);
}

// Module-specific data
static power_monitor_data_t power_data = {0};

// Removed unused home_overlay
static detail_screen_t* detail_screen = NULL;

// Global view initialization flags
static bool s_rendering_in_progress = false;

// Removed navigation callbacks - using direct function calls
static bool s_creation_in_progress = false;
static bool s_reset_in_progress = false;

// Global current view containers for this module
static lv_obj_t* g_home_container = NULL;
static lv_obj_t* g_detail_container = NULL;

// View containers - create once, reuse by showing/hiding
static lv_obj_t* g_power_grid_view_container = NULL;
static lv_obj_t* g_starter_voltage_view_container = NULL;

// Guard to prevent recursive calls

// Available view types in order (first is default)
static const power_monitor_view_type_t available_views[] = {
	POWER_MONITOR_VIEW_BAR_GRAPH,    // Power grid view (value 3)
	POWER_MONITOR_VIEW_NUMERICAL     // Starter Voltage view (value 4)
};

static alerts_modal_t* current_modal = NULL;

// Helper function to get current view type from global state
static power_monitor_view_type_t get_current_view_type(void)
{
	int view_index = power_monitor_get_view_index();

	if (view_index < 0 || view_index >= s_total_views) {
		ESP_LOGE(TAG, "Invalid view index: %d (total: %d)", view_index, s_total_views);
		return POWER_MONITOR_VIEW_BAR_GRAPH; // Default fallback
	}

	return available_views[view_index];
}

// Navigation callback implementations
static void power_monitor_navigation_cycle_to_next_view(void);
static void power_monitor_navigation_hide_detail_screen(void);
static void power_monitor_navigation_request_home_screen(void);

static void power_monitor_navigation_cycle_to_next_view(void)
{
	ESP_LOGI(TAG, "=== SIMPLE VIEW CYCLING ===");

	// Get current index from device state
	int current_index = power_monitor_get_view_index();
	ESP_LOGI(TAG, "Before cycle: index=%d, total_views=%d", current_index, s_total_views);

	// Simple wrap-around logic
	int next_index = (current_index + 1) % s_total_views;

	// Update device state directly
	power_monitor_set_view_index(next_index);
	ESP_LOGI(TAG, "View cycle complete - updated from index %d to %d", current_index, next_index);
}

// Callback to request home screen after LVGL cleanup
static void power_monitor_navigation_hide_detail_screen(void)
{
	ESP_LOGI(TAG, "About to DESTROY detail screen completely");
	ESP_LOGI(TAG, "Detail screen pointer: %p", detail_screen);
	if (detail_screen) {
		ESP_LOGI(TAG, "Calling detail_screen_destroy");
		detail_screen_destroy(detail_screen);
		detail_screen = NULL; // Clear the pointer after destruction
		ESP_LOGI(TAG, "detail_screen_destroy completed");
	} else {
		ESP_LOGW(TAG, "Detail screen is NULL");
	}
	ESP_LOGI(TAG, "Detail screen completely destroyed");

	// Request transition back to home screen
	ESP_LOGI(TAG, "Requesting transition back to home screen");
	power_monitor_navigation_request_home_screen();
}

static void power_monitor_navigation_request_home_screen(void)
{
	ESP_LOGI(TAG, "About to request home screen transition");
	extern void screen_navigation_request_home_screen(void);
	ESP_LOGI(TAG, "Calling screen_navigation_request_home_screen");
	screen_navigation_request_home_screen();
	ESP_LOGI(TAG, "Home screen transition requested");
	ESP_LOGI(TAG, "power_monitor_navigation_request_home_screen completed");
}


// Simple container management
static lv_obj_t* power_monitor_container = NULL;

// Sensor data labels moved to detail_screen.c - power monitor only provides data

// Initialize power-monitor's own container (simplified)
static void power_monitor_init_widget(void)
{
	if (power_monitor_container) {
		ESP_LOGI(TAG, "Container already initialized");
		return;
	}

	// Create power-monitor's own container on the main screen
	power_monitor_container = lv_obj_create(lv_scr_act());
	lv_obj_set_size(power_monitor_container, 320, 240);
	lv_obj_align(power_monitor_container, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_bg_opa(power_monitor_container, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(power_monitor_container, 0, 0);
	lv_obj_clear_flag(power_monitor_container, LV_OBJ_FLAG_SCROLLABLE);

	ESP_LOGI(TAG, "Power monitor container created successfully");
}


// Removed unused power_monitor_show_for_detail_screen function


// View creator function for the shared current view template
static void power_monitor_view_creator(lv_obj_t* container, void* user_data)
{
	ESP_LOGI(TAG, "=== POWER MONITOR VIEW CREATOR CALLED ===");
	ESP_LOGI(TAG, "Container: %p, User data: %p", container, user_data);

	if (!container) {
		ESP_LOGE(TAG, "Container is NULL");
		return;
	}

	// Content will be cleared by widget destruction

	// Render the appropriate view based on current view type
	power_monitor_view_type_t view_type = get_current_view_type();
	switch (view_type) {
		case POWER_MONITOR_VIEW_BAR_GRAPH:
			power_monitor_power_grid_view_render(container);
			power_monitor_power_grid_view_update_data();
			break;
		case POWER_MONITOR_VIEW_NUMERICAL:
			power_monitor_starter_voltage_view_render(container);
			power_monitor_starter_voltage_view_update_data();
			break;
		default:
			// Default to power grid view
			power_monitor_power_grid_view_render(container);
			power_monitor_power_grid_view_update_data();
			break;
	}

	ESP_LOGI(TAG, "Power monitor view content created successfully");
}

// Create current view using the shared template system
void power_monitor_create_current_view_content(lv_obj_t* container)
{
	ESP_LOGI(TAG, "=== CREATING CURRENT VIEW CONTENT ===");
	ESP_LOGI(TAG, "Container: %p", container);

	if (!container) {
		ESP_LOGE(TAG, "Container is NULL");
		return;
	}

	// Safety check: validate container
	if (!lv_obj_is_valid(container)) {
		ESP_LOGE(TAG, "Container is not valid");
		return;
	}

	ESP_LOGI(TAG, "Container is valid, rendering current view");

	// Clear container first
	lv_obj_clean(container);
	ESP_LOGI(TAG, "Container cleared");

	// Re-apply container styling after clean (clean removes styling)
	// Check if this is the current_view_container from detail screen
	extern detail_screen_t* detail_screen;
	if (detail_screen && container == detail_screen->current_view_container) {
		lv_obj_set_style_bg_color(container, lv_color_hex(0x000000), 0);
		lv_obj_set_style_border_width(container, 1, 0);
		lv_obj_set_style_border_color(container, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_radius(container, 4, 0);
		lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
		ESP_LOGI(TAG, "Re-applied current_view_container styling after clean");
	}

	// Use the same approach as initial creation: create view containers first
	// This ensures consistent behavior between initial creation and subsequent cycles
	power_monitor_create_view_containers(container);

	// Get current view type and render appropriate view
	// Use device state for persistent view index
	int view_index = power_monitor_get_view_index();
	power_monitor_view_type_t view_type;

	// Map index to view type: 0=BAR_GRAPH, 1=NUMERICAL
	if (view_index == 0) {
		view_type = POWER_MONITOR_VIEW_BAR_GRAPH;
	} else if (view_index == 1) {
		view_type = POWER_MONITOR_VIEW_NUMERICAL;
	} else {
		ESP_LOGW(TAG, "Unknown view index: %d, defaulting to BAR_GRAPH", view_index);
		view_type = POWER_MONITOR_VIEW_BAR_GRAPH;
	}

		// Log the current view type and index
		ESP_LOGI(TAG, "View cycling: index=%d, type=%d", view_index, view_type);

	// Render on the appropriate view container (same as initial creation)
	if (view_type == POWER_MONITOR_VIEW_BAR_GRAPH) {
		power_monitor_power_grid_view_render(g_power_grid_view_container);
	} else if (view_type == POWER_MONITOR_VIEW_NUMERICAL) {
		power_monitor_starter_voltage_view_render(g_starter_voltage_view_container);
	} else {
		ESP_LOGW(TAG, "Unknown view type: %d", view_type);
	}

	// Mark view cycling as complete if this was called during cycling
	if (current_view_manager_is_cycling_in_progress()) {
		current_view_manager_set_cycling_in_progress(false);
		ESP_LOGI(TAG, "View cycling marked as complete in create_current_view_content");
	}

	ESP_LOGI(TAG, "Current view content creation complete");
}





// Simple view cycling - just calls the main cycling function
static void power_monitor_cycle_view(void)
{
	ESP_LOGI(TAG, "=== CYCLING CURRENT VIEW ===");
	power_monitor_cycle_current_view();
}


// Removed unused power_monitor_get_view_info function

// Static gauge instances for detail screen (like historic detail.c)
static bar_graph_gauge_t detail_starter_voltage_gauge = {0};
static bar_graph_gauge_t detail_starter_current_gauge = {0};
static bar_graph_gauge_t detail_house_voltage_gauge = {0};
static bar_graph_gauge_t detail_house_current_gauge = {0};
static bar_graph_gauge_t detail_solar_voltage_gauge = {0};
static bar_graph_gauge_t detail_solar_current_gauge = {0};

// Create 6 bar graph gauges in the gauges container (matching original detail.c)
static void power_monitor_create_detail_gauges(lv_obj_t* container)
{
	ESP_LOGI(TAG, "*** CREATING 6 BAR GRAPH GAUGES IN DETAIL SCREEN ***");
	ESP_LOGI(TAG, "Container: %p, size: %dx%d", container,
		lv_obj_get_width(container), lv_obj_get_height(container));

	if (!container) {
		ESP_LOGE(TAG, "Gauges container is NULL");
		return;
	}

	// Always create fresh gauges
	ESP_LOGI(TAG, "Creating fresh detail gauges");

	// Force layout update to get correct dimensions
	lv_obj_update_layout(container);

	// Force layout update

	// Get container dimensions after layout update
	lv_coord_t container_width = lv_obj_get_width(container);
	lv_coord_t container_height = lv_obj_get_height(container);

	ESP_LOGI(TAG, "Container dimensions after layout update: %dx%d", container_width, container_height);

	// Check for valid dimensions
	if (container_width <= 0 || container_height <= 0) {
		ESP_LOGE(TAG, "Invalid container dimensions: %dx%d", container_width, container_height);
		return;
	}

	// Calculate gauge dimensions for vertical stack with 6px padding between gauges (2px + 4px from gauge height reduction)
	int gauge_padding = 12; // 6px padding between gauges (2px original + 4px from gauge height reduction)
	int total_padding = gauge_padding * 5; // 5 gaps between 6 gauges
	lv_coord_t gauge_width = container_width;  // Full width, no padding
	lv_coord_t gauge_height = (container_height - 2 - total_padding) / 6; // 6 rows with padding and 2px offset for the title bleed

	ESP_LOGI(TAG, "Gauge dimensions: %dx%d each with %dpx padding", gauge_width, gauge_height, gauge_padding);

	// Configure container for flexbox layout (vertical stack with fixed heights)
	lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_row(container, gauge_padding, 0); // Vertical gap between gauges

	// Row 1: Starter Battery Voltage
	bar_graph_gauge_init(&detail_starter_voltage_gauge, container, 0, 0, gauge_width, gauge_height);
	// Gauge uses fixed height calculated above (1/6 of available space)
	float starter_baseline = device_state_get_starter_baseline_voltage_v();
	float starter_min = device_state_get_starter_min_voltage_v();
	float starter_max = device_state_get_starter_max_voltage_v();
	ESP_LOGI(TAG, "Detail starter gauge config: %.1f-%.1fV (baseline=%.1f)", starter_min, starter_max, starter_baseline);
	bar_graph_gauge_configure_advanced(
		&detail_starter_voltage_gauge,
		BAR_GRAPH_MODE_BIPOLAR,
		starter_baseline,
		starter_min,
		starter_max,
		"STARTER BATTERY", "V", "V", 0x00FF00,
		true, true, true // Show title, Show Y-axis, Show border
	);
	detail_starter_voltage_gauge.bar_width = 3; // Match current view bar width
	detail_starter_voltage_gauge.bar_gap = 1;   // Match current view bar gap
	bar_graph_gauge_set_update_interval(&detail_starter_voltage_gauge, 33); // 30 FPS for high performance

	// Row 2: Starter Battery Current
	bar_graph_gauge_init(&detail_starter_current_gauge, container, 0, 0, gauge_width, gauge_height);
	bar_graph_gauge_configure_advanced(&detail_starter_current_gauge,
		BAR_GRAPH_MODE_BIPOLAR, 0.0f, -50.0f, 50.0f,
		"STARTER BATTERY", "A", "A", 0x0080FF, true, true, true);
	detail_starter_current_gauge.bar_width = 3; // Match current view bar width
	detail_starter_current_gauge.bar_gap = 1;   // Match current view bar gap
	bar_graph_gauge_set_update_interval(&detail_starter_current_gauge, 50); // 20 FPS for high performance

	// Row 3: House Battery Voltage
	bar_graph_gauge_init(&detail_house_voltage_gauge, container, 0, 0, gauge_width, gauge_height);
	float house_baseline = device_state_get_house_baseline_voltage_v();
	float house_min = device_state_get_house_min_voltage_v();
	float house_max = device_state_get_house_max_voltage_v();
	ESP_LOGI(TAG, "Detail house gauge config: %.1f-%.1fV (baseline=%.1f)", house_min, house_max, house_baseline);
	bar_graph_gauge_configure_advanced(&detail_house_voltage_gauge,
		BAR_GRAPH_MODE_BIPOLAR, house_baseline, house_min, house_max,
		"HOUSE BATTERY", "V", "V", 0x00FF00, true, true, true);
	detail_house_voltage_gauge.bar_width = 3; // Match current view bar width
	detail_house_voltage_gauge.bar_gap = 1;   // Match current view bar gap
	bar_graph_gauge_set_update_interval(&detail_house_voltage_gauge, 33); // 30 FPS for high performance

	// Row 4: House Battery Current
	bar_graph_gauge_init(&detail_house_current_gauge, container, 0, 0, gauge_width, gauge_height);
	bar_graph_gauge_configure_advanced(&detail_house_current_gauge,
		BAR_GRAPH_MODE_BIPOLAR, 0.0f, -50.0f, 50.0f,
		"HOUSE BATTERY", "A", "A", 0x0080FF, true, true, true);
	detail_house_current_gauge.bar_width = 3; // Match current view bar width
	detail_house_current_gauge.bar_gap = 1;   // Match current view bar gap
	bar_graph_gauge_set_update_interval(&detail_house_current_gauge, 50); // 20 FPS for high performance

	// Row 5: Solar Input Voltage
	bar_graph_gauge_init(&detail_solar_voltage_gauge, container, 0, 0, gauge_width, gauge_height);
	bar_graph_gauge_configure_advanced(&detail_solar_voltage_gauge,
		BAR_GRAPH_MODE_POSITIVE_ONLY, 0.0f, 0.0f, 25.0f,
		"SOLAR VOLTS", "V", "V", 0xFF8000, true, true, true);
	detail_solar_voltage_gauge.bar_width = 3; // Match current view bar width
	detail_solar_voltage_gauge.bar_gap = 1;   // Match current view bar gap
	bar_graph_gauge_set_update_interval(&detail_solar_voltage_gauge, 33); // 30 FPS for high performance

	// Row 6: Solar Input Current
	bar_graph_gauge_init(&detail_solar_current_gauge, container, 0, 0, gauge_width, gauge_height);
	bar_graph_gauge_configure_advanced(&detail_solar_current_gauge,
		BAR_GRAPH_MODE_POSITIVE_ONLY, 0.0f, 0.0f, 10.0f,
		"SOLAR CURRENT", "A", "A", 0xFF8000, true, true, true);
	detail_solar_current_gauge.bar_width = 3; // Match current view bar width
	detail_solar_current_gauge.bar_gap = 1;   // Match current view bar gap
	bar_graph_gauge_set_update_interval(&detail_solar_current_gauge, 50); // 20 FPS for high performance

	// Update labels and ticks for all gauges
	bar_graph_gauge_update_labels_and_ticks(&detail_starter_voltage_gauge);
	bar_graph_gauge_update_labels_and_ticks(&detail_starter_current_gauge);
	bar_graph_gauge_update_labels_and_ticks(&detail_house_voltage_gauge);
	bar_graph_gauge_update_labels_and_ticks(&detail_house_current_gauge);
	bar_graph_gauge_update_labels_and_ticks(&detail_solar_voltage_gauge);
	bar_graph_gauge_update_labels_and_ticks(&detail_solar_current_gauge);

	ESP_LOGI(TAG, "3 gauges created successfully (LVGL memory increased to 512KB)");
}

// Sensor label cleanup moved to detail_screen.c

// Sensor data labels creation moved to detail_screen.c

/*
// DEAD CODE REMOVED - Sensor data update moved to detail_screen.c - power monitor only provides data
	int y_pos = 35; // More space from title
	int line_height = 30; // Increased line height for bigger fonts
	int group_spacing = 20; // Extra spacing between groups
	int left_column_x = 5;
	int right_column_x = -5;
	lv_color_t labelColor = lv_color_hex(0x00bbe6);
	lv_color_t valueColor = lv_color_hex(0x39ab00);
	lv_color_t groupColor = lv_color_hex(0xFFFFFF);

	// Label configuration map - all labels defined in one place
	// Index mapping: 0,1,2,3,4 = Starter Battery group, 5,6,7,8,9 = House Battery group, 10,11,12,13,14 = Solar Input group
	// Each group: [Group Header, Volts Label, Volts Value, Amperes Label, Amperes Value]
	// Static labels (is_static=true): Group headers and "Volts:"/"Amperes:" labels
	// Dynamic labels (is_static=false): Actual sensor values (indices 2,4,7,9,12,14)
	const struct {
		const char* text;
		const lv_font_t* font;
		lv_color_t color;
		int x_offset;
		lv_align_t align;
		lv_text_align_t text_align;
		bool is_static;
	} label_configs[15] = {
		// Starter Battery Group
		{"Starter Battery", &lv_font_montserrat_16, groupColor, left_column_x, LV_ALIGN_TOP_LEFT, LV_TEXT_ALIGN_LEFT, true},
		{"Volts:", &lv_font_montserrat_14, labelColor, left_column_x, LV_ALIGN_TOP_LEFT, LV_TEXT_ALIGN_LEFT, true},
		{"0.0", &lv_font_montserrat_24, valueColor, right_column_x - 5, LV_ALIGN_TOP_RIGHT, LV_TEXT_ALIGN_RIGHT, false},
		{"Amperes:", &lv_font_montserrat_14, labelColor, left_column_x, LV_ALIGN_TOP_LEFT, LV_TEXT_ALIGN_LEFT, true},
		{"0.0", &lv_font_montserrat_24, valueColor, right_column_x - 5, LV_ALIGN_TOP_RIGHT, LV_TEXT_ALIGN_RIGHT, false},

		// House Battery Group
		{"House Battery", &lv_font_montserrat_16, groupColor, left_column_x, LV_ALIGN_TOP_LEFT, LV_TEXT_ALIGN_LEFT, true},
		{"Volts:", &lv_font_montserrat_14, labelColor, left_column_x, LV_ALIGN_TOP_LEFT, LV_TEXT_ALIGN_LEFT, true},
		{"0.0", &lv_font_montserrat_24, valueColor, right_column_x - 5, LV_ALIGN_TOP_RIGHT, LV_TEXT_ALIGN_RIGHT, false},
		{"Amperes:", &lv_font_montserrat_14, labelColor, left_column_x, LV_ALIGN_TOP_LEFT, LV_TEXT_ALIGN_LEFT, true},
		{"0.0", &lv_font_montserrat_24, valueColor, right_column_x - 5, LV_ALIGN_TOP_RIGHT, LV_TEXT_ALIGN_RIGHT, false},

		// Solar Input Group
		{"Solar Input", &lv_font_montserrat_16, groupColor, left_column_x, LV_ALIGN_TOP_LEFT, LV_TEXT_ALIGN_LEFT, true},
		{"Volts:", &lv_font_montserrat_14, labelColor, left_column_x, LV_ALIGN_TOP_LEFT, LV_TEXT_ALIGN_LEFT, true},
		{"0.0", &lv_font_montserrat_24, valueColor, right_column_x - 5, LV_ALIGN_TOP_RIGHT, LV_TEXT_ALIGN_RIGHT, false},
		{"Amperes:", &lv_font_montserrat_14, labelColor, left_column_x, LV_ALIGN_TOP_LEFT, LV_TEXT_ALIGN_LEFT, true},
		{"0.0", &lv_font_montserrat_24, valueColor, right_column_x - 5, LV_ALIGN_TOP_RIGHT, LV_TEXT_ALIGN_RIGHT, false}
	};

	// Create all labels with proper vertical positioning
	for (int i = 0; i < 15; i++) {
		// sensor_labels[i] = lv_label_create(container); // REMOVED
		lv_obj_set_style_text_font(sensor_labels[i], label_configs[i].font, 0);
		lv_obj_set_style_text_color(sensor_labels[i], label_configs[i].color, 0);
		lv_obj_set_style_text_align(sensor_labels[i], label_configs[i].text_align, 0);

		// Calculate correct y position for each label
		int current_y = y_pos;
		if (i % 5 == 0) {
			// Group header - first line of group
			current_y = y_pos;
		} else if (i % 5 == 1) {
			// "Volts:" label - second line of group
			current_y = y_pos + line_height;
		} else if (i % 5 == 2) {
			// Volts value - same line as "Volts:" label
			current_y = y_pos + line_height;
		} else if (i % 5 == 3) {
			// "Amperes:" label - third line of group
			current_y = y_pos + 2 * line_height;
		} else if (i % 5 == 4) {
			// Amperes value - same line as "Amperes:" label
			current_y = y_pos + 2 * line_height;
		}

		lv_obj_align(sensor_labels[i], label_configs[i].align, label_configs[i].x_offset, current_y);

		// Set text for static labels
		if (label_configs[i].is_static) {
			lv_label_set_text(sensor_labels[i], label_configs[i].text);
			ESP_LOGD(TAG, "Created static label %d: %s at y=%d", i, label_configs[i].text, current_y);
		} else {
			// Dynamic labels start with "0.0" placeholder
			lv_label_set_text(sensor_labels[i], "0.0");
			ESP_LOGD(TAG, "Created dynamic label %d: %s at y=%d", i, "0.0", current_y);
		}

		// Update y position for next group
		if (i % 5 == 4) {
			// End of group - move to next group position
			y_pos += 3 * line_height + group_spacing;
		}
	}

	ESP_LOGI(TAG, "Sensor data labels created successfully");

	// Debug: Verify sensor_labels array is populated
	for (int i = 0; i < 15; i++) {
		if (sensor_labels[i]) {
			ESP_LOGD(TAG, "sensor_labels[%d] = %p", i, sensor_labels[i]);
	} else {
			ESP_LOGE(TAG, "sensor_labels[%d] is NULL!", i);
		}
	}

	// Mark as created
	// sensor_labels_created = true; // REMOVED

	// Immediately update with current data
	power_monitor_update_sensor_data();
}
*/

// Sensor data and alert flashing functions removed - functionality moved to detail_screen.c

// Apply alert flashing for current view values
static void power_monitor_apply_current_view_alert_flashing(void)
{
	// Only apply current view alert flashing if we have valid containers
	if (!g_home_container && !detail_screen) {
		return; // No valid containers, skip current view alert flashing
	}

	// Get current data
	power_monitor_data_t *data = power_monitor_get_data();
	if (!data) {
		return;
	}

	// Get alert thresholds
	int starter_lo = device_state_get_starter_alert_low_voltage_v();
	int starter_hi = device_state_get_starter_alert_high_voltage_v();
	int house_lo = device_state_get_house_alert_low_voltage_v();
	int house_hi = device_state_get_house_alert_high_voltage_v();
	int solar_lo = device_state_get_solar_alert_low_voltage_v();
	int solar_hi = device_state_get_solar_alert_high_voltage_v();

	// Blink timing - asymmetric: 1 second on, 0.5 seconds off (1.5 second total cycle)
	int tick_ms = (int)(xTaskGetTickCount() * 1);
	bool blink_on = (tick_ms % 1500) < 1000;

	// Apply alert flashing only to the currently active view
	power_monitor_view_type_t current_view = get_current_view_type();
	if (current_view == POWER_MONITOR_VIEW_BAR_GRAPH) {
		power_monitor_power_grid_view_apply_alert_flashing(data, starter_lo, starter_hi, house_lo, house_hi, solar_lo, solar_hi, blink_on);
	} else if (current_view == POWER_MONITOR_VIEW_NUMERICAL) {
		power_monitor_starter_voltage_view_apply_alert_flashing(data, starter_lo, starter_hi, house_lo, house_hi, solar_lo, solar_hi, blink_on);
	} else {
		ESP_LOGW(TAG, "Unknown view type: %d, skipping alert flashing", current_view);
	}
}

// Update detail screen gauges with LERP display data (optimized for performance)
static void power_monitor_update_detail_gauges(void)
{
	// Only update gauges if detail screen exists and gauges are properly initialized
	if (!detail_screen) {
		return; // No detail screen, skip gauge updates
	}

	// Get LERP data for smooth display values
	lerp_power_monitor_data_t lerp_data;
	lerp_data_get_current(&lerp_data);

	// Update all 6 gauges with LERP display data (rate-limited by gauge update intervals)
	// Note: Canvas updates are only applied when detail screen is visible, but data collection continues

	if (detail_starter_voltage_gauge.initialized) {
		bar_graph_gauge_add_data_point(&detail_starter_voltage_gauge, lerp_value_get_display(&lerp_data.starter_voltage));
		// Canvas update is called automatically by add_data_point if rate limit allows
	}

	if (detail_starter_current_gauge.initialized) {
		bar_graph_gauge_add_data_point(&detail_starter_current_gauge, lerp_value_get_display(&lerp_data.starter_current));
	}

	if (detail_house_voltage_gauge.initialized) {
		// Pass actual voltage value - gauge handles bipolar display internally
		bar_graph_gauge_add_data_point(&detail_house_voltage_gauge, lerp_value_get_display(&lerp_data.house_voltage));
	}

	if (detail_house_current_gauge.initialized) {
		bar_graph_gauge_add_data_point(&detail_house_current_gauge, lerp_value_get_display(&lerp_data.house_current));
	}

	if (detail_solar_voltage_gauge.initialized) {
		bar_graph_gauge_add_data_point(&detail_solar_voltage_gauge, lerp_value_get_display(&lerp_data.solar_voltage));
	}

	if (detail_solar_current_gauge.initialized) {
		bar_graph_gauge_add_data_point(&detail_solar_current_gauge, lerp_value_get_display(&lerp_data.solar_current));
	}
}


// Interface function to get current values from detail screen gauges (for power grid view)
bool power_monitor_get_detail_gauge_values(float* starter_voltage, float* house_voltage, float* solar_voltage)
{
	if (!starter_voltage || !house_voltage || !solar_voltage) {
		return false;
	}

	// Safety check - only access gauge data if detail screen exists
	if (!detail_screen) {
		*starter_voltage = 0.0f;
		*house_voltage = 0.0f;
		*solar_voltage = 0.0f;
		return true;
	}

	// Get current values from detail screen gauges if they're initialized and have data
	if (detail_starter_voltage_gauge.initialized && detail_starter_voltage_gauge.head >= 0) {
		*starter_voltage = detail_starter_voltage_gauge.data_points[detail_starter_voltage_gauge.head];
	} else {
		*starter_voltage = 0.0f;
	}

	if (detail_house_voltage_gauge.initialized && detail_house_voltage_gauge.head >= 0) {
		*house_voltage = detail_house_voltage_gauge.data_points[detail_house_voltage_gauge.head];
	} else {
		*house_voltage = 0.0f;
	}

	if (detail_solar_voltage_gauge.initialized && detail_solar_voltage_gauge.head >= 0) {
		*solar_voltage = detail_solar_voltage_gauge.data_points[detail_solar_voltage_gauge.head];
	} else {
		*solar_voltage = 0.0f;
	}

	return true;
}

// Get internal data
static power_monitor_data_t* power_monitor_get_data_internal(void)
{
	// Return pointer to our internal data structure
	return &power_data;
}

// Public function for getting data (required by mock_data system)
power_monitor_data_t* power_monitor_get_data(void)
{
	return &power_data;
}


// Public function for updating (required by screen manager)




// Data-only update function (no UI structure changes)
void power_monitor_update_data_only(void)
{
	// Update internal data from mock data system
	if (data_config_get_source() == DATA_SOURCE_MOCK) {
		mock_data_write_to_state_objects();
	}

	// Update LERP data system with current power data
	lerp_data_set_targets(&power_data);
	lerp_data_update();

	// Update detail screen gauges data (if detail screen exists)
	power_monitor_update_detail_gauges();

	// Update view data (this updates the data in the views, not the UI structure)
	power_monitor_power_grid_view_update_data();
	power_monitor_starter_voltage_view_update_data();

	// Update sensor labels in detail screen (if detail screen exists)
	extern detail_screen_t* detail_screen;
	if (detail_screen) {
		power_monitor_data_t* data = power_monitor_get_data();
		if (data) {
			detail_screen_update_sensor_labels(detail_screen, data);
		}
	}

	// Current view alert flashing (still handled by power monitor)
	power_monitor_apply_current_view_alert_flashing();
}


// Wrapper functions for module interface compatibility
static void power_monitor_render_detail_wrapper(lv_obj_t* container) {
	// The detail screen is managed separately, this is just a placeholder
	// The actual detail screen rendering is handled by the detail screen template
	(void)container; // Suppress unused parameter warning
}


// Public function for initializing with default view (required by main.c)
void power_monitor_init_with_default_view(power_monitor_view_type_t default_view)
{
	power_monitor_init();
	power_monitor_set_current_view_type((power_monitor_view_type_t)default_view);
}

// Module interface implementation
void power_monitor_init(void)
{
	ESP_LOGI(TAG, "Power Monitor Module Initialized (V2)");

	// Set up crash handlers for debugging
	signal(SIGSEGV, crash_handler);
	signal(SIGABRT, crash_handler);
	signal(SIGFPE, crash_handler);
	signal(SIGILL, crash_handler);
	signal(SIGBUS, crash_handler);
	ESP_LOGI(TAG, "Crash handlers installed");

	// Initialize module-specific screen view state
	module_screen_view_initialize("power-monitor", SCREEN_HOME, sizeof(available_views) / sizeof(available_views[0]));

	// Initialize shared current view manager (for backward compatibility)
	current_view_manager_init(sizeof(available_views) / sizeof(available_views[0]));

	// Navigation callbacks removed - using direct function calls

	// Initialize data
	memset(&power_data, 0, sizeof(power_data));

	// Initialize LERP data system
	lerp_data_init();

	// Initialize widget
	power_monitor_init_widget();

}

void power_monitor_show_in_container(lv_obj_t *container)
{
	ESP_LOGI(TAG, "=== SHOW IN CONTAINER (V2) ===");
	ESP_LOGI(TAG, "Container: %p", container);

	if (!container) {
		ESP_LOGE(TAG, "Container is NULL");
		return;
	}

	// Create current view content directly in the container
	power_monitor_create_current_view_content(container);
}

void power_monitor_show_in_container_home(lv_obj_t *container)
{
	static int call_count = 0;
	call_count++;
	ESP_LOGI(TAG, "=== SHOW IN CONTAINER HOME CALLED #%d ===", call_count);

	// Store the home container for this module
	g_home_container = container;


	// Render the current view directly into the container
	ESP_LOGI(TAG, "About to call power_monitor_render_current_view");
	power_monitor_render_current_view(container);
	ESP_LOGI(TAG, "power_monitor_render_current_view completed");
	ESP_LOGI(TAG, "=== SHOW IN CONTAINER HOME COMPLETE #%d ===", call_count);
}

void power_monitor_show_in_container_detail(lv_obj_t *container)
{
	// Store the detail container for this module
	g_detail_container = container;


	// Render the current view directly into the container
	power_monitor_render_current_view(container);
}

void power_monitor_cycle_current_view(void)
{
	static int call_count = 0;
	call_count++;
	ESP_LOGI(TAG, "=== CYCLING CURRENT VIEW CALLED #%d ===", call_count);

	// Simple guard against rapid cycling
	if (s_detail_view_needs_refresh) {
		ESP_LOGW(TAG, "View cycling already in progress, skipping");
		return;
	}

	// Cycle to next view using simple logic
	power_monitor_navigation_cycle_to_next_view();

	// Mark that we need to re-render the detail view on next update
	// This avoids immediate re-rendering which can cause crashes
	extern screen_type_t screen_navigation_get_current_screen(void);
	if (screen_navigation_get_current_screen() == SCREEN_DETAIL_VIEW) {
		ESP_LOGI(TAG, "Marking detail view for re-render on next update cycle");
		s_detail_view_needs_refresh = true;
	}
}

// Test function removed - view cycling is working properly

void power_monitor_destroy(void)
{
	ESP_LOGI(TAG, "=== DESTROY ===");
	// No cleanup needed - everything will be created fresh
}

// Removed duplicate power_monitor_update_internal function



void power_monitor_cleanup_internal(void)
{
	ESP_LOGI(TAG, "=== CLEANUP INTERNAL (V2) ===");

	// Clean up containers
	if (power_monitor_container) {
		lv_obj_del(power_monitor_container);
		power_monitor_container = NULL;
	}

	// Clean up detail screen
	if (detail_screen) {
		detail_screen_destroy(detail_screen);
		detail_screen = NULL;
	}

	// Clean up current view manager
	current_view_manager_cleanup();

	// Sensor data labels cleanup moved to detail_screen.c

	// Reset static gauge variables to prevent memory conflicts
	extern void power_monitor_reset_static_gauges(void);
	extern void power_monitor_reset_starter_voltage_static_gauge(void);
	power_monitor_reset_static_gauges();
	power_monitor_reset_starter_voltage_static_gauge();

	// Reset detail gauge variables to prevent crashes during updates
	memset(&detail_starter_voltage_gauge, 0, sizeof(bar_graph_gauge_t));
	memset(&detail_starter_current_gauge, 0, sizeof(bar_graph_gauge_t));
	memset(&detail_house_voltage_gauge, 0, sizeof(bar_graph_gauge_t));
	memset(&detail_house_current_gauge, 0, sizeof(bar_graph_gauge_t));
	memset(&detail_solar_voltage_gauge, 0, sizeof(bar_graph_gauge_t));
	memset(&detail_solar_current_gauge, 0, sizeof(bar_graph_gauge_t));
	ESP_LOGI(TAG, "Detail gauge variables reset");
}

void power_monitor_cleanup(void)
{
	ESP_LOGI(TAG, "Power monitor cleanup called");
	power_monitor_cleanup_internal();
}

// Button configurations for power monitor
static detail_button_config_t power_monitor_buttons[] = {
	{"ALERTS", power_monitor_handle_alerts_button},
	{"TIMELINE", power_monitor_handle_timeline_button}
};

// Detail screen management
void power_monitor_create_detail_screen(void)
{
	ESP_LOGI(TAG, "=== CREATING DETAIL SCREEN (V2) ===");

	if (detail_screen) {
		ESP_LOGW(TAG, "Detail screen already exists");
		return;
	}

	// Create detail screen using shared template
	detail_screen_config_t config = {
		.module_name = "power-monitor",
		.display_name = "POWER MONITOR",
		.show_gauges_section = true,
		.show_settings_button = true,
		.show_status_indicators = false,
		.setting_buttons = power_monitor_buttons,
		.setting_buttons_count = 2,
		.on_back_clicked = power_monitor_handle_back_button,
		.on_view_clicked = NULL, // Handled directly by current view content
		.overlay_creator = NULL,
		.overlay_user_data = NULL
	};

	detail_screen = detail_screen_create(&config);
	if (detail_screen) {
		ESP_LOGI(TAG, "Detail screen created successfully");

		// Create gauges in the gauges container
		if (detail_screen->gauges_container) {
			power_monitor_create_detail_gauges(detail_screen->gauges_container);
		}

		// Sensor data labels creation moved to detail_screen.c
	} else {
		ESP_LOGE(TAG, "Failed to create detail screen");
	}
}

void power_monitor_show_detail_screen(void)
{
	ESP_LOGI(TAG, "=== SHOW DETAIL SCREEN (V2) ===");

	// Create if missing
	if (!detail_screen) {
		power_monitor_create_detail_screen();
	}

	if (detail_screen) {
		// Show the detail screen via template helper (idempotent create inside)
		detail_screen_show(detail_screen);
		ESP_LOGI(TAG, "Detail screen shown");

		// Debug: Log container size after initial content is added
		if (detail_screen->current_view_container) {
			ESP_LOGI(TAG, "Current view container size after initial content: %dx%d",
				lv_obj_get_width(detail_screen->current_view_container),
				lv_obj_get_height(detail_screen->current_view_container));
		}
	} else {
		ESP_LOGE(TAG, "Detail screen unavailable");
	}
}

void power_monitor_destroy_detail_screen(void)
{
	ESP_LOGI(TAG, "=== DESTROY DETAIL SCREEN ===");
	// No cleanup needed - everything will be created fresh
}

// Touch event handler for detail screen
void power_monitor_handle_detail_touch(void)
{
	ESP_LOGI(TAG, "=== HANDLE DETAIL TOUCH (V2) ===");
	power_monitor_cycle_view();
}

// Get current view type
power_monitor_view_type_t power_monitor_get_current_view_type(void)
{
	return get_current_view_type();
}

// Set current view type
void power_monitor_set_current_view_type(power_monitor_view_type_t view_type)
{
	// Find the view type in our available views array
	int view_count = sizeof(available_views) / sizeof(available_views[0]);
	for (int i = 0; i < view_count; i++) {
		if (available_views[i] == view_type) {
			// Note: We can't directly set the index in the shared manager
			// This would require adding a set_index function to the shared manager
			ESP_LOGI(TAG, "View type %d found at index %d (setting not implemented)", view_type, i);
			return;
		}
	}
	ESP_LOGW(TAG, "View type %d not found in available views, keeping current", view_type);
}

// Create current view in container (for detail screen)
void power_monitor_create_current_view_in_container(lv_obj_t* container)
{
	ESP_LOGI(TAG, "=== CREATE CURRENT VIEW IN CONTAINER (V2) ===");
	ESP_LOGI(TAG, "Container: %p", container);

	if (!container) {
		ESP_LOGE(TAG, "Container is NULL");
		return;
	}

	// Always update the current view (for view cycling)
	ESP_LOGI(TAG, "Updating current view content (always update for view cycling)");

	// Check if this is the gauges container
	if (detail_screen && container == detail_screen->gauges_container) {
		// This is the gauges container - create the gauges
		power_monitor_create_detail_gauges(container);
		ESP_LOGI(TAG, "GAUGES CONTAINER SETUP COMPLETE");
	}
	// Check if this is the sensor data section (for raw sensor values)
	else if (detail_screen && container == detail_screen->sensor_data_section) {
		// Sensor data labels creation moved to detail_screen.c
		ESP_LOGI(TAG, "SENSOR DATA SECTION SETUP COMPLETE (labels handled by detail_screen.c)");
	}
	else {
		// Default behavior - create current view content
		power_monitor_create_current_view_content(container);
		ESP_LOGI(TAG, "DEFAULT CONTAINER SETUP COMPLETE");
	}
}

// System reset function to clear corrupted state

// Modal close callback
static void power_monitor_close_modal(void)
{
	if (current_modal) {
				alerts_modal_hide(current_modal);
		alerts_modal_destroy(current_modal);
		current_modal = NULL;
		ESP_LOGI(TAG, "Modal closed");
	}
}

// Handle back button - return to home screen
void power_monitor_handle_back_button(void)
{
	ESP_LOGI(TAG, "=== BACK BUTTON CLICKED ===");

	// Clear modal if exists first
	if (current_modal) {
		alerts_modal_destroy(current_modal);
		current_modal = NULL;
	}

	// Reset all static variables
	s_rendering_in_progress = false;

	// Reset view state
	power_monitor_power_grid_view_reset_state();

	// Handle back button - this will destroy the detail screen and clean up containers
	power_monitor_navigation_hide_detail_screen();

	// Clear container references AFTER destruction
	g_home_container = NULL;
	g_detail_container = NULL;
	g_power_grid_view_container = NULL;
	g_starter_voltage_view_container = NULL;
}

// Handle alerts button - open alerts modal
void power_monitor_handle_alerts_button(void)
{
	ESP_LOGI(TAG, "=== ALERTS BUTTON CLICKED ===");
	ESP_LOGI(TAG, "ALERTS BUTTON HANDLER CALLED - THIS IS WORKING!");

	// Safety check: prevent multiple modals
	if (current_modal) {
		ESP_LOGW(TAG, "Modal already exists, destroying first");
		alerts_modal_destroy(current_modal);
		current_modal = NULL;
	}

	// Create alerts modal
	ESP_LOGI(TAG, "Creating alerts modal");
	current_modal = alerts_modal_create(power_monitor_close_modal);

	if (current_modal) {
		ESP_LOGI(TAG, "Showing alerts modal");
				alerts_modal_show(current_modal);
		ESP_LOGI(TAG, "Alerts modal opened");
	} else {
		ESP_LOGE(TAG, "Failed to create alerts modal");
	}
}

// Handle timeline button - open timeline modal (using alerts modal for now)
void power_monitor_handle_timeline_button(void)
{
	ESP_LOGI(TAG, "=== TIMELINE BUTTON CLICKED ===");

	// Close any existing modal
	if (current_modal) {
		alerts_modal_destroy(current_modal);
		current_modal = NULL;
	}

	// Create alerts modal (timeline functionality not implemented yet)
	current_modal = alerts_modal_create(power_monitor_close_modal);
	if (current_modal) {
				alerts_modal_show(current_modal);
		ESP_LOGI(TAG, "Timeline modal opened (using alerts modal)");
	} else {
		ESP_LOGE(TAG, "Failed to create timeline modal");
	}
}

// Legacy function implementations for compatibility
void power_monitor_open_alerts_modal(void)
{
	power_monitor_handle_alerts_button();
}

void power_monitor_open_timeline_modal(void)
{
	power_monitor_handle_timeline_button();
}

// Stub implementations for view_manager compatibility
void *power_monitor_detail_get_instance(void)
{
	// Return NULL since detail screen is handled differently in V2
	return NULL;
}

void power_monitor_detail_render_bar_graphs(void *view)
{
	// This is handled by the power grid view directly
	// No need to implement this stub
}

// Create view containers once and reuse them
static void power_monitor_create_view_containers(lv_obj_t* parent_container)
{
	// Prevent recursive calls
	if (s_creation_in_progress) {
		ESP_LOGW(TAG, "View container creation already in progress, skipping");
		return;
	}

	s_creation_in_progress = true;

	if (!parent_container) {
		ESP_LOGE(TAG, "Parent container is NULL");
		s_creation_in_progress = false;
		return;
	}

	if (!lv_obj_is_valid(parent_container)) {
		ESP_LOGE(TAG, "Parent container is not valid");
		s_creation_in_progress = false;
		return;
	}

	// Always create fresh containers - let device state machine handle state
	ESP_LOGI(TAG, "Creating fresh view containers");

	// Create power grid view container
		ESP_LOGI(TAG, "Creating power grid view container in parent: %p", parent_container);
		g_power_grid_view_container = lv_obj_create(parent_container);
		if (!g_power_grid_view_container) {
			ESP_LOGE(TAG, "Failed to create power grid view container");
			return;
		}
		// Fill the parent container completely
		lv_obj_set_size(g_power_grid_view_container, LV_PCT(100), LV_PCT(100));
		lv_obj_align(g_power_grid_view_container, LV_ALIGN_TOP_LEFT, 0, 0);
		lv_obj_set_style_bg_opa(g_power_grid_view_container, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(g_power_grid_view_container, 0, 0);

		// Force layout calculation to ensure container gets proper size
		lv_obj_update_layout(g_power_grid_view_container);
		ESP_LOGI(TAG, "Power grid view container size after layout: %dx%d",
			lv_obj_get_width(g_power_grid_view_container), lv_obj_get_height(g_power_grid_view_container));
		lv_obj_set_style_pad_all(g_power_grid_view_container, 0, 0);
		lv_obj_clear_flag(g_power_grid_view_container, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_flag(g_power_grid_view_container, LV_OBJ_FLAG_CLICKABLE);

		// Add touch callback
		if (parent_container == g_home_container) {
			lv_obj_add_event_cb(g_power_grid_view_container, power_monitor_home_current_view_touch_cb, LV_EVENT_CLICKED, NULL);
			ESP_LOGI(TAG, "Added home touch callback to power grid view container: %p", g_power_grid_view_container);
		} else if (parent_container == g_detail_container) {
			lv_obj_add_event_cb(g_power_grid_view_container, power_monitor_detail_current_view_touch_cb, LV_EVENT_CLICKED, NULL);
			ESP_LOGI(TAG, "Added detail touch callback to power grid view container: %p", g_power_grid_view_container);
		} else {
			ESP_LOGW(TAG, "Unknown parent container for power grid view: %p (home: %p, detail: %p)", parent_container, g_home_container, g_detail_container);
		}

	ESP_LOGI(TAG, "Power grid view container created: %p", g_power_grid_view_container);

	// Create starter voltage view container
		ESP_LOGI(TAG, "Creating starter voltage view container in parent: %p", parent_container);
		g_starter_voltage_view_container = lv_obj_create(parent_container);
		if (!g_starter_voltage_view_container) {
			ESP_LOGE(TAG, "Failed to create starter voltage view container");
			return;
		}
		ESP_LOGI(TAG, "Starter voltage view container created: %p", g_starter_voltage_view_container);
		// Fill the parent container completely
		lv_obj_set_size(g_starter_voltage_view_container, LV_PCT(100), LV_PCT(100));
		lv_obj_align(g_starter_voltage_view_container, LV_ALIGN_TOP_LEFT, 0, 0);
		lv_obj_set_style_bg_opa(g_starter_voltage_view_container, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(g_starter_voltage_view_container, 0, 0);

		// Force layout calculation to ensure container gets proper size
		lv_obj_update_layout(g_starter_voltage_view_container);
		ESP_LOGI(TAG, "Starter voltage view container size after layout: %dx%d",
			lv_obj_get_width(g_starter_voltage_view_container), lv_obj_get_height(g_starter_voltage_view_container));

		lv_obj_set_style_pad_all(g_starter_voltage_view_container, 0, 0);
		lv_obj_clear_flag(g_starter_voltage_view_container, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_flag(g_starter_voltage_view_container, LV_OBJ_FLAG_CLICKABLE);

		// Add touch callback
		if (parent_container == g_home_container) {
			lv_obj_add_event_cb(g_starter_voltage_view_container, power_monitor_home_current_view_touch_cb, LV_EVENT_CLICKED, NULL);
			ESP_LOGI(TAG, "Added home touch callback to starter voltage view container: %p", g_starter_voltage_view_container);
		} else if (parent_container == g_detail_container) {
			lv_obj_add_event_cb(g_starter_voltage_view_container, power_monitor_detail_current_view_touch_cb, LV_EVENT_CLICKED, NULL);
			ESP_LOGI(TAG, "Added detail touch callback to starter voltage view container: %p", g_starter_voltage_view_container);
		} else {
			ESP_LOGW(TAG, "Unknown parent container for starter voltage view: %p (home: %p, detail: %p)", parent_container, g_home_container, g_detail_container);
	}

	s_creation_in_progress = false;
}


// Simple current view rendering - reuse existing view containers
void power_monitor_render_current_view(lv_obj_t* container)
{
	ESP_LOGI(TAG, "=== RENDER CURRENT VIEW START ===");

	// Prevent recursive rendering
	if (s_rendering_in_progress) {
		ESP_LOGI(TAG, "Rendering already in progress, skipping");
		return;
	}
	s_rendering_in_progress = true;

	ESP_LOGI(TAG, "Rendering flag set, proceeding with view creation");

	ESP_LOGI(TAG, "=== RENDER CURRENT VIEW: About to create view containers ===");

	// Make container clickable for touch navigation
	lv_obj_add_flag(container, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

	// Always create fresh view containers
	ESP_LOGI(TAG, "=== CREATING FRESH VIEW CONTAINERS ===");
	power_monitor_create_view_containers(container);

	// Get current view type from shared manager
	power_monitor_view_type_t current_type = get_current_view_type();
	ESP_LOGI(TAG, "Showing view type: %d (index: %d)", current_type, current_view_manager_get_index());

	// Always render the current view fresh - no complex cleanup logic
	if (current_type == POWER_MONITOR_VIEW_BAR_GRAPH) {
			ESP_LOGI(TAG, "Rendering power grid view content");
			power_monitor_power_grid_view_render(g_power_grid_view_container);
			ESP_LOGI(TAG, "Power grid view shown and rendered");
	} else if (current_type == POWER_MONITOR_VIEW_NUMERICAL) {
			ESP_LOGI(TAG, "Rendering starter voltage view content");
			power_monitor_starter_voltage_view_render(g_starter_voltage_view_container);
			ESP_LOGI(TAG, "Starter voltage view shown and rendered");
	}

	// Check if content was actually created
	int child_count = lv_obj_get_child_cnt(container);
	ESP_LOGI(TAG, "After rendering, container has %d children", child_count);

	// Reset rendering flag
	s_rendering_in_progress = false;

	// CRITICAL: Ensure view state is consistent after rendering
	ESP_LOGI(TAG, "=== RENDER CURRENT VIEW: Final state check ===");
	ESP_LOGI(TAG, "Current view type: %d, index: %d", current_type, current_view_manager_get_index());
	ESP_LOGI(TAG, "Power grid container: %p, valid: %d", g_power_grid_view_container,
		g_power_grid_view_container ? lv_obj_is_valid(g_power_grid_view_container) : 0);
	ESP_LOGI(TAG, "Starter voltage container: %p, valid: %d", g_starter_voltage_view_container,
		g_starter_voltage_view_container ? lv_obj_is_valid(g_starter_voltage_view_container) : 0);

	ESP_LOGI(TAG, "=== RENDER CURRENT VIEW COMPLETE ===");
}

static void power_monitor_update_all_containers(void)
{
	ESP_LOGI(TAG, "=== UPDATE ALL CONTAINERS START ===");
	ESP_LOGI(TAG, "Current view index: %d", current_view_manager_get_index());

	ESP_LOGI(TAG, "Updating home container: %p, valid: %d", g_home_container, g_home_container ? lv_obj_is_valid(g_home_container) : 0);
	// Update home container if it exists
	if (g_home_container && lv_obj_is_valid(g_home_container)) {
		ESP_LOGI(TAG, "Updating current view data in home container");
		// Only update data, don't recreate the view
		power_monitor_power_grid_view_update_data();
		ESP_LOGI(TAG, "Home container update complete");
	}

	ESP_LOGI(TAG, "Updating detail container: %p, valid: %d", g_detail_container, g_detail_container ? lv_obj_is_valid(g_detail_container) : 0);
	// Update detail container if it exists
	if (g_detail_container && lv_obj_is_valid(g_detail_container)) {
		ESP_LOGI(TAG, "Updating current view data in detail container");
		// Only update data, don't recreate the view
		power_monitor_power_grid_view_update_data();
		ESP_LOGI(TAG, "Detail container update complete");
	}

	ESP_LOGI(TAG, "=== UPDATE ALL CONTAINERS COMPLETE ===");
}

// Touch callback for home current view - navigates to detail screen
static void power_monitor_home_current_view_touch_cb(lv_event_t * e)
{
	static int touch_count = 0;
	touch_count++;
	ESP_LOGI(TAG, "*** HOME TOUCH CALLBACK CALLED #%d ***", touch_count);

	// Prevent recursive calls
	if (s_reset_in_progress || s_creation_in_progress) {
		ESP_LOGW(TAG, "Home touch callback ignored - navigation in progress");
		return;
	}

	ESP_LOGI(TAG, "Home current view touched - navigating to detail screen");

	// Update detail container with current view before navigating
	if (g_detail_container) {
		power_monitor_render_current_view(g_detail_container);
	}

	// Use the proper navigation function to update device state
	extern void screen_navigation_request_detail_view(const char *module_name);
	screen_navigation_request_detail_view("power-monitor");
}

// Touch callback for detail current view - cycles through views
static void power_monitor_detail_current_view_touch_cb(lv_event_t * e)
{
	static int touch_count = 0;
	touch_count++;
	ESP_LOGI(TAG, "*** DETAIL TOUCH CALLBACK CALLED #%d ***", touch_count);

	// Debug: Check event details
	if (e) {
		ESP_LOGI(TAG, "Event type: %d, target: %p, current target: %p",
			lv_event_get_code(e), lv_event_get_target(e), lv_event_get_current_target(e));
	}

	// Check if we're actually on the detail screen
	extern screen_type_t screen_navigation_get_current_screen(void);
	screen_type_t current_screen = screen_navigation_get_current_screen();
	ESP_LOGI(TAG, "Current screen: %d (detail=%d)", current_screen, SCREEN_DETAIL_VIEW);

	if (current_screen != SCREEN_DETAIL_VIEW) {
		ESP_LOGW(TAG, "Detail touch callback called but not on detail screen, ignoring");
		return;
	}

	// Prevent recursive calls
	if (s_reset_in_progress || s_creation_in_progress) {
		ESP_LOGW(TAG, "Detail touch callback ignored - navigation in progress");
		return;
	}

	ESP_LOGI(TAG, "Detail current view touched - cycling views");
	power_monitor_cycle_current_view();
}

// =============================================================================
// VIEW STATE MANAGEMENT
// =============================================================================

/**
 * @brief Get current view index from device state (with fallback)
 */
static int power_monitor_get_view_index(void)
{
	// Try to get from device state first
	extern int module_screen_view_get_view_index(const char *module_name);
	int index = module_screen_view_get_view_index("power-monitor");

	// Validate and clamp the index
	if (index < 0 || index >= s_total_views) {
		ESP_LOGW(TAG, "Invalid view index %d from device state, using 0", index);
		index = 0;
	}

	return index;
}

/**
 * @brief Set current view index in device state
 */
static void power_monitor_set_view_index(int index)
{
	// Validate and clamp the index
	if (index < 0 || index >= s_total_views) {
		ESP_LOGW(TAG, "Invalid view index %d, clamping to valid range", index);
		index = (index < 0) ? 0 : (s_total_views - 1);
	}

	ESP_LOGI(TAG, "Setting view index to %d", index);

	// Set in device state
	extern void module_screen_view_set_view_index(const char *module_name, int view_index);
	module_screen_view_set_view_index("power-monitor", index);

	// Save device state to persist across resets
	extern void device_state_save(void);
	device_state_save();
}

// =============================================================================
// VIEW LIFECYCLE MANAGEMENT
// =============================================================================

/**
 * @brief Destroy the current view objects properly
 */
static void power_monitor_destroy_current_view(void)
{
	int view_index = power_monitor_get_view_index();
	ESP_LOGI(TAG, "Destroying current view objects for index %d", view_index);

	// CRITICAL: Cleanup gauge canvas buffers BEFORE destroying LVGL objects
	// This prevents memory leaks from malloc'd canvas buffers
	ESP_LOGI(TAG, "Cleaning up gauge canvas buffers before LVGL object destruction");
	extern void power_monitor_reset_static_gauges(void);
	extern void power_monitor_reset_starter_voltage_static_gauge(void);
	power_monitor_reset_static_gauges();
	power_monitor_reset_starter_voltage_static_gauge();

	// Get the detail screen container to clean
	extern detail_screen_t* detail_screen;
	if (detail_screen && detail_screen->current_view_container) {
		ESP_LOGI(TAG, "Cleaning current view container");
		lv_obj_clean(detail_screen->current_view_container);

		// Re-apply container styling after clean (clean removes styling)
		lv_obj_set_style_bg_color(detail_screen->current_view_container, lv_color_hex(0x000000), 0);
		lv_obj_set_style_border_width(detail_screen->current_view_container, 1, 0);
		lv_obj_set_style_border_color(detail_screen->current_view_container, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_radius(detail_screen->current_view_container, 4, 0);
		lv_obj_clear_flag(detail_screen->current_view_container, LV_OBJ_FLAG_SCROLLABLE);
	} else {
		ESP_LOGW(TAG, "No detail screen container to clean");
	}

	ESP_LOGI(TAG, "Current view objects destroyed for index %d", view_index);
}

// =============================================================================
// STANDARDIZED MODULE INTERFACE IMPLEMENTATION
// =============================================================================

/**
 * @brief Standardized module initialization function
 */
static void power_monitor_module_init(void)
{
	ESP_LOGI(TAG, "Power monitor module initializing via standardized interface");
	power_monitor_init_with_default_view(POWER_MONITOR_VIEW_BAR_GRAPH);
}

/**
 * @brief Standardized module update function
 *
 * This function is called every main loop tick to update module data.
 * It should NOT create or destroy UI elements, only update data.
 */
static void power_monitor_module_update(void)
{
	// Skip data updates during view cycling to prevent crashes
	if (s_detail_view_needs_refresh) {
		ESP_LOGD(TAG, "Skipping data updates during view cycling");
	} else {
		// Update data only - no UI creation/destruction
		power_monitor_update_data_only();
	}

	// Handle delayed detail view refresh after view cycling
	if (s_detail_view_needs_refresh) {
		extern screen_type_t screen_navigation_get_current_screen(void);
		if (screen_navigation_get_current_screen() == SCREEN_DETAIL_VIEW) {
			extern detail_screen_t* detail_screen;
			if (detail_screen && detail_screen->current_view_container) {
			ESP_LOGI(TAG, "Performing delayed re-render of detail view after cycle");

			// DESTROY: Properly destroy the current view object
			ESP_LOGI(TAG, "Destroying current view object");
			power_monitor_destroy_current_view();

			// Ensure layout is calculated on parent container before creating new view
			lv_obj_update_layout(detail_screen->left_column);
			lv_obj_update_layout(detail_screen->current_view_container); // Also update the current view container itself
			ESP_LOGI(TAG, "Container size after parent layout update: %dx%d",
				lv_obj_get_width(detail_screen->current_view_container),
				lv_obj_get_height(detail_screen->current_view_container));

			// CREATE: Create new view object with updated index
			int current_index = power_monitor_get_view_index();
			ESP_LOGI(TAG, "Creating new view object for index %d", current_index);

			// Force a final layout update on the container before creating content
			lv_obj_update_layout(detail_screen->current_view_container);
			ESP_LOGI(TAG, "Final container size before content creation: %dx%d",
				lv_obj_get_width(detail_screen->current_view_container),
				lv_obj_get_height(detail_screen->current_view_container));

			power_monitor_create_current_view_in_container(detail_screen->current_view_container);

				s_detail_view_needs_refresh = false;
			}
		} else {
			// Not on detail screen anymore, clear the flag
			s_detail_view_needs_refresh = false;
		}
	}
}

/**
 * @brief Standardized module cleanup function
 */
static void power_monitor_module_cleanup(void)
{
	ESP_LOGI(TAG, "Power monitor module cleaning up via standardized interface");
	power_monitor_cleanup();
}

/**
 * @brief Power monitor module interface definition
 *
 * This structure provides the standardized interface that main.c
 * can use to interact with this module without knowing its internals.
 */
const display_module_t power_monitor_module = {
	.name = "power-monitor",
	.init = power_monitor_module_init,
	.update = power_monitor_module_update,
	.cleanup = power_monitor_module_cleanup
};