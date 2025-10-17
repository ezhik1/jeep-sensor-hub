#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "power-monitor.h"

// State
#include "../../state/device_state.h"

// Data
#include "../../data/mock_data/mock_data.h"
#include "../../data/lerp_data/lerp_data.h"
#include "../../data/config.h"

// Views
#include "views/power_grid_view.h"
#include "views/starter_voltage_view.h"


// Shared Modules
#include "../shared/module_interface.h"
#include "../shared/current_view/current_view_manager.h"
#include "../shared/alerts_modal/alerts_modal.h"
#include "../shared/timeline_modal/timeline_modal.h"
#include "../shared/gauges/bar_graph_gauge.h"

#include "../../screens/screen_manager.h"
#include "../../screens/detail_screen/detail_screen.h"
#include "../../screens/home_screen/home_screen.h"

// Module Configurations
#include "voltage_alerts_config.h"
#include "timeline_modal_config.h"

// UI Styling
#include "../../displayModules/shared/palette.h"
#include "../../fonts/lv_font_noplato_24.h"


// Power monitor data defaults
#define POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS 30
#define POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS 30

#define POWER_MONITOR_DEFAULT_STARTER_ALERT_LOW_VOLTAGE_V 11.0f
#define POWER_MONITOR_DEFAULT_STARTER_ALERT_HIGH_VOLTAGE_V 14.0f
#define POWER_MONITOR_DEFAULT_STARTER_BASELINE_VOLTAGE_V 12.6f
#define POWER_MONITOR_DEFAULT_STARTER_MIN_VOLTAGE_V	11.0f
#define POWER_MONITOR_DEFAULT_STARTER_MAX_VOLTAGE_V 14.4f

#define POWER_MONITOR_DEFAULT_HOUSE_ALERT_LOW_VOLTAGE_V	11.0f
#define POWER_MONITOR_DEFAULT_HOUSE_ALERT_HIGH_VOLTAGE_V 14.0f
#define POWER_MONITOR_DEFAULT_HOUSE_BASELINE_VOLTAGE_V	12.6f
#define POWER_MONITOR_DEFAULT_HOUSE_MIN_VOLTAGE_V 11.0f
#define POWER_MONITOR_DEFAULT_HOUSE_MAX_VOLTAGE_V 14.4f

#define POWER_MONITOR_DEFAULT_SOLAR_ALERT_LOW_VOLTAGE_V	12.0f
#define POWER_MONITOR_DEFAULT_SOLAR_ALERT_HIGH_VOLTAGE_V 22.0f
#define POWER_MONITOR_DEFAULT_SOLAR_MIN_VOLTAGE_V 0.0f
#define POWER_MONITOR_DEFAULT_SOLAR_MAX_VOLTAGE_V 20.0f

// Forward declarations
static void power_monitor_cycle_view(void);
static void power_monitor_close_modal(void);

void power_monitor_create_current_view_content(lv_obj_t* container);
static void power_monitor_home_current_view_touch_cb(lv_event_t * e);
static void power_monitor_detail_current_view_touch_cb(lv_event_t * e);
static void power_monitor_create_detail_gauges(lv_obj_t* container);
static void power_monitor_update_detail_gauges(void);
static power_monitor_data_t* power_monitor_get_data_internal(void);
void power_monitor_render_current_view(lv_obj_t* container);
void power_monitor_cleanup_internal(void);
static void power_monitor_destroy_current_view(void);
static int power_monitor_get_view_index(void);
static void power_monitor_set_view_index(int index);

static const char *TAG = "power_monitor";

// Flag to indicate detail view needs refresh after view cycling
static bool s_detail_view_needs_refresh = false;


// Module-specific data
static power_monitor_data_t power_data = {0};

// Removed unused home_overlay
static detail_screen_t* detail_screen = NULL;

// Global view initialization flags
static bool s_rendering_in_progress = false;

// Removed navigation callbacks - using direct function calls
static bool s_reset_in_progress = false;


// Global current view containers for this module

// Guard to prevent recursive calls

// Available view types in order (first is default)
static const power_monitor_view_type_t available_views[] = {
	POWER_MONITOR_VIEW_BAR_GRAPH,    // Power grid view (value 3)
	POWER_MONITOR_VIEW_NUMERICAL     // Starter Voltage view (value 4)
};

// Union to support both modal types
typedef union {
	alerts_modal_t* alerts;
	timeline_modal_t* timeline;
} modal_union_t;

static modal_union_t current_modal = {.alerts = NULL};
static bool current_modal_is_timeline = false;

