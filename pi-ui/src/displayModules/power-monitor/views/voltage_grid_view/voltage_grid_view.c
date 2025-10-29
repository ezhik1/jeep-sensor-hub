#include <stdio.h>
#include <string.h>

#include "../../power-monitor.h"
#include "voltage_grid_view.h"

#include "../../../../state/device_state.h"
#include "../../../../data/lerp_data/lerp_data.h"
#include "../../../../app_data_store.h"

#include "../../../shared/gauges/bar_graph_gauge/bar_graph_gauge.h"
#include "../../../shared/utils/number_formatting/number_formatting.h"
#include "../../../shared/utils/warning_icon/warning_icon.h"

#include "../../../shared/palette.h"
#include "../../../../fonts/lv_font_noplato_24.h"

// Device state for JSON-backed history
#include "../../../../state/device_state.h"

static const char *TAG = "voltage_grid_view";

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
#define NUMERIC_VALUE_PERCENT      27     // Percentage of width for numeric value
#define BAR_GRAPH_PERCENT          73     // Percentage of width for bar graph

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

	// Set up flexbox for row (horizontal: 27% numeric + 73% gauge)
	lv_obj_set_flex_flow(row_container, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(row_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(row_container, 2, 0); // No gap between numeric and gauge

	// NUMERIC CONTAINER : 27% of width
	lv_obj_t* numeric_container = lv_obj_create(row_container);
	lv_obj_set_size(numeric_container, LV_PCT(NUMERIC_VALUE_PERCENT), LV_SIZE_CONTENT);
	lv_obj_set_style_bg_opa(numeric_container, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(numeric_container, 0, 0); // No border
	lv_obj_set_style_radius(numeric_container, 0, 0); // No border radius
	lv_obj_set_style_pad_all(numeric_container, 0, 0);
	lv_obj_set_style_pad_left(numeric_container, 2, 0);
	lv_obj_clear_flag(numeric_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(numeric_container, LV_OBJ_FLAG_EVENT_BUBBLE);

	lv_obj_set_flex_flow(numeric_container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(numeric_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(numeric_container, 0, 0);

	// Create a container for the value label to handle warning icons properly
	lv_obj_t* value_container = lv_obj_create(numeric_container);
	lv_obj_set_size(value_container, 60, 30); // Fixed size for value area (wider for 4-digit numbers)
	lv_obj_set_style_bg_opa(value_container, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(value_container, 0, 0);
	lv_obj_set_style_pad_all(value_container, 0, 0);
	lv_obj_clear_flag(value_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(value_container, LV_OBJ_FLAG_EVENT_BUBBLE);

	// Number Value for Gauge Row
	*value_label = lv_label_create(value_container);
	lv_label_set_text(*value_label, "00.0");
	lv_obj_set_size(*value_label, 60, LV_SIZE_CONTENT); // Fixed width for 4 characters (00.0)
	lv_obj_set_style_text_color(*value_label, color, 0);
	lv_obj_set_style_text_font(*value_label, &lv_font_noplato_24, 0); // Use monospace font
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

	// Center the value label in its container
	lv_obj_center(*value_label);

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

	// GAUGE CONTAINER : 73% of width
	lv_obj_t* gauge_container = lv_obj_create(row_container);
	lv_obj_set_size(gauge_container, LV_PCT(BAR_GRAPH_PERCENT), LV_PCT(100));
	lv_obj_set_style_bg_opa(gauge_container, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(gauge_container, 0, 0); // No border
	lv_obj_set_style_radius(gauge_container, 0, 0); // No border radius
	lv_obj_set_style_pad_all(gauge_container, 0, 0);
	// lv_obj_set_style_pad_bottom(gauge_container, 0, 0); // 2px bottom padding (overrides pad_all)
	lv_obj_clear_flag(gauge_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(gauge_container, LV_OBJ_FLAG_EVENT_BUBBLE);

	// 0,0 gauge width to use flex layout
	bar_graph_gauge_init(gauge, gauge_container, 0, 0, 0, 0, 2, 3);
	bar_graph_gauge_configure_advanced(
		gauge, // gauge pointer
		mode, // graph mode
		baseline, min_val, max_val, // bounds: baseline, min, max
		"", "V", "V", color, // title, unit, y-axis unit, color
		false, true, false // Show title, Show Y-axis, Show Border
	);
}

// Static bar graph gauges for this view (temporarily reverted from shared)
bar_graph_gauge_t s_starter_voltage_gauge;
bar_graph_gauge_t s_house_voltage_gauge;
bar_graph_gauge_t s_solar_voltage_gauge;

// Static numeric value labels and title labels for each gauge
static lv_obj_t* s_starter_value_label = NULL;
static lv_obj_t* s_starter_title_label = NULL;
static lv_obj_t* s_house_value_label = NULL;
static lv_obj_t* s_house_title_label = NULL;
static lv_obj_t* s_solar_value_label = NULL;
static lv_obj_t* s_solar_title_label = NULL;

// Value editor for interactive editing
static int s_current_editing_gauge = -1; // -1 = none, 0 = starter, 1 = house, 2 = solar


void power_monitor_voltage_grid_view_render(lv_obj_t *container)
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
	printf("[I] voltage_grid_view: Voltage grid container dimensions: %dx%d\n", container_width, container_height);

	// Use the container as-is - don't resize it!
	printf("[I] voltage_grid_view: Using container dimensions as-is: %dx%d\n", container_width, container_height);

	// Set up flexbox for the main container (vertical stack)
	lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_gap(container, 0, 0); // No vertical gap between gauges
	lv_obj_set_style_pad_all(container, CONTAINER_PADDING_PX, 0); // Container padding

	// Calculate gauge dimensions using configuration
	int gauge_height = (container_height - CONTAINER_PADDING_PX) / 3;

	printf("[D] voltage_grid_view: Container dimensions: %dx%d, gauge_height=%d\n", container_width, container_height, gauge_height);

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
		"CABIN\n(V)", PALETTE_WARM_WHITE,
		container_width, gauge_height,
		starter_baseline, starter_min, starter_max,
		BAR_GRAPH_MODE_BIPOLAR, &lv_font_montserrat_16, 0
	);

	// Apply timeline settings for current view
	power_monitor_update_gauge_timeline_duration(POWER_MONITOR_GAUGE_GRID_STARTER_VOLTAGE);

	create_gauge_row(
		container, &s_house_voltage_gauge,
		&s_house_value_label, &s_house_title_label,
		"HOUSE\n(V)", PALETTE_WARM_WHITE,
		container_width, gauge_height,
		house_baseline, house_min, house_max,
		BAR_GRAPH_MODE_BIPOLAR, &lv_font_montserrat_16, 1
	);

	// Apply timeline settings for current view
	power_monitor_update_gauge_timeline_duration(POWER_MONITOR_GAUGE_GRID_HOUSE_VOLTAGE);

	create_gauge_row(
		container, &s_solar_voltage_gauge,
		&s_solar_value_label, &s_solar_title_label,
		"SOLAR\n(V)", PALETTE_WARM_WHITE,
		container_width, gauge_height,
		0.0f, solar_min, solar_max,
		BAR_GRAPH_MODE_POSITIVE_ONLY, &lv_font_montserrat_16, 2
	);

	// Apply timeline settings for current view
	power_monitor_update_gauge_timeline_duration(POWER_MONITOR_GAUGE_GRID_SOLAR_VOLTAGE);

	// Gauges are now seeded directly when bar_graph_gauge_add_data_point is called

	// Update labels and ticks for Y-axis (show ticks but not values)
	bar_graph_gauge_update_y_axis_labels(&s_starter_voltage_gauge);
	bar_graph_gauge_update_y_axis_labels(&s_house_voltage_gauge);
	bar_graph_gauge_update_y_axis_labels(&s_solar_voltage_gauge);

	// Mark view as initialized
	s_view_initialized = true;
}

// Reset view state when view is destroyed
void power_monitor_voltage_grid_view_reset_state(void)
{
	// History persistence is now handled by centralized system in power-monitor.c
	// Just cleanup gauges
	if (s_starter_voltage_gauge.initialized) {
		bar_graph_gauge_cleanup(&s_starter_voltage_gauge);
	}
	if (s_house_voltage_gauge.initialized) {
		bar_graph_gauge_cleanup(&s_house_voltage_gauge);
	}
	if (s_solar_voltage_gauge.initialized) {
		bar_graph_gauge_cleanup(&s_solar_voltage_gauge);
	}

	// Clear row containers if still valid
	for (int i = 0; i < 3; i++) {
		if (s_row_containers[i] && lv_obj_is_valid(s_row_containers[i])) {
			lv_obj_del(s_row_containers[i]);
			s_row_containers[i] = NULL;
		}
	}

	s_view_initialized = false;
}


void power_monitor_voltage_grid_view_update_data(void)
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
	// Note: bar_graph_gauge_add_data_point() handles rendering internally

	if (s_starter_voltage_gauge.initialized) {

		// Update numeric value label with current value
		if (s_starter_value_label && lv_obj_is_valid(s_starter_value_label)) {
			// Get power monitor data to check for errors
			power_monitor_data_t* power_data_pointer = power_monitor_get_data();

			number_formatting_config_t config = {
				.label = s_starter_value_label,
				.font = &lv_font_noplato_24, // Use monospace font
				.color = PALETTE_WARM_WHITE,
				.warning_color = PALETTE_YELLOW,
				.error_color = PALETTE_RED, // Red for errors
				.show_warning = false, // No warning for power grid view
				.show_error = power_data_pointer->starter_battery.voltage.error,
				.warning_icon_size = WARNING_ICON_SIZE_30,
				.number_alignment = LABEL_ALIGN_CENTER,
				.warning_alignment = LABEL_ALIGN_CENTER
			};
			format_and_display_number(starter_voltage, &config);
		}
	}

	if (s_house_voltage_gauge.initialized) {

		// Update numeric value label with current value
		if (s_house_value_label && lv_obj_is_valid(s_house_value_label)) {
			// Get power monitor data to check for errors
			power_monitor_data_t* power_data_pointer = power_monitor_get_data();

			number_formatting_config_t config = {
				.label = s_house_value_label,
				.font = &lv_font_noplato_24, // Use monospace font
				.color = PALETTE_WARM_WHITE,
				.warning_color = PALETTE_YELLOW,
				.error_color = PALETTE_RED, // Red for errors
				.show_warning = false,
				.show_error = power_data_pointer->house_battery.voltage.error,
				.warning_icon_size = WARNING_ICON_SIZE_30,
				.number_alignment = LABEL_ALIGN_CENTER,
				.warning_alignment = LABEL_ALIGN_CENTER
			};
			format_and_display_number(house_voltage, &config);
		}
	}

	if (s_solar_voltage_gauge.initialized) {

		// Update numeric value label with current value
		if (s_solar_value_label && lv_obj_is_valid(s_solar_value_label)) {
			// Get power monitor data to check for errors
			power_monitor_data_t* power_data_pointer = power_monitor_get_data();

			number_formatting_config_t config = {
				.label = s_solar_value_label,
				.font = &lv_font_noplato_24, // Use monospace font
				.color = PALETTE_WARM_WHITE,
				.warning_color = PALETTE_YELLOW,
				.error_color = PALETTE_RED, // Red for errors
				.show_warning = false,
				.show_error = power_data_pointer->solar_input.voltage.error,
				.warning_icon_size = WARNING_ICON_SIZE_30,
				.number_alignment = LABEL_ALIGN_CENTER,
				.warning_alignment = LABEL_ALIGN_CENTER
			};
			format_and_display_number(solar_voltage, &config);
		}
	}

	// Mark view as initialized for proper cleanup
	s_view_initialized = true;
}



void power_monitor_reset_static_gauges(void)
{

	// Properly cleanup all bar graph gauges to free canvas buffers
	if (s_starter_voltage_gauge.initialized) {

		bar_graph_gauge_cleanup(&s_starter_voltage_gauge);
	}
	if (s_house_voltage_gauge.initialized) {

		bar_graph_gauge_cleanup(&s_house_voltage_gauge);
	}
	if (s_solar_voltage_gauge.initialized) {

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
}

// Apply alert flashing to current view values
void power_monitor_voltage_grid_view_apply_alert_flashing(const power_monitor_data_t* data, int starter_lo, int starter_hi, int house_lo, int house_hi, int solar_lo, int solar_hi, bool blink_on)
{
	if (!data) return;

	// Get LERP data for raw values (for threshold checking)
	lerp_power_monitor_data_t lerp_data;
	lerp_data_get_current(&lerp_data);

	// Apply alert flashing using generic function
	if (s_starter_value_label) {
		float starter_v_raw = lerp_value_get_raw(&lerp_data.starter_voltage);
		apply_alert_flashing(s_starter_value_label, starter_v_raw, (float)starter_lo, (float)starter_hi, blink_on);
	}

	if (s_house_value_label) {
		float house_v_raw = lerp_value_get_raw(&lerp_data.house_voltage);
		apply_alert_flashing(s_house_value_label, house_v_raw, (float)house_lo, (float)house_hi, blink_on);
	}

	if (s_solar_value_label) {
		float solar_v_raw = lerp_value_get_raw(&lerp_data.solar_voltage);
		apply_alert_flashing(s_solar_value_label, solar_v_raw, (float)solar_lo, (float)solar_hi, blink_on);
	}
}

// Function to update gauge configuration with current device state values
void power_monitor_voltage_grid_view_update_configuration(void)
{
	if (!s_view_initialized) return;

	printf("[I] voltage_grid_view: Updating power grid view gauge configuration...\n");

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
			"", "V", "V", PALETTE_WARM_WHITE, false, true, false // No border for current view
		);
	}

	// Update house gauge configuration
	if (s_house_voltage_gauge.initialized) {

		bar_graph_gauge_configure_advanced(&s_house_voltage_gauge,
			BAR_GRAPH_MODE_BIPOLAR, house_baseline, house_min, house_max,
			"", "V", "V", PALETTE_WARM_WHITE, false, true, false // No border for current view
		);
	}

	// Update solar gauge configuration
	if (s_solar_voltage_gauge.initialized) {

		bar_graph_gauge_configure_advanced(&s_solar_voltage_gauge,
			BAR_GRAPH_MODE_POSITIVE_ONLY, 0.0f, solar_min, solar_max,
			"", "V", "V", PALETTE_WARM_WHITE, false, true, false // No border for current view
		);
	}
}
