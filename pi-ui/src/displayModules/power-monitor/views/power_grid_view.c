#include <stdio.h>
#include <string.h>

#include "../power-monitor.h"
#include "power_grid_view.h"

#include "../../../state/device_state.h"
#include "../../../data/lerp_data/lerp_data.h"

#include "../../shared/gauges/bar_graph_gauge.h"

#include "../../../fonts/lv_font_noplato_24.h"

static const char *TAG = "power_grid_view";

// View initialization flag
static bool s_view_initialized = false;

// Container tracking for cleanup
static lv_obj_t* s_row_containers[3] = {NULL, NULL, NULL}; // Track row containers for cleanup

// ============================================================================
// LAYOUT CONFIGURATION - Edit these values to change the layout
// ============================================================================

// Container padding from edges
#define CONTAINER_PADDING_PX        4     // Padding from container edges

// Row layout split (numeric value : bar graph)
#define NUMERIC_VALUE_PERCENT      20     // Percentage of width for numeric value
#define BAR_GRAPH_PERCENT          80     // Percentage of width for bar graph

// Gauge spacing
#define GAUGE_PADDING_PX            1     // Vertical padding between gauges

// ============================================================================

// Helper function to create a gauge row with 20:80 split using flexbox
static void create_gauge_row(
	lv_obj_t* parent, bar_graph_gauge_t* gauge,
	lv_obj_t** value_label, lv_obj_t** title_label,
	const char* title_text, lv_color_t color,
	int container_width, int gauge_height,
	float baseline, float min_val, float max_val,
	bar_graph_mode_t mode, const lv_font_t* font,
	int row_index
){
	// ROW CONTAINER : full width, Numeric and Gauge
	lv_obj_t* row_container = lv_obj_create(parent);
	lv_obj_set_size(row_container, LV_PCT(100), gauge_height);
	lv_obj_set_style_bg_opa(row_container, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(row_container, 0, 0); // No border

	// Track container for cleanup
	if (row_index >= 0 && row_index < 3) {
		s_row_containers[row_index] = row_container;
	}

	lv_obj_set_style_radius(row_container, 0, 0); // No border radius
	lv_obj_set_style_pad_all(row_container, 0, 0);
	lv_obj_clear_flag(row_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(row_container, LV_OBJ_FLAG_EVENT_BUBBLE);

	// Set up flexbox for row (horizontal: 20% numeric + 80% gauge)
	lv_obj_set_flex_flow(row_container, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(row_container, 2, 0); // 2px gap between numeric and gauge

	// NUMERIC CONTAINER : 20% of width

	lv_obj_t* numeric_container = lv_obj_create(row_container);
	int numeric_container_width = (container_width * NUMERIC_VALUE_PERCENT) / 100;
	lv_obj_set_size(numeric_container, numeric_container_width, LV_SIZE_CONTENT);
	lv_obj_set_style_bg_opa(numeric_container, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(numeric_container, 0, 0); // No border
	lv_obj_set_style_radius(numeric_container, 0, 0); // No border radius
	lv_obj_set_style_pad_all(numeric_container, 0, 0);
	lv_obj_clear_flag(numeric_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(numeric_container, LV_OBJ_FLAG_EVENT_BUBBLE);

	lv_obj_set_flex_flow(numeric_container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(numeric_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(numeric_container, 0, 0);

	// Number Value for Gauge Row
	*value_label = lv_label_create(numeric_container);
	lv_label_set_text(*value_label, "00.0");
	lv_obj_set_size(*value_label, 50, LV_SIZE_CONTENT); // Fixed width for 4 characters (00.0)
	lv_obj_set_style_text_color(*value_label, color, 0);
	lv_obj_set_style_text_font(*value_label, font, 0); // Use passed font
	lv_obj_set_style_text_align(*value_label, LV_TEXT_ALIGN_RIGHT, 0);
	lv_obj_set_style_pad_all(*value_label, 0, 0); // Remove any internal padding
	lv_obj_set_style_border_width(*value_label, 0, 0); // No border
	lv_obj_set_style_radius(*value_label, 0, 0); // No border radius
	lv_obj_clear_flag(*value_label, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(*value_label, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(*value_label, LV_OBJ_FLAG_EVENT_BUBBLE);
	// Layout updates disabled by fixed size
	lv_obj_set_style_text_decor(*value_label, LV_TEXT_DECOR_NONE, 0); // No text decoration
	lv_obj_set_style_text_letter_space(*value_label, 0, 0); // No letter spacing changes
	lv_obj_set_style_text_line_space(*value_label, 0, 0); // No line spacing changes

	// Create title label with natural sizing for proper centering
	*title_label = lv_label_create(numeric_container);
	lv_label_set_text(*title_label, title_text);
	lv_obj_set_size(*title_label, LV_SIZE_CONTENT, LV_SIZE_CONTENT); // Let label size itself naturally
	lv_obj_set_style_text_color(*title_label, color, 0);
	lv_obj_set_style_text_font(*title_label, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_align(*title_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_pad_all(*title_label, 0, 0); // Remove any internal padding
	lv_obj_set_style_border_width(*title_label, 0, 0); // No border
	lv_obj_set_style_radius(*title_label, 0, 0); // No border radius
	lv_obj_clear_flag(*title_label, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(*title_label, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(*title_label, LV_OBJ_FLAG_EVENT_BUBBLE);
	// Layout updates disabled by fixed size
	lv_obj_set_style_text_decor(*title_label, LV_TEXT_DECOR_NONE, 0); // No text decoration
	lv_obj_set_style_text_letter_space(*title_label, 0, 0); // No letter spacing changes
	lv_obj_set_style_text_line_space(*title_label, 0, 0); // No line spacing changes

	// Create gauge container (80% of width) - use fixed width to prevent flexbox flooding
	lv_obj_t* gauge_container = lv_obj_create(row_container);
	int gauge_container_width = (container_width * BAR_GRAPH_PERCENT) / 100 - 6; // Shrink by 6px total
	lv_obj_set_size(gauge_container, gauge_container_width, gauge_height - 2); // 2px padding from bottom
	lv_obj_set_style_bg_opa(gauge_container, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(gauge_container, 0, 0); // No border
	lv_obj_set_style_radius(gauge_container, 0, 0); // No border radius
	lv_obj_set_style_pad_all(gauge_container, 0, 0);
	lv_obj_set_style_pad_bottom(gauge_container, 2, 0); // 2px bottom padding (overrides pad_all)
	lv_obj_clear_flag(gauge_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(gauge_container, LV_OBJ_FLAG_EVENT_BUBBLE);

	// Create gauge (shrink by 6px to give more space for numeric values)
	int gauge_width = (container_width * BAR_GRAPH_PERCENT) / 100 - 6;
	bar_graph_gauge_init(gauge, gauge_container, 0, 0, gauge_width, gauge_height, 3, 1);
	bar_graph_gauge_configure_advanced(
		gauge, mode, baseline, min_val, max_val,
		"", "V", "V", lv_color_to_int(color), false, true, false // No border for current view
	);
}

// Static bar graph gauges for this view (temporarily reverted from shared)
static bar_graph_gauge_t s_starter_voltage_gauge;
static bar_graph_gauge_t s_house_voltage_gauge;
static bar_graph_gauge_t s_solar_voltage_gauge;

// Static numeric value labels and title labels for each gauge
static lv_obj_t* s_starter_value_label = NULL;
static lv_obj_t* s_starter_title_label = NULL;
static lv_obj_t* s_house_value_label = NULL;
static lv_obj_t* s_house_title_label = NULL;
static lv_obj_t* s_solar_value_label = NULL;
static lv_obj_t* s_solar_title_label = NULL;

// Value editor for interactive editing
static int s_current_editing_gauge = -1; // -1 = none, 0 = starter, 1 = house, 2 = solar

void power_monitor_power_grid_view_render(lv_obj_t *container)
{

	// Reset view state
	s_view_initialized = false;

	// Force container to be visible and sized
	lv_obj_clear_flag(container, LV_OBJ_FLAG_HIDDEN);

	// Force the container to have a minimum size if it's 0x0
	lv_coord_t container_width = lv_obj_get_width(container);
	lv_coord_t container_height = lv_obj_get_height(container);

	if (container_width == 0 || container_height == 0) {

		lv_obj_set_size(container, 238, 189);
		lv_obj_update_layout(container);

		container_width = lv_obj_get_width(container);
		container_height = lv_obj_get_height(container);
	}

	// Set container background to black (border is handled by parent container)
	lv_obj_set_style_bg_color(container, lv_color_hex(0x000000), 0);
	lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
	// Don't modify border_width - let parent container handle it
	lv_obj_set_style_pad_all(container, 0, 0);
	lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

	// Container clickability is handled by parent - just clear scrollable
	lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

	// Use the corrected container dimensions
	printf("[I] power_grid_view: Power grid container dimensions: %dx%d\n", container_width, container_height);

	// Use the container as-is - don't resize it!
	printf("[I] power_grid_view: Using container dimensions as-is: %dx%d\n", container_width, container_height);

	// Set up flexbox for the main container (vertical stack)
	lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_gap(container, 0, 0); // No vertical gap between gauges
	lv_obj_set_style_pad_all(container, CONTAINER_PADDING_PX, 0); // Container padding

	// Calculate gauge dimensions using configuration
	int gauge_height = (container_height - CONTAINER_PADDING_PX) / 3;

	printf("[D] power_grid_view: Container dimensions: %dx%d, gauge_height=%d\n", container_width, container_height, gauge_height);

	// Read actual gauge configuration values from device state
	float starter_baseline = device_state_get_float("power_monitor.starter_baseline_voltage_v");
	float starter_min = device_state_get_float("power_monitor.starter_min_voltage_v");
	float starter_max = device_state_get_float("power_monitor.starter_max_voltage_v");

	float house_baseline = device_state_get_float("power_monitor.house_baseline_voltage_v");
	float house_min = device_state_get_float("power_monitor.house_min_voltage_v");
	float house_max = device_state_get_float("power_monitor.house_max_voltage_v");

	float solar_min = device_state_get_float("power_monitor.solar_min_voltage_v");
	float solar_max = device_state_get_float("power_monitor.solar_max_voltage_v");

	// Create gauge rows with 20:80 split using flexbox helper function
	// Initialize gauge structures
	memset(&s_starter_voltage_gauge, 0, sizeof(bar_graph_gauge_t));
	memset(&s_house_voltage_gauge, 0, sizeof(bar_graph_gauge_t));
	memset(&s_solar_voltage_gauge, 0, sizeof(bar_graph_gauge_t));

	create_gauge_row(
		container, &s_starter_voltage_gauge,
		&s_starter_value_label, &s_starter_title_label,
		"CABIN\n(V)", lv_color_hex(0x00FF00),
		container_width, gauge_height,
		starter_baseline, starter_min, starter_max,
		BAR_GRAPH_MODE_BIPOLAR, &lv_font_noplato_24, 0
	);

	create_gauge_row(
		container, &s_house_voltage_gauge,
		&s_house_value_label, &s_house_title_label,
		"HOUSE\n(V)", lv_color_hex(0x0080FF),
		container_width, gauge_height,
		house_baseline, house_min, house_max,
		BAR_GRAPH_MODE_BIPOLAR, &lv_font_noplato_24, 1
	);

	create_gauge_row(
		container, &s_solar_voltage_gauge,
		&s_solar_value_label, &s_solar_title_label,
		"SOLAR\n(V)", lv_color_hex(0xFF8000),
		container_width, gauge_height,
		0.0f, solar_min, solar_max,
		BAR_GRAPH_MODE_POSITIVE_ONLY, &lv_font_noplato_24, 2
	);

	// Update labels and ticks for Y-axis (show ticks but not values)
	bar_graph_gauge_update_labels_and_ticks(&s_starter_voltage_gauge);
	bar_graph_gauge_update_labels_and_ticks(&s_house_voltage_gauge);
	bar_graph_gauge_update_labels_and_ticks(&s_solar_voltage_gauge);

	// Add initial data to make gauges visible
	bar_graph_gauge_add_data_point(&s_starter_voltage_gauge, 12.5f);
	bar_graph_gauge_add_data_point(&s_house_voltage_gauge, 13.2f);
	bar_graph_gauge_add_data_point(&s_solar_voltage_gauge, 14.1f);

	// Force canvas updates to make data visible
	bar_graph_gauge_update_canvas(&s_starter_voltage_gauge);
	bar_graph_gauge_update_canvas(&s_house_voltage_gauge);
	bar_graph_gauge_update_canvas(&s_solar_voltage_gauge);

	// Mark view as initialized
	s_view_initialized = true;
}

// Reset view state when view is destroyed
void power_monitor_power_grid_view_reset_state(void)
{
	s_view_initialized = false;
}


void power_monitor_power_grid_view_update_data(void)
{
	// Check if view is still valid before updating
	if (!s_view_initialized) {
		// View not initialized, skipping update
		return;
	}

	// Updating data

	// Use LERP display values for smooth bars
	lerp_power_monitor_data_t lerp;
	lerp_data_get_current(&lerp);
	float starter_voltage = lerp_value_get_display(&lerp.starter_voltage);
	float house_voltage = lerp_value_get_display(&lerp.house_voltage);
	float solar_voltage = lerp_value_get_display(&lerp.solar_voltage);

	// Update bar graphs with data from LERP system
	// Note: bar_graph_gauge_add_data_point() already calls bar_graph_gauge_update_canvas() internally

	if (s_starter_voltage_gauge.initialized) {
		// Add safety check for gauge validity
		if (s_starter_voltage_gauge.container && lv_obj_is_valid(s_starter_voltage_gauge.container)) {
			bar_graph_gauge_add_data_point(&s_starter_voltage_gauge, starter_voltage);
		} else {
			printf("[W] power_grid_view: Starter voltage gauge container is invalid, skipping data update\n");
		}

		// Update numeric value label with current value
		if (s_starter_value_label && lv_obj_is_valid(s_starter_value_label)) {
			char value_text[16];
			snprintf(value_text, sizeof(value_text), "%.1f", starter_voltage);
			lv_label_set_text(s_starter_value_label, value_text);
		}
	}

	if (s_house_voltage_gauge.initialized) {
		// Add safety check for gauge validity
		if (s_house_voltage_gauge.container && lv_obj_is_valid(s_house_voltage_gauge.container)) {
			bar_graph_gauge_add_data_point(&s_house_voltage_gauge, house_voltage);
		} else {
			printf("[W] power_grid_view: House voltage gauge container is invalid, skipping data update\n");
		}

		// Update numeric value label with current value
		if (s_house_value_label && lv_obj_is_valid(s_house_value_label)) {
			char value_text[16];
			snprintf(value_text, sizeof(value_text), "%.1f", house_voltage);
			lv_label_set_text(s_house_value_label, value_text);
		}
	}

	if (s_solar_voltage_gauge.initialized) {
		// Add safety check for gauge validity
		if (s_solar_voltage_gauge.container && lv_obj_is_valid(s_solar_voltage_gauge.container)) {
			bar_graph_gauge_add_data_point(&s_solar_voltage_gauge, solar_voltage);
		} else {
			printf("[W] power_grid_view: Solar voltage gauge container is invalid, skipping data update\n");
		}

		// Update numeric value label with current value
		if (s_solar_value_label && lv_obj_is_valid(s_solar_value_label)) {
			char value_text[16];
			snprintf(value_text, sizeof(value_text), "%.1f", solar_voltage);
			lv_label_set_text(s_solar_value_label, value_text);
		}
	}

	// Update gauge intervals and timeline durations
	power_grid_view_update_gauge_intervals();

	// Mark view as initialized for proper cleanup
	s_view_initialized = true;
	// printf("[I] power_grid_view: Power grid view render complete, view initialized\n");
}


void power_monitor_reset_static_gauges(void)
{
	printf("[I] power_grid_view: === RESETTING STATIC GAUGES ===\n");

	// Properly cleanup all bar graph gauges to free canvas buffers
	if (s_starter_voltage_gauge.initialized) {
		printf("[I] power_grid_view: Cleaning up starter voltage gauge canvas buffer\n");
		bar_graph_gauge_cleanup(&s_starter_voltage_gauge);
	}
	if (s_house_voltage_gauge.initialized) {
		printf("[I] power_grid_view: Cleaning up house voltage gauge canvas buffer\n");
		bar_graph_gauge_cleanup(&s_house_voltage_gauge);
	}
	if (s_solar_voltage_gauge.initialized) {
		printf("[I] power_grid_view: Cleaning up solar voltage gauge canvas buffer\n");
		bar_graph_gauge_cleanup(&s_solar_voltage_gauge);
	}

	// Reset all static gauge variables to prevent memory conflicts
	memset(&s_starter_voltage_gauge, 0, sizeof(bar_graph_gauge_t));
	memset(&s_house_voltage_gauge, 0, sizeof(bar_graph_gauge_t));
	memset(&s_solar_voltage_gauge, 0, sizeof(bar_graph_gauge_t));

	// Reset other static variables
	s_current_editing_gauge = -1;
	s_view_initialized = false;

	// Reset row container pointers
	for (int i = 0; i < 3; i++) {
		s_row_containers[i] = NULL;
	}

	// Reset label pointers
	s_starter_value_label = NULL;
	s_starter_title_label = NULL;
	s_house_value_label = NULL;
	s_house_title_label = NULL;
	s_solar_value_label = NULL;
	s_solar_title_label = NULL;

	printf("[I] power_grid_view: Static gauges reset complete\n");
}

// Apply alert flashing to current view values
void power_monitor_power_grid_view_apply_alert_flashing(const power_monitor_data_t* data, int starter_lo, int starter_hi, int house_lo, int house_hi, int solar_lo, int solar_hi, bool blink_on)
{
	if (!data) return;

	// Get LERP data for raw values (for threshold checking)
	lerp_power_monitor_data_t lerp_data;
	lerp_data_get_current(&lerp_data);

	// Apply alert flashing to starter voltage value - use raw value for threshold checking
	float starter_v_raw = lerp_value_get_raw(&lerp_data.starter_voltage);
	bool starter_alert = (starter_v_raw <= (float)starter_lo) || (starter_v_raw >= (float)starter_hi);

	if (starter_alert && s_starter_value_label) {
		// Only change color, no hiding/showing to prevent layout shifts
		if (blink_on) {
			lv_obj_set_style_text_color(s_starter_value_label, lv_color_hex(0xFF3333), 0); // Red when blinking
		} else {
			lv_obj_set_style_text_color(s_starter_value_label, lv_color_hex(0x00FF00), 0); // Green when not blinking
		}
	} else if (s_starter_value_label) {
		// Normal state - always green
		lv_obj_set_style_text_color(s_starter_value_label, lv_color_hex(0x00FF00), 0);
	}

	// Apply alert flashing to house voltage value - use raw value for threshold checking
	float house_v_raw = lerp_value_get_raw(&lerp_data.house_voltage);
	bool house_alert = (house_v_raw <= (float)house_lo) || (house_v_raw >= (float)house_hi);

	if (house_alert && s_house_value_label) {
		// Only change color, no hiding/showing to prevent layout shifts
		if (blink_on) {
			lv_obj_set_style_text_color(s_house_value_label, lv_color_hex(0xFF3333), 0); // Red when blinking
		} else {
			lv_obj_set_style_text_color(s_house_value_label, lv_color_hex(0x0080FF), 0); // Blue when not blinking
		}
	} else if (s_house_value_label) {
		// Normal state - always blue
		lv_obj_set_style_text_color(s_house_value_label, lv_color_hex(0x0080FF), 0);
	}

	// Apply alert flashing to solar voltage value - use raw value for threshold checking
	float solar_v_raw = lerp_value_get_raw(&lerp_data.solar_voltage);
	bool solar_alert = (solar_v_raw <= (float)solar_lo) || (solar_v_raw >= (float)solar_hi);

	if (solar_alert && s_solar_value_label) {
		// Only change color, no hiding/showing to prevent layout shifts
		if (blink_on) {
			lv_obj_set_style_text_color(s_solar_value_label, lv_color_hex(0xFF3333), 0); // Red when blinking
		} else {
			lv_obj_set_style_text_color(s_solar_value_label, lv_color_hex(0xFF8000), 0); // Orange when not blinking
		}
	} else if (s_solar_value_label) {
		// Normal state - always orange
		lv_obj_set_style_text_color(s_solar_value_label, lv_color_hex(0xFF8000), 0);
	}
}

// Function to update gauge configuration with current device state values
void power_monitor_power_grid_view_update_configuration(void)
{
	if (!s_view_initialized) return;

	printf("[I] power_grid_view: Updating power grid view gauge configuration...\n");

	// Read actual gauge configuration values from device state
	float starter_baseline = device_state_get_float("power_monitor.starter_baseline_voltage_v");
	float starter_min = device_state_get_float("power_monitor.starter_min_voltage_v");
	float starter_max = device_state_get_float("power_monitor.starter_max_voltage_v");
	float house_baseline = device_state_get_float("power_monitor.house_baseline_voltage_v");
	float house_min = device_state_get_float("power_monitor.house_min_voltage_v");
	float house_max = device_state_get_float("power_monitor.house_max_voltage_v");
	float solar_min = 0.0f;     // Fixed minimum display range (solar can be 0)
	float solar_max = 25.0f;    // Fixed maximum display range (solar can be higher)

	// Update starter gauge configuration
	if (s_starter_voltage_gauge.initialized) {

		bar_graph_gauge_configure_advanced(&s_starter_voltage_gauge,
			BAR_GRAPH_MODE_BIPOLAR, starter_baseline, starter_min, starter_max,
			"", "V", "V", 0x00FF00, false, true, false // No border for current view
		);
	}

	// Update house gauge configuration
	if (s_house_voltage_gauge.initialized) {

		bar_graph_gauge_configure_advanced(&s_house_voltage_gauge,
			BAR_GRAPH_MODE_BIPOLAR, house_baseline, house_min, house_max,
			"", "V", "V", 0x0080FF, false, true, false // No border for current view
		);
	}

	// Update solar gauge configuration
	if (s_solar_voltage_gauge.initialized) {

		bar_graph_gauge_configure_advanced(&s_solar_voltage_gauge,
			BAR_GRAPH_MODE_POSITIVE_ONLY, 0.0f, solar_min, solar_max,
			"", "V", "V", 0xFF8000, false, true, false // No border for current view
		);
	}
}

// Update power grid view gauge timelines based on per-gauge timeline settings
void power_grid_view_update_gauge_intervals(void)
{
	// Update starter voltage gauge timeline
	if (s_starter_voltage_gauge.initialized) {
		int timeline_duration = device_state_get_int("power_monitor.gauge_timeline_settings.starter_voltage.current_view");
		uint32_t timeline_duration_ms = timeline_duration * 1000;
		bar_graph_gauge_set_timeline_duration(&s_starter_voltage_gauge, timeline_duration_ms);
	}

	// Update house voltage gauge timeline
	if (s_house_voltage_gauge.initialized) {
		int timeline_duration = device_state_get_int("power_monitor.gauge_timeline_settings.house_voltage.current_view");
		uint32_t timeline_duration_ms = timeline_duration * 1000;
		bar_graph_gauge_set_timeline_duration(&s_house_voltage_gauge, timeline_duration_ms);
	}

	// Update solar voltage gauge timeline
	if (s_solar_voltage_gauge.initialized) {
		int timeline_duration = device_state_get_int("power_monitor.gauge_timeline_settings.solar_voltage.current_view");
		uint32_t timeline_duration_ms = timeline_duration * 1000;
		bar_graph_gauge_set_timeline_duration(&s_solar_voltage_gauge, timeline_duration_ms);
	}
}