// Helper function to get current view type from global state
static power_monitor_view_type_t get_current_view_type(void)
{
	int view_index = power_monitor_get_view_index();

	if (view_index < 0 || view_index >= 2) {
		printf("[E] power_monitor: Invalid view index: %d (total: 2)\n", view_index);
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

	// Get current index from device state
	int current_index = power_monitor_get_view_index();
	printf("[I] power_monitor: Before cycle: index=%d, total_views=2\n", current_index);

	// Simple wrap-around logic
	int next_index = (current_index + 1) % 2;

	// Update device state directly
	power_monitor_set_view_index(next_index);
	printf("[I] power_monitor: View cycle complete - updated from index %d to %d\n", current_index, next_index);
}

// Callback to request home screen after LVGL cleanup
static void power_monitor_navigation_hide_detail_screen(void)
{
	printf("[I] power_monitor: About to DESTROY detail screen completely\n");
	printf("[I] power_monitor: Detail screen pointer: %p\n", detail_screen);

	if (detail_screen) {

		printf("[I] power_monitor: Calling detail_screen_destroy\n");
		detail_screen_destroy(detail_screen);
		detail_screen = NULL; // Clear the pointer after destruction
		printf("[I] power_monitor: detail_screen_destroy completed\n");
	} else {

		printf("[W] power_monitor: Detail screen is NULL\n");
	}

	printf("[I] power_monitor: Detail screen completely destroyed\n");

	// Request transition back to home screen
	printf("[I] power_monitor: Requesting transition back to home screen\n");
	power_monitor_navigation_request_home_screen();
}

static void power_monitor_navigation_request_home_screen(void)
{
	printf("[I] power_monitor: About to request home screen transition\n");
	extern void screen_navigation_request_home_screen(void);
	printf("[I] power_monitor: Calling screen_navigation_request_home_screen\n");
	screen_navigation_request_home_screen();
	printf("[I] power_monitor: Home screen transition requested\n");
	printf("[I] power_monitor: power_monitor_navigation_request_home_screen completed\n");
}


// Simple container management
static lv_obj_t* power_monitor_container = NULL;

// Sensor data labels moved to detail_screen.c - power monitor only provides data

// Initialize power-monitor's own container (simplified)
static void power_monitor_init_widget(void)
{
	if (power_monitor_container) {
		printf("[I] power_monitor: Container already initialized\n");
		return;
	}

	// Create power-monitor's own container on the main screen
	power_monitor_container = lv_obj_create(lv_scr_act());
	lv_obj_set_size(power_monitor_container, 320, 240);
	lv_obj_align(power_monitor_container, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_bg_opa(power_monitor_container, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(power_monitor_container, 0, 0);
	lv_obj_clear_flag(power_monitor_container, LV_OBJ_FLAG_SCROLLABLE);

	printf("[I] power_monitor: Power monitor container created successfully\n");
}


// Removed unused power_monitor_show_for_detail_screen function



// Create current view using the shared template system
void power_monitor_create_current_view_content(lv_obj_t* container)
{

	if (!container) {

		printf("[E] power_monitor: Container is NULL\n");
		return;
	}

	// Safety check: validate container
	if (!lv_obj_is_valid(container)) {

		printf("[E] power_monitor: Container is not valid\n");
		return;
	}

	// Clear container first
	lv_obj_clean(container);

	// Re-apply container styling after clean (clean removes styling)
	// Use detail screen's reusable function for consistent styling
	extern detail_screen_t* detail_screen;
	if (detail_screen && container == detail_screen->current_view_container) {
		detail_screen_restore_current_view_styling(container);
	}

	// Get current view type and render appropriate view directly in container
	int view_index = power_monitor_get_view_index();

	// Map index to view type: 0=BAR_GRAPH, 1=NUMERICAL
	if (view_index == 0) {

		power_monitor_power_grid_view_render(container);
	} else if (view_index == 1) {

		power_monitor_starter_voltage_view_render(container);
	} else {

		power_monitor_power_grid_view_render(container);
	}


	// Mark view cycling as complete if this was called during cycling
	if( current_view_manager_is_cycling_in_progress() ){

		current_view_manager_set_cycling_in_progress(false);
	}
}

// Simple view cycling - just calls the main cycling function
static void power_monitor_cycle_view(void)
{
	printf("[I] power_monitor: === CYCLING CURRENT VIEW ===\n");
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

// Gauge mapping structure for consolidated update functions
typedef struct {
	power_monitor_gauge_type_t gauge_type;
	bar_graph_gauge_t* detail_gauge;
	const char* gauge_name;
} gauge_map_entry_t;

// Map gauge types to their detail gauges and names
static const gauge_map_entry_t gauge_map[] = {
	{POWER_MONITOR_GAUGE_STARTER_VOLTAGE, &detail_starter_voltage_gauge, "starter_voltage"},
	{POWER_MONITOR_GAUGE_STARTER_CURRENT, &detail_starter_current_gauge, "starter_current"},
	{POWER_MONITOR_GAUGE_HOUSE_VOLTAGE, &detail_house_voltage_gauge, "house_voltage"},
	{POWER_MONITOR_GAUGE_HOUSE_CURRENT, &detail_house_current_gauge, "house_current"},
	{POWER_MONITOR_GAUGE_SOLAR_VOLTAGE, &detail_solar_voltage_gauge, "solar_voltage"},
	{POWER_MONITOR_GAUGE_SOLAR_CURRENT, &detail_solar_current_gauge, "solar_current"},
};

// Helper function to find gauge map entry
static const gauge_map_entry_t* find_gauge_map_entry(power_monitor_gauge_type_t gauge_type) {
	for (size_t i = 0; i < sizeof(gauge_map) / sizeof(gauge_map[0]); i++) {
		if (gauge_map[i].gauge_type == gauge_type) {
			return &gauge_map[i];
		}
	}
	return NULL;
}

// Create 6 bar graph gauges in the gauges container (matching original detail.c)
static void power_monitor_create_detail_gauges(lv_obj_t* container)
{

	if (!container) {
		printf("[E] power_monitor: Gauges container is NULL\n");
		return;
	}

	// Force layout update to get correct dimensions
	lv_obj_update_layout(container);

	// Get container dimensions after layout update
	lv_coord_t container_width = lv_obj_get_width(container);
	lv_coord_t container_height = lv_obj_get_height(container);

	// Check for valid dimensions
	if (container_width <= 0 || container_height <= 0) {

		printf("[E] power_monitor: Invalid container dimensions: %dx%d\n", container_width, container_height);
		return;
	}

	// Calculate gauge dimensions for vertical stack with 6px padding between gauges (2px + 4px from gauge height reduction)
	int gauge_padding = 12; // 6px padding between gauges (2px original + 4px from gauge height reduction)
	int total_padding = gauge_padding * 5; // 5 gaps between 6 gauges
	lv_coord_t gauge_width = container_width;  // Full width, no padding
	lv_coord_t gauge_height = (container_height - 2 - total_padding) / 6; // 6 rows with padding and 2px offset for the title bleed

	// Configure container for flexbox layout (vertical stack with fixed heights)
	lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_row(container, gauge_padding, 0); // Vertical gap between gauges

	// Get gauge initial configuration values from device state
	// Voltage gauges
	float starter_baseline = device_state_get_float("power_monitor.starter_baseline_voltage_v");
	float starter_min = device_state_get_float("power_monitor.starter_min_voltage_v");
	float starter_max = device_state_get_float("power_monitor.starter_max_voltage_v");

	float house_baseline = device_state_get_float("power_monitor.house_baseline_voltage_v");
	float house_min = device_state_get_float("power_monitor.house_min_voltage_v");
	float house_max = device_state_get_float("power_monitor.house_max_voltage_v");

	float solar_min = device_state_get_float("power_monitor.solar_min_voltage_v");
	float solar_max = device_state_get_float("power_monitor.solar_max_voltage_v");

	// Current gauges
	float starter_current_baseline = device_state_get_float("power_monitor.starter_baseline_current_a");
	float starter_current_min = device_state_get_float("power_monitor.starter_min_current_a");
	float starter_current_max = device_state_get_float("power_monitor.starter_max_current_a");
	float house_current_baseline = device_state_get_float("power_monitor.house_baseline_current_a");
	float house_current_min = device_state_get_float("power_monitor.house_min_current_a");
	float house_current_max = device_state_get_float("power_monitor.house_max_current_a");
	float solar_current_baseline = device_state_get_float("power_monitor.solar_baseline_current_a");
	float solar_current_min = device_state_get_float("power_monitor.solar_min_current_a");
	float solar_current_max = device_state_get_float("power_monitor.solar_max_current_a");


	// Row 1: Starter Battery Voltage
	bar_graph_gauge_init(&detail_starter_voltage_gauge, container, 0, 0, gauge_width, gauge_height, 3, 1);
	// Gauge uses fixed height calculated above (1/6 of available space

	bar_graph_gauge_configure_advanced(
		&detail_starter_voltage_gauge, // gauge pointer
		BAR_GRAPH_MODE_BIPOLAR, // graph mode
		starter_baseline, starter_min, starter_max, // bounds: baseline, min, max
		"STARTER BATTERY", "V", "V", PALETTE_WARM_WHITE, // title, unit, y-axis unit, color
		true, true, true // Show title, Show Y-axis, Show Border
	);
	bar_graph_gauge_set_update_interval(&detail_starter_voltage_gauge, 0); // No rate limiting - gauge calculates its own rate

	// Row 2: Starter Battery Current
	bar_graph_gauge_init(&detail_starter_current_gauge, container, 0, 0, gauge_width, gauge_height, 3, 1);
	bar_graph_gauge_configure_advanced(
		&detail_starter_current_gauge, // gauge pointer
		BAR_GRAPH_MODE_BIPOLAR, // graph mode
		starter_current_baseline, starter_current_min, starter_current_max, // bounds: baseline, min, max
		"STARTER CURRENT", "A", "A", PALETTE_WARM_WHITE, // title, unit, y-axis unit, color
		true, true, true // Show title, Show Y-axis, Show Border
	);
	bar_graph_gauge_set_update_interval(&detail_starter_current_gauge, 0); // No rate limiting - gauge calculates its own rate

	// Row 3: House Battery Voltage
	bar_graph_gauge_init(&detail_house_voltage_gauge, container, 0, 0, gauge_width, gauge_height, 3, 1);
	bar_graph_gauge_configure_advanced(
		&detail_house_voltage_gauge, // gauge pointer
		BAR_GRAPH_MODE_BIPOLAR, // graph mode
		house_baseline, house_min, house_max, // bounds: baseline, min, max
		"HOUSE BATTERY", "V", "V", PALETTE_WARM_WHITE, // title, unit, y-axis unit, color
		true, true, true // Show title, Show Y-axis, Show Border
	);
	bar_graph_gauge_set_update_interval(&detail_house_voltage_gauge, 0); // No rate limiting - gauge calculates its own rate

	// Row 4: House Battery Current
	bar_graph_gauge_init(&detail_house_current_gauge, container, 0, 0, gauge_width, gauge_height, 3, 1);
	bar_graph_gauge_configure_advanced(
		&detail_house_current_gauge, // gauge pointer
		BAR_GRAPH_MODE_BIPOLAR, // graph mode
		house_current_baseline, house_current_min, house_current_max, // bounds: baseline, min, max
		"HOUSE CURRENT", "A", "A", PALETTE_WARM_WHITE, // title, unit, y-axis unit, color
		true, true, true // Show title, Show Y-axis, Show Border
	);
	bar_graph_gauge_set_update_interval(&detail_house_current_gauge, 0); // No rate limiting - gauge calculates its own rate

	// Row 5: Solar Input Voltage
	bar_graph_gauge_init(&detail_solar_voltage_gauge, container, 0, 0, gauge_width, gauge_height, 3, 1);
	bar_graph_gauge_configure_advanced(
		&detail_solar_voltage_gauge, // gauge pointer
		BAR_GRAPH_MODE_POSITIVE_ONLY, // graph mode
		0.0f, 0.0f, 25.0f, // bounds: baseline, min, max
		"SOLAR VOLTS", "V", "V", PALETTE_WARM_WHITE, // title, unit, y-axis unit, color
		true, true, true // Show title, Show Y-axis, Show Border
	);
	bar_graph_gauge_set_update_interval(&detail_solar_voltage_gauge, 0); // No rate limiting - gauge calculates its own rate

	// Row 6: Solar Input Current
	bar_graph_gauge_init(&detail_solar_current_gauge, container, 0, 0, gauge_width, gauge_height, 3, 1);
	bar_graph_gauge_configure_advanced(
		&detail_solar_current_gauge, // gauge pointer
		BAR_GRAPH_MODE_BIPOLAR, // graph mode
		solar_current_baseline, solar_current_min, solar_current_max, // bounds: baseline, min, max
		"SOLAR CURRENT", "A", "A", PALETTE_WARM_WHITE, // title, unit, y-axis unit, color
		true, true, true // Show title, Show Y-axis, Show Border
	);
	bar_graph_gauge_set_update_interval(&detail_solar_current_gauge, 0); // No rate limiting - gauge calculates its own rate

	// Update labels and ticks for all gauges
	bar_graph_gauge_update_labels_and_ticks(&detail_starter_voltage_gauge);
	bar_graph_gauge_update_labels_and_ticks(&detail_starter_current_gauge);
	bar_graph_gauge_update_labels_and_ticks(&detail_house_voltage_gauge);
	bar_graph_gauge_update_labels_and_ticks(&detail_house_current_gauge);
	bar_graph_gauge_update_labels_and_ticks(&detail_solar_voltage_gauge);
	bar_graph_gauge_update_labels_and_ticks(&detail_solar_current_gauge);

	// Set timeline durations for all detail gauges
	power_monitor_update_gauge_timeline(POWER_MONITOR_GAUGE_STARTER_VOLTAGE, true);
	power_monitor_update_gauge_timeline(POWER_MONITOR_GAUGE_STARTER_CURRENT, true);
	power_monitor_update_gauge_timeline(POWER_MONITOR_GAUGE_HOUSE_VOLTAGE, true);
	power_monitor_update_gauge_timeline(POWER_MONITOR_GAUGE_HOUSE_CURRENT, true);
	power_monitor_update_gauge_timeline(POWER_MONITOR_GAUGE_SOLAR_VOLTAGE, true);
	power_monitor_update_gauge_timeline(POWER_MONITOR_GAUGE_SOLAR_CURRENT, true);
}


// Apply alert flashing for current view values
static void power_monitor_apply_current_view_alert_flashing(void)
{
	// Get current data
	power_monitor_data_t *data = power_monitor_get_data();
	if (!data) {
		return;
	}

	// Get alert thresholds
	int starter_lo = device_state_get_int("power_monitor.starter_alert_low_voltage_v");
	int starter_hi = device_state_get_int("power_monitor.starter_alert_high_voltage_v");
	int house_lo = device_state_get_int("power_monitor.house_alert_low_voltage_v");
	int house_hi = device_state_get_int("power_monitor.house_alert_high_voltage_v");
	int solar_lo = device_state_get_int("power_monitor.solar_alert_low_voltage_v");
	int solar_hi = device_state_get_int("power_monitor.solar_alert_high_voltage_v");

	// Blink timing - asymmetric: 1 second on, 0.5 seconds off (1.5 second total cycle)
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	int tick_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	bool blink_on = (tick_ms % 1500) < 1000;

	// Apply alert flashing to the currently active view (works for both home and detail screen)
	power_monitor_view_type_t current_view = get_current_view_type();
	if (current_view == POWER_MONITOR_VIEW_BAR_GRAPH) {
		power_monitor_power_grid_view_apply_alert_flashing(data, starter_lo, starter_hi, house_lo, house_hi, solar_lo, solar_hi, blink_on);
	} else if (current_view == POWER_MONITOR_VIEW_NUMERICAL) {
		power_monitor_starter_voltage_view_apply_alert_flashing(data, starter_lo, starter_hi, house_lo, house_hi, solar_lo, solar_hi, blink_on);
	} else {
		printf("[W] power_monitor: Unknown view type: %d, skipping alert flashing\n", current_view);
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

	if (detail_screen->sensor_data_section) {
		power_monitor_update_sensor_labels_in_detail_screen(detail_screen->sensor_data_section, &lerp_data);
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

// Public function to update detail screen gauge ranges
void power_monitor_update_detail_gauge_ranges(void)
{
	// Only update if detail screen exists
	if (!detail_screen) {
		return;
	}

	// Get current gauge range values from device state
	// Voltage gauges
	float starter_baseline = device_state_get_float("power_monitor.starter_baseline_voltage_v");
	float starter_min = device_state_get_float("power_monitor.starter_min_voltage_v");
	float starter_max = device_state_get_float("power_monitor.starter_max_voltage_v");
	float house_baseline = device_state_get_float("power_monitor.house_baseline_voltage_v");
	float house_min = device_state_get_float("power_monitor.house_min_voltage_v");
	float house_max = device_state_get_float("power_monitor.house_max_voltage_v");
	float solar_min = device_state_get_float("power_monitor.solar_min_voltage_v");
	float solar_max = device_state_get_float("power_monitor.solar_max_voltage_v");

	// Current gauges
	float starter_current_baseline = device_state_get_float("power_monitor.starter_baseline_current_a");
	float starter_current_min = device_state_get_float("power_monitor.starter_min_current_a");
	float starter_current_max = device_state_get_float("power_monitor.starter_max_current_a");
	float house_current_baseline = device_state_get_float("power_monitor.house_baseline_current_a");
	float house_current_min = device_state_get_float("power_monitor.house_min_current_a");
	float house_current_max = device_state_get_float("power_monitor.house_max_current_a");
	float solar_current_baseline = device_state_get_float("power_monitor.solar_baseline_current_a");
	float solar_current_min = device_state_get_float("power_monitor.solar_min_current_a");
	float solar_current_max = device_state_get_float("power_monitor.solar_max_current_a");

	// Update detail screen gauge ranges
	if (detail_starter_voltage_gauge.initialized) {
		bar_graph_gauge_configure_advanced(
			&detail_starter_voltage_gauge,
			BAR_GRAPH_MODE_BIPOLAR,
			starter_baseline,
			starter_min,
			starter_max,
			"STARTER BATTERY", "V", "V", PALETTE_WARM_WHITE,
			true, true, true
		);
	}

	if (detail_house_voltage_gauge.initialized) {
		bar_graph_gauge_configure_advanced(
			&detail_house_voltage_gauge,
			BAR_GRAPH_MODE_BIPOLAR,
			house_baseline,
			house_min,
			house_max,
			"HOUSE BATTERY", "V", "V", PALETTE_WARM_WHITE,
			true, true, true
		);
	}

	if (detail_solar_voltage_gauge.initialized) {
		bar_graph_gauge_configure_advanced(
			&detail_solar_voltage_gauge,
			BAR_GRAPH_MODE_POSITIVE_ONLY,
			0.0f,
			solar_min,
			solar_max,
			"SOLAR INPUT", "V", "V", PALETTE_WARM_WHITE,
			true, true, true
		);
	}

	// Update current gauge ranges
	if (detail_starter_current_gauge.initialized) {
		bar_graph_gauge_configure_advanced(
			&detail_starter_current_gauge,
			BAR_GRAPH_MODE_BIPOLAR,
			starter_current_baseline,
			starter_current_min,
			starter_current_max,
			"STARTER CURRENT", "A", "A", PALETTE_WARM_WHITE,
			true, true, true
		);
	}

	if (detail_house_current_gauge.initialized) {
		bar_graph_gauge_configure_advanced(
			&detail_house_current_gauge,
			BAR_GRAPH_MODE_BIPOLAR,
			house_current_baseline,
			house_current_min,
			house_current_max,
			"HOUSE CURRENT", "A", "A", PALETTE_WARM_WHITE,
			true, true, true
		);
	}

	if (detail_solar_current_gauge.initialized) {
		bar_graph_gauge_configure_advanced(
			&detail_solar_current_gauge,
			BAR_GRAPH_MODE_BIPOLAR,
			solar_current_baseline,
			solar_current_min,
			solar_current_max,
			"SOLAR CURRENT", "A", "A", PALETTE_WARM_WHITE,
			true, true, true
		);
	}
}


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

	// Current view alert flashing (still handled by power monitor)
	power_monitor_apply_current_view_alert_flashing();
}




// Public function for initializing with default view (required by main.c)
void power_monitor_init_with_default_view(power_monitor_view_type_t default_view)
{
	power_monitor_init();
	power_monitor_set_current_view_type((power_monitor_view_type_t)default_view);
}

// Default setting mapping structure
typedef struct {
	const char* path;
	double value;
} power_monitor_default_t;


// Initialize power monitor defaults in device state only if values don't exist
static void power_monitor_init_defaults(void)
{
	// Map all default settings
	power_monitor_default_t defaults[] = {
		// Gauge timeline settings - named properties for each gauge
		{"power_monitor.gauge_timeline_settings.starter_voltage.current_view", POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS},
		{"power_monitor.gauge_timeline_settings.starter_voltage.detail_view", POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS},

		{"power_monitor.gauge_timeline_settings.starter_current.current_view", POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS},
		{"power_monitor.gauge_timeline_settings.starter_current.detail_view", POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS},

		{"power_monitor.gauge_timeline_settings.house_voltage.current_view", POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS},
		{"power_monitor.gauge_timeline_settings.house_voltage.detail_view", POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS},

		{"power_monitor.gauge_timeline_settings.house_current.current_view", POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS},
		{"power_monitor.gauge_timeline_settings.house_current.detail_view", POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS},

		{"power_monitor.gauge_timeline_settings.solar_voltage.current_view", POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS},
		{"power_monitor.gauge_timeline_settings.solar_voltage.detail_view", POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS},

		{"power_monitor.gauge_timeline_settings.solar_current.current_view", POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS},
		{"power_monitor.gauge_timeline_settings.solar_current.detail_view", POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS},

		// Starter battery settings
		{"power_monitor.starter_alert_low_voltage_v", (double)POWER_MONITOR_DEFAULT_STARTER_ALERT_LOW_VOLTAGE_V},
		{"power_monitor.starter_alert_high_voltage_v", (double)POWER_MONITOR_DEFAULT_STARTER_ALERT_HIGH_VOLTAGE_V},
		{"power_monitor.starter_baseline_voltage_v", (double)POWER_MONITOR_DEFAULT_STARTER_BASELINE_VOLTAGE_V},
		{"power_monitor.starter_min_voltage_v", (double)POWER_MONITOR_DEFAULT_STARTER_MIN_VOLTAGE_V},
		{"power_monitor.starter_max_voltage_v", (double)POWER_MONITOR_DEFAULT_STARTER_MAX_VOLTAGE_V},

		// Starter current settings
		{"power_monitor.starter_alert_low_current_a", (double)-30.0f},
		{"power_monitor.starter_alert_high_current_a", (double)30.0f},
		{"power_monitor.starter_baseline_current_a", (double)0.0f},
		{"power_monitor.starter_min_current_a", (double)-40.0f},
		{"power_monitor.starter_max_current_a", (double)40.0f},

		// House battery settings
		{"power_monitor.house_alert_low_voltage_v", (double)POWER_MONITOR_DEFAULT_HOUSE_ALERT_LOW_VOLTAGE_V},
		{"power_monitor.house_alert_high_voltage_v", (double)POWER_MONITOR_DEFAULT_HOUSE_ALERT_HIGH_VOLTAGE_V},
		{"power_monitor.house_baseline_voltage_v", (double)POWER_MONITOR_DEFAULT_HOUSE_BASELINE_VOLTAGE_V},
		{"power_monitor.house_min_voltage_v", (double)POWER_MONITOR_DEFAULT_HOUSE_MIN_VOLTAGE_V},
		{"power_monitor.house_max_voltage_v", (double)POWER_MONITOR_DEFAULT_HOUSE_MAX_VOLTAGE_V},

		// House current settings
		{"power_monitor.house_alert_low_current_a", (double)-30.0f},
		{"power_monitor.house_alert_high_current_a", (double)30.0f},
		{"power_monitor.house_baseline_current_a", (double)0.0f},
		{"power_monitor.house_min_current_a", (double)-40.0f},
		{"power_monitor.house_max_current_a", (double)40.0f},

		// Solar settings
		{"power_monitor.solar_alert_low_voltage_v", (double)POWER_MONITOR_DEFAULT_SOLAR_ALERT_LOW_VOLTAGE_V},
		{"power_monitor.solar_alert_high_voltage_v", (double)POWER_MONITOR_DEFAULT_SOLAR_ALERT_HIGH_VOLTAGE_V},
		{"power_monitor.solar_min_voltage_v", (double)POWER_MONITOR_DEFAULT_SOLAR_MIN_VOLTAGE_V},
		{"power_monitor.solar_max_voltage_v", (double)POWER_MONITOR_DEFAULT_SOLAR_MAX_VOLTAGE_V},

		// Solar current settings
		{"power_monitor.solar_alert_low_current_a", (double)-30.0f},
		{"power_monitor.solar_alert_high_current_a", (double)30.0f},
		{"power_monitor.solar_baseline_current_a", (double)0.0f},
		{"power_monitor.solar_min_current_a", (double)-40.0f},
		{"power_monitor.solar_max_current_a", (double)40.0f},
	};

	// Process all defaults
	for (size_t i = 0; i < sizeof(defaults) / sizeof(defaults[0]); i++) {

		if (!device_state_path_exists(defaults[i].path)) {

			device_state_set_value(defaults[i].path, defaults[i].value);
		}
	}
}

// Module interface implementation
void power_monitor_init(void)
{

	// Initialize power monitor defaults
	power_monitor_init_defaults();



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

	if (!container) {
		printf("[E] power_monitor: Container is NULL\n");
		return;
	}

	// Create current view content directly in the container
	power_monitor_create_current_view_content(container);
}

void power_monitor_show_in_container_home(lv_obj_t *container)
{

	// Render the current view directly into the container
	power_monitor_render_current_view(container);

	// Add touch callback for home screen navigation
	lv_obj_add_event_cb(container, power_monitor_home_current_view_touch_cb, LV_EVENT_CLICKED, NULL);
}

// void power_monitor_show_in_container_detail(lv_obj_t *container)
// {

// 	// Render the current view directly into the container
// 	power_monitor_render_current_view(container);
// }

void power_monitor_cycle_current_view(void)
{

	// Simple guard against rapid cycling
	if (s_detail_view_needs_refresh) {

		return;
	}

	power_monitor_navigation_cycle_to_next_view();

	// Mark that we need to re-render the detail view on next update
	// This avoids immediate re-rendering which can cause crashes
	extern screen_type_t screen_navigation_get_current_screen(void);
	screen_type_t current_screen = screen_navigation_get_current_screen();

	if( current_screen == SCREEN_DETAIL_VIEW ){

		s_detail_view_needs_refresh = true;
	}
}

void power_monitor_cleanup_internal(void)
{

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
	printf("[I] power_monitor: Detail gauge variables reset\n");
}

// Consolidated gauge update function - replaces both specific and detail functions
void power_monitor_update_gauge_timeline(power_monitor_gauge_type_t gauge_type, bool is_detail_view)
{
	const gauge_map_entry_t* entry = find_gauge_map_entry(gauge_type);
	if (!entry || !entry->detail_gauge->initialized) {
		return;
	}

	// Build the path based on view type
	char timeline_path[256];
	const char* view_type = is_detail_view ? "detail_view" : "current_view";
	snprintf(timeline_path, sizeof(timeline_path), "power_monitor.gauge_timeline_settings.%s.%s", entry->gauge_name, view_type);

	int timeline_duration = device_state_get_int(timeline_path);
	uint32_t timeline_duration_ms = timeline_duration * 1000;

	// Update the gauge timeline duration
	bar_graph_gauge_set_timeline_duration(entry->detail_gauge, timeline_duration_ms);
}


// Update all bar graph gauge intervals based on their individual timeline settings
void power_monitor_update_gauge_intervals(void)
{
	// Update detail screen gauges with their own timeline settings
	power_monitor_update_gauge_timeline(POWER_MONITOR_GAUGE_STARTER_VOLTAGE, false);
	power_monitor_update_gauge_timeline(POWER_MONITOR_GAUGE_STARTER_CURRENT, false);
	power_monitor_update_gauge_timeline(POWER_MONITOR_GAUGE_HOUSE_VOLTAGE, false);
	power_monitor_update_gauge_timeline(POWER_MONITOR_GAUGE_HOUSE_CURRENT, false);
	power_monitor_update_gauge_timeline(POWER_MONITOR_GAUGE_SOLAR_VOLTAGE, false);
	power_monitor_update_gauge_timeline(POWER_MONITOR_GAUGE_SOLAR_CURRENT, false);

	// Update power grid view gauges with their own timeline settings
	extern void power_grid_view_update_gauge_intervals(void);
	power_grid_view_update_gauge_intervals();
}

void power_monitor_cleanup(void)
{

	power_monitor_cleanup_internal();
}

// Button configurations for power monitor
static detail_button_config_t power_monitor_buttons[] = {
	{"ALERTS", power_monitor_handle_alerts_button},
	{"TIMELINE", power_monitor_handle_timeline_button}
};

// Callback functions for detail screen
static void power_monitor_on_current_view_created(lv_obj_t* container)
{
	printf("[I] power_monitor: Current view container created callback\n");
	power_monitor_create_current_view_content(container);
}

static void power_monitor_on_gauges_created(lv_obj_t* container)
{
	printf("[I] power_monitor: Gauges container created callback\n");
	power_monitor_create_detail_gauges(container);
}

static void power_monitor_on_sensor_data_created(lv_obj_t* container)
{
	printf("[I] power_monitor: Sensor data container created callback\n");
	power_monitor_create_sensor_labels_in_detail_screen(container);
}

static void power_monitor_on_view_clicked(void)
{
	printf("[I] power_monitor: *** VIEW CLICKED CALLBACK CALLED ***\n");
	printf("[I] power_monitor: View clicked callback - cycling current view\n");
	power_monitor_cycle_current_view();
	printf("[I] power_monitor: View cycling call completed\n");
}

// Power monitor specific sensor label creation function
void power_monitor_create_sensor_labels_in_detail_screen(lv_obj_t* container)
{
	if (!container) {
		printf("[E] power_monitor: Container is NULL for sensor labels\n");
		return;
	}

	printf("[I] power_monitor: Creating sensor data labels in detail screen\n");

	// Create sensor data labels with horizontal layout (label: value on same line)
	lv_color_t labelColor = PALETTE_GRAY;
	lv_color_t valueColor = PALETTE_GREEN;
	lv_color_t groupColor = PALETTE_WHITE;

	// Sensor data layout: Group headers + horizontal label:value pairs
	const char* group_names[] = {"Starter Battery", "House Battery", "Solar Input"};
	const char* value_labels[] = {"Volts:", "Amperes:"};

	// Create 3 groups (Starter, House, Solar)
	for (int group = 0; group < 3; group++) {

		// Group header
		lv_obj_t* group_label = lv_label_create(container);
		lv_obj_set_style_text_font(group_label, &lv_font_montserrat_16, 0);
		lv_obj_set_style_text_color(group_label, groupColor, 0);
		lv_label_set_text(group_label, group_names[group]);
		lv_obj_set_style_pad_top(group_label, group == 0 ? 5 : 10, 0); // Extra spacing between groups

		// Create 2 value pairs (Volts, Amperes) for each group
		for (int value_type = 0; value_type < 2; value_type++) {

			// Create horizontal container for label:value pair
			lv_obj_t* value_row = lv_obj_create(container);
			lv_obj_set_size(value_row, LV_PCT(100), LV_SIZE_CONTENT);
			lv_obj_set_style_bg_color(value_row, PALETTE_BLACK, 0);
			lv_obj_set_style_bg_opa(value_row, LV_OPA_COVER, 0);
			lv_obj_set_style_border_width(value_row, 0, 0);
			lv_obj_set_style_pad_all(value_row, 2, 0);
			lv_obj_clear_flag(value_row, LV_OBJ_FLAG_SCROLLABLE);

			// Set up horizontal flexbox for label:value layout
			lv_obj_set_flex_flow(value_row, LV_FLEX_FLOW_ROW);
			lv_obj_set_flex_align(value_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

			// Label (left side)
			lv_obj_t* label = lv_label_create(value_row);
			lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
			lv_obj_set_style_text_color(label, labelColor, 0);
			lv_label_set_text(label, value_labels[value_type]);

			// Value (right side) - will be updated by power_monitor_update_sensor_labels_in_detail_screen
			lv_obj_t* value = lv_label_create(value_row);
			lv_obj_set_style_text_font(value, &lv_font_noplato_24, 0);
			lv_obj_set_style_text_color(value, valueColor, 0);
			lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);
			lv_label_set_text(value, "0.0");

			// Store reference for direct updates to data
			if (group == 0) { // Starter Battery

				if (value_type == 0) power_data.sensor_labels.starter_voltage = value;
				else power_data.sensor_labels.starter_current = value;
			} else if (group == 1) { // House Battery

				if (value_type == 0) power_data.sensor_labels.house_voltage = value;
				else power_data.sensor_labels.house_current = value;
			} else if (group == 2) { // Solar Input

				if (value_type == 0) power_data.sensor_labels.solar_voltage = value;
				else power_data.sensor_labels.solar_current = value;
			}
		}
	}

	printf("[I] power_monitor: Sensor data labels created successfully\n");
}

// Power monitor specific sensor label update function
void power_monitor_update_sensor_labels_in_detail_screen(lv_obj_t* sensor_section, const lerp_power_monitor_data_t* lerp_data)
{
	if (!lerp_data) {
		return;
	}

	// Get alert thresholds for voltage values
	int starter_lo = device_state_get_int("power_monitor.starter_alert_low_voltage_v");
	int starter_hi = device_state_get_int("power_monitor.starter_alert_high_voltage_v");
	int house_lo = device_state_get_int("power_monitor.house_alert_low_voltage_v");
	int house_hi = device_state_get_int("power_monitor.house_alert_high_voltage_v");
	int solar_lo = device_state_get_int("power_monitor.solar_alert_low_voltage_v");
	int solar_hi = device_state_get_int("power_monitor.solar_alert_high_voltage_v");

	// Get alert thresholds for current values
	int starter_current_lo = device_state_get_int("power_monitor.starter_alert_low_current_a");
	int starter_current_hi = device_state_get_int("power_monitor.starter_alert_high_current_a");
	int house_current_lo = device_state_get_int("power_monitor.house_alert_low_current_a");
	int house_current_hi = device_state_get_int("power_monitor.house_alert_high_current_a");
	int solar_current_lo = device_state_get_int("power_monitor.solar_alert_low_current_a");
	int solar_current_hi = device_state_get_int("power_monitor.solar_alert_high_current_a");

	// Get raw values for threshold checking
	float starter_v_raw = lerp_value_get_raw(&lerp_data->starter_voltage);
	float house_v_raw = lerp_value_get_raw(&lerp_data->house_voltage);
	float solar_v_raw = lerp_value_get_raw(&lerp_data->solar_voltage);
	float starter_c_raw = lerp_value_get_raw(&lerp_data->starter_current);
	float house_c_raw = lerp_value_get_raw(&lerp_data->house_current);
	float solar_c_raw = lerp_value_get_raw(&lerp_data->solar_current);

	// Check if values are outside alert thresholds
	bool starter_alert = (starter_v_raw <= (float)starter_lo) || (starter_v_raw >= (float)starter_hi);
	bool house_alert = (house_v_raw <= (float)house_lo) || (house_v_raw >= (float)house_hi);
	bool solar_alert = (solar_v_raw <= (float)solar_lo) || (solar_v_raw >= (float)solar_hi);
	bool starter_current_alert = (starter_c_raw <= (float)starter_current_lo) || (starter_c_raw >= (float)starter_current_hi);
	bool house_current_alert = (house_c_raw <= (float)house_current_lo) || (house_c_raw >= (float)house_current_hi);
	bool solar_current_alert = (solar_c_raw <= (float)solar_current_lo) || (solar_c_raw >= (float)solar_current_hi);

	// Update values using direct references - much cleaner!
	char sensor_text[32];

	// Update starter battery values
	snprintf(sensor_text, sizeof(sensor_text), "%.1f", lerp_value_get_display(&lerp_data->starter_voltage));
	lv_label_set_text(power_data.sensor_labels.starter_voltage, sensor_text);
	lv_obj_set_style_text_color(power_data.sensor_labels.starter_voltage,
		starter_alert ? PALETTE_YELLOW : PALETTE_WHITE, 0);

	snprintf(sensor_text, sizeof(sensor_text), "%.1f", lerp_value_get_display(&lerp_data->starter_current));
	lv_label_set_text(power_data.sensor_labels.starter_current, sensor_text);
	lv_obj_set_style_text_color(power_data.sensor_labels.starter_current,
		starter_current_alert ? PALETTE_YELLOW : PALETTE_WHITE, 0);

	// Update house battery values
	snprintf(sensor_text, sizeof(sensor_text), "%.1f", lerp_value_get_display(&lerp_data->house_voltage));
	lv_label_set_text(power_data.sensor_labels.house_voltage, sensor_text);
	lv_obj_set_style_text_color(power_data.sensor_labels.house_voltage,
		house_alert ? PALETTE_YELLOW : PALETTE_WHITE, 0);

	snprintf(sensor_text, sizeof(sensor_text), "%.1f", lerp_value_get_display(&lerp_data->house_current));
	lv_label_set_text(power_data.sensor_labels.house_current, sensor_text);
	lv_obj_set_style_text_color(power_data.sensor_labels.house_current,
		house_current_alert ? PALETTE_YELLOW : PALETTE_WHITE, 0);

	// Update solar input values
	snprintf(sensor_text, sizeof(sensor_text), "%.1f", lerp_value_get_display(&lerp_data->solar_voltage));
	lv_label_set_text(power_data.sensor_labels.solar_voltage, sensor_text);
	lv_obj_set_style_text_color(power_data.sensor_labels.solar_voltage,
		solar_alert ? PALETTE_YELLOW : PALETTE_WHITE, 0);

	snprintf(sensor_text, sizeof(sensor_text), "%.1f", lerp_value_get_display(&lerp_data->solar_current));
	lv_label_set_text(power_data.sensor_labels.solar_current, sensor_text);
	lv_obj_set_style_text_color(power_data.sensor_labels.solar_current,
		solar_current_alert ? PALETTE_YELLOW : PALETTE_WHITE, 0);
}

// Detail screen management
void power_monitor_create_detail_screen(void)
{
	printf("[I] power_monitor: === CREATING DETAIL SCREEN (V2) ===\n");

	if (detail_screen) {
		printf("[W] power_monitor: Detail screen already exists\n");
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
		.on_view_clicked = power_monitor_on_view_clicked,
		.on_current_view_created = power_monitor_on_current_view_created,
		.on_gauges_created = power_monitor_on_gauges_created,
		.on_sensor_data_created = power_monitor_on_sensor_data_created,
		.overlay_creator = NULL,
		.overlay_user_data = NULL
	};

	detail_screen = detail_screen_create(&config);
	if (detail_screen) {
		printf("[I] power_monitor: Detail screen created successfully\n");
		// Content creation is now handled by callbacks
	} else {
		printf("[E] power_monitor: Failed to create detail screen\n");
	}
}

void power_monitor_show_detail_screen(void)
{
	printf("[I] power_monitor: === SHOW DETAIL SCREEN (V2) ===\n");

	// Create if missing
	if (!detail_screen) {
		power_monitor_create_detail_screen();
	}

	if (detail_screen) {
		// Show the detail screen via template helper (idempotent create inside)
		detail_screen_show(detail_screen);
		printf("[I] power_monitor: Detail screen shown\n");

		// Debug: Log container size after initial content is added
		if (detail_screen->current_view_container) {
			printf("[I] power_monitor: Current view container size after initial content: %dx%d\n",
				lv_obj_get_width(detail_screen->current_view_container),
				lv_obj_get_height(detail_screen->current_view_container));
		}
	} else {
		printf("[E] power_monitor: Detail screen unavailable\n");
	}
}

void power_monitor_destroy_detail_screen(void)
{
	printf("[I] power_monitor: === DESTROY DETAIL SCREEN ===\n");

	// Clear sensor label references
	memset(&power_data.sensor_labels, 0, sizeof(power_monitor_sensor_labels_t));
}

// Touch event handler for detail screen
void power_monitor_handle_detail_touch(void)
{
		printf("[I] power_monitor: === HANDLE DETAIL TOUCH (V2) ===\n");
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
			printf("[I] power_monitor: View type %d found at index %d (setting not implemented)\n", view_type, i);
			return;
		}
	}
	printf("[W] power_monitor: View type %d not found in available views, keeping current\n", view_type);
}

// System reset function to clear corrupted state

// Modal close callback
static void power_monitor_close_modal(void)
{
	if (current_modal_is_timeline) {
		if (current_modal.timeline) {
			timeline_modal_destroy(current_modal.timeline);
			current_modal.timeline = NULL;
			printf("[I] power_monitor: Timeline modal closed\n");
		}
	} else {
		if (current_modal.alerts) {
			alerts_modal_hide(current_modal.alerts);
			alerts_modal_destroy(current_modal.alerts);
			current_modal.alerts = NULL;
			printf("[I] power_monitor: Alerts modal closed\n");
		}
	}
	current_modal_is_timeline = false;
}

// Handle back button - return to home screen
void power_monitor_handle_back_button(void)
{
	printf("[I] power_monitor: === BACK BUTTON CLICKED ===\n");

	// Clear modal if exists first
	if (current_modal_is_timeline) {
		if (current_modal.timeline) {
			timeline_modal_destroy(current_modal.timeline);
			current_modal.timeline = NULL;
		}
	} else {
		if (current_modal.alerts) {
			alerts_modal_destroy(current_modal.alerts);
			current_modal.alerts = NULL;
		}
	}
	current_modal_is_timeline = false;

	// Reset all static variables
	s_rendering_in_progress = false;

	// Reset view state
	power_monitor_power_grid_view_reset_state();

	// Handle back button - this will destroy the detail screen and clean up containers
	power_monitor_navigation_hide_detail_screen();

}

// Handle alerts button - open alerts modal
void power_monitor_handle_alerts_button(void)
{
	printf("[I] power_monitor: === ALERTS BUTTON CLICKED ===\n");
	printf("[I] power_monitor: ALERTS BUTTON HANDLER CALLED - THIS IS WORKING!\n");

	// Safety check: prevent multiple modals
	if (current_modal_is_timeline) {
		if (current_modal.timeline) {
			printf("[W] power_monitor: Timeline modal exists, closing first\n");
			timeline_modal_destroy(current_modal.timeline);
			current_modal.timeline = NULL;
		}
	} else {
		if (current_modal.alerts) {
			printf("[W] power_monitor: Alerts modal exists, destroying first\n");
			alerts_modal_destroy(current_modal.alerts);
			current_modal.alerts = NULL;
		}
	}
	current_modal_is_timeline = false;

	// Create alerts modal
	printf("[I] power_monitor: Creating alerts modal\n");
	current_modal.alerts = alerts_modal_create(&voltage_alerts_config, power_monitor_close_modal);

	if (current_modal.alerts) {
		printf("[I] power_monitor: Showing alerts modal\n");
		alerts_modal_show(current_modal.alerts);
		printf("[I] power_monitor: Alerts modal opened\n");
	} else {
		printf("[E] power_monitor: Failed to create alerts modal\n");
	}
}

// Handle timeline button - open timeline modal
void power_monitor_handle_timeline_button(void)
{
	printf("[I] power_monitor: === TIMELINE BUTTON CLICKED ===\n");

	// Safety check: prevent multiple modals
	if (current_modal_is_timeline) {
		if (current_modal.timeline) {
			printf("[W] power_monitor: Timeline modal already exists, closing first\n");
			timeline_modal_destroy(current_modal.timeline);
			current_modal.timeline = NULL;
		}
	} else {
		if (current_modal.alerts) {
			printf("[W] power_monitor: Alerts modal exists, destroying first\n");
			alerts_modal_destroy(current_modal.alerts);
			current_modal.alerts = NULL;
		}
	}
	current_modal_is_timeline = true;

	// Create timeline modal
	printf("[I] power_monitor: Creating timeline modal\n");
	current_modal.timeline = timeline_modal_create(&power_monitor_timeline_modal_config, power_monitor_close_modal);
	if (current_modal.timeline) {
		timeline_modal_show(current_modal.timeline);
		printf("[I] power_monitor: Timeline modal opened\n");
	} else {
		printf("[E] power_monitor: Failed to create timeline modal\n");
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



// Simple current view rendering - reuse existing view containers
void power_monitor_render_current_view(lv_obj_t* container)
{
	printf("[I] power_monitor: === RENDER CURRENT VIEW START ===\n");

	// Prevent recursive rendering
	if (s_rendering_in_progress) {
		printf("[I] power_monitor: Rendering already in progress, skipping\n");
		return;
	}
	s_rendering_in_progress = true;

	printf("[I] power_monitor: Rendering flag set, proceeding with view creation\n");

	printf("[I] power_monitor: === RENDER CURRENT VIEW: About to create view containers ===\n");

	// Make container clickable for touch navigation
	lv_obj_add_flag(container, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);


	// Get current view type from shared manager
	power_monitor_view_type_t current_type = get_current_view_type();
		printf("[I] power_monitor: Showing view type: %d (index: %d)\n", current_type, current_view_manager_get_index());

	// Always render the current view fresh - no complex cleanup logic
	if (current_type == POWER_MONITOR_VIEW_BAR_GRAPH) {
			printf("[I] power_monitor: Rendering power grid view content\n");
			power_monitor_power_grid_view_render(container);
			printf("[I] power_monitor: Power grid view shown and rendered\n");
	} else if (current_type == POWER_MONITOR_VIEW_NUMERICAL) {
			printf("[I] power_monitor: Rendering starter voltage view content\n");
			power_monitor_starter_voltage_view_render(container);
			printf("[I] power_monitor: Starter voltage view shown and rendered\n");
	}

	// Update gauge intervals for the current view's timeline settings
	power_monitor_update_gauge_intervals();

	// Check if content was actually created
	int child_count = lv_obj_get_child_cnt(container);
	printf("[I] power_monitor: After rendering, container has %d children\n", child_count);

	// Reset rendering flag
	s_rendering_in_progress = false;

	// CRITICAL: Ensure view state is consistent after rendering
	printf("[I] power_monitor: === RENDER CURRENT VIEW: Final state check ===\n");
	printf("[I] power_monitor: Current view type: %d, index: %d\n", current_type, current_view_manager_get_index());
	// Global containers removed - no need to check them

	printf("[I] power_monitor: === RENDER CURRENT VIEW COMPLETE ===\n");
}


// Touch callback for home current view - navigates to detail screen
static void power_monitor_home_current_view_touch_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	printf("[I] power_monitor: *** HOME TOUCH EVENT: code=%d (CLICKED=%d) ***\n", code, LV_EVENT_CLICKED);

	if (code != LV_EVENT_CLICKED) {
		printf("[I] power_monitor: Not a click event, ignoring\n");
		return;
	}

	static int touch_count = 0;
	touch_count++;
	printf("[I] power_monitor: *** HOME TOUCH CALLBACK CALLED #%d ***\n", touch_count);

	// Prevent recursive calls
	if (s_reset_in_progress) {
		printf("[W] power_monitor: Home touch callback ignored - navigation in progress\n");
		return;
	}

	printf("[I] power_monitor: Home current view touched - navigating to detail screen\n");

	// Update detail container with current view before navigating
	// Detail screen is handled by the detail screen template

	// Use the proper navigation function to update device state
	extern void screen_navigation_request_detail_view(const char *module_name);
	screen_navigation_request_detail_view("power-monitor");
}

// Touch callback for detail current view - cycles through views
static void power_monitor_detail_current_view_touch_cb(lv_event_t * e)
{
	static int touch_count = 0;
	touch_count++;
	printf("[I] power_monitor: *** DETAIL TOUCH CALLBACK CALLED #%d ***\n", touch_count);

	// Debug: Check event details
	if (e) {
		printf("[I] power_monitor: Event type: %d, target: %p, current target: %p\n",
			lv_event_get_code(e), lv_event_get_target(e), lv_event_get_current_target(e));
	}

	// Check if we're actually on the detail screen
	extern screen_type_t screen_navigation_get_current_screen(void);
	screen_type_t current_screen = screen_navigation_get_current_screen();
		printf("[I] power_monitor: Current screen: %d (detail=%d)\n", current_screen, SCREEN_DETAIL_VIEW);

	if (current_screen != SCREEN_DETAIL_VIEW) {
		printf("[W] power_monitor: Detail touch callback called but not on detail screen, ignoring\n");
		return;
	}

	// Prevent recursive calls
	if (s_reset_in_progress) {
		printf("[W] power_monitor: Detail touch callback ignored - navigation in progress\n");
		return;
	}

	printf("[I] power_monitor: Detail current view touched - cycling views\n");
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
	if (index < 0 || index >= 2) {
		printf("[W] power_monitor: Invalid view index %d from device state, using 0\n", index);
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
	if (index < 0 || index >= 2) {
		printf("[W] power_monitor: Invalid view index %d, clamping to valid range\n", index);
		index = (index < 0) ? 0 : 1;
	}

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
	printf("[I] power_monitor: Destroying current view objects for index %d, view_index\n");

	// CRITICAL: Cleanup gauge canvas buffers BEFORE destroying LVGL objects
	// This prevents memory leaks from malloc'd canvas buffers
	printf("[I] power_monitor: Cleaning up gauge canvas buffers before LVGL object destruction\n");
	extern void power_monitor_reset_static_gauges(void);
	extern void power_monitor_reset_starter_voltage_static_gauge(void);
	power_monitor_reset_static_gauges();
	power_monitor_reset_starter_voltage_static_gauge();

	// Get the detail screen container to clean
	extern detail_screen_t* detail_screen;
	if (detail_screen && detail_screen->current_view_container) {
		printf("[I] power_monitor: Cleaning current view container\n");
		lv_obj_clean(detail_screen->current_view_container);

		// Re-apply container styling after clean (clean removes styling)
		lv_obj_set_style_bg_color(detail_screen->current_view_container, lv_color_hex(0x000000), 0);
		lv_obj_set_style_border_width(detail_screen->current_view_container, 1, 0);
		lv_obj_set_style_border_color(detail_screen->current_view_container, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_radius(detail_screen->current_view_container, 4, 0);
		lv_obj_clear_flag(detail_screen->current_view_container, LV_OBJ_FLAG_SCROLLABLE);
	} else {
		printf("[W] power_monitor: No detail screen container to clean\n");
	}

	printf("[I] power_monitor: Current view objects destroyed for index %d, view_index\n");
}

// =============================================================================
// STANDARDIZED MODULE INTERFACE IMPLEMENTATION
// =============================================================================

/**
 * @brief Standardized module initialization function
 */
static void power_monitor_module_init(void)
{
	printf("[I] power_monitor: Power monitor module initializing via standardized interface\n");
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
	static int update_count = 0;
	update_count++;


	extern screen_type_t screen_navigation_get_current_screen(void);
	bool is_detail_screen = screen_navigation_get_current_screen() == SCREEN_DETAIL_VIEW;

	// Skip data updates during view cycling only if we're on detail screen
	if( s_detail_view_needs_refresh && is_detail_screen ){

		printf("[D] power_monitor: Skipping data updates during view cycling on detail screen\n");
	} else {

		power_monitor_update_data_only();
	}

	// Handle delayed detail view refresh after view cycling
	if( s_detail_view_needs_refresh ){

		if( is_detail_screen ){

			extern detail_screen_t* detail_screen;

			if( detail_screen && detail_screen->current_view_container ){

				printf("[I] power_monitor: Performing delayed re-render of detail view after cycle\n");

				// DESTROY: Properly destroy the current view object
				power_monitor_destroy_current_view();

				// Ensure layout is calculated on parent container before creating new view
				// Use detail screen's reusable layout preparation function for consistency
				if (!detail_screen_prepare_current_view_layout(detail_screen)) {
					printf("[E] power_monitor: Failed to prepare current view layout during cycling\n");
					s_detail_view_needs_refresh = false;
					return;
				}

				// CREATE: Create new view object with updated index
				int current_index = power_monitor_get_view_index();

				// Use the callback system instead of direct call
				if( detail_screen->on_current_view_created ){

					detail_screen->on_current_view_created( detail_screen->current_view_container );
				}

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
	printf("[I] power_monitor: Power monitor module cleaning up via standardized interface\n");
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