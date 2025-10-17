#include <stdio.h>
#include <string.h>

#include "starter_voltage_view.h"

#include "../power-monitor.h"
#include "../../shared/gauges/bar_graph_gauge.h"

#include "../../../state/device_state.h"

#include "../../../data/lerp_data/lerp_data.h"

#include "../../shared/palette.h"
#include "../../../fonts/lv_font_zector_72.h"


static const char *TAG = "starter_voltage_view";

// Static bar graph gauge for this view (single gauge)
static bar_graph_gauge_t s_voltage_gauge = {0};

// Static numeric value label
static lv_obj_t* s_voltage_value_label = NULL;
static lv_obj_t* s_voltage_title_label = NULL;

void power_monitor_starter_voltage_view_render(lv_obj_t *container)
{
	// Render starter voltage view

	if (!container) {
		printf("[E] starter_voltage_view: Container is NULL\n");
		return;
	}

	if (!lv_obj_is_valid(container)) {
		printf("[E] starter_voltage_view: Container is not valid\n");
		return;
	}

	// Container validated

	// Always create fresh content

	// Force container to be visible and sized
	lv_obj_clear_flag(container, LV_OBJ_FLAG_HIDDEN);

	// Get container dimensions
	lv_coord_t container_width = lv_obj_get_width(container);
	lv_coord_t container_height = lv_obj_get_height(container);


	// Set container background to black (border is handled by parent container)
	lv_obj_set_style_bg_color(container, lv_color_hex(0x000000), 0);
	lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
	// Don't modify border_width - let parent container handle it
	lv_obj_set_style_pad_all(container, 0, 0);
	lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

	int voltage_height = container_height / 3;
	int gauge_height = container_height - voltage_height;

	// Layout calculations

	s_voltage_title_label = lv_label_create(container);

	if (s_voltage_title_label) {

		lv_label_set_text(s_voltage_title_label, "Starter\nVoltage\n(V)");
		lv_obj_set_style_text_color(s_voltage_title_label, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_text_font(s_voltage_title_label, &lv_font_montserrat_14, 0);
		// Use fixed positioning for consistent text layout during view cycling
		lv_obj_align(s_voltage_title_label, LV_ALIGN_TOP_LEFT, 10, 5);
		lv_obj_clear_flag(s_voltage_title_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(s_voltage_title_label, LV_OBJ_FLAG_EVENT_BUBBLE);
	}

	s_voltage_value_label = lv_label_create(container);
	if (s_voltage_value_label) {

		lv_label_set_text(s_voltage_value_label, "12.6");
		lv_obj_set_style_text_color(s_voltage_value_label, lv_color_hex(0x00FF00), 0);
		lv_obj_set_style_text_font(s_voltage_value_label, &lv_font_zector_72, 0);

		// Position in top-right corner with fixed positioning for stability
		// Use fixed offset to prevent text shifting during view cycling
		lv_coord_t top_offset = 15; // Fixed 15px from top for consistent positioning

		// Position numeric value
		lv_obj_align(s_voltage_value_label, LV_ALIGN_TOP_RIGHT, -10, top_offset);

		// Allow layout to settle
		lv_obj_update_layout(container);

		lv_obj_clear_flag(s_voltage_value_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(s_voltage_value_label, LV_OBJ_FLAG_EVENT_BUBBLE);
	}

	lv_obj_t* gauge_container = lv_obj_create(container);

	if (gauge_container) {

		// Configure gauge container
		lv_obj_set_size(gauge_container, container_width, gauge_height);

		// Ensure layout is ready for alignment
		lv_obj_update_layout(container);

		// Try bottom-left alignment first
		lv_obj_align(gauge_container, LV_ALIGN_BOTTOM_LEFT, 0, 0);

		// Then center it horizontally
		lv_coord_t center_x = (container_width - container_width) / 2; // Should be 0 since gauge width = container width
		lv_obj_set_x(gauge_container, center_x);

		// Allow alignment to settle
		lv_obj_update_layout(container);
		lv_obj_set_style_bg_opa(gauge_container, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(gauge_container, 0, 0);
		lv_obj_set_style_pad_all(gauge_container, 2, 0); // Reduced padding to match power grid
		lv_obj_clear_flag(gauge_container, LV_OBJ_FLAG_SCROLLABLE);
		// Make gauge_container clickable for touch events to bubble up
		lv_obj_add_flag(gauge_container, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(gauge_container, LV_OBJ_FLAG_EVENT_BUBBLE);

		// Clean up any existing gauge first
		if (s_voltage_gauge.initialized) {

			printf("[I] starter_voltage_view: Cleaning up existing gauge before reinitializing\n");
			bar_graph_gauge_cleanup(&s_voltage_gauge);
			memset(&s_voltage_gauge, 0, sizeof(bar_graph_gauge_t));
		}

		bar_graph_gauge_init(
			&s_voltage_gauge, gauge_container,
			0, 0, container_width, gauge_height, 3, 1
		);

		// Check if gauge was initialized successfully
		if (!s_voltage_gauge.initialized) {
			printf("[E] starter_voltage_view: Failed to initialize bar graph gauge\n");
			return;
		}

		// Configure gauge for voltage display
		float starter_baseline = device_state_get_float("power_monitor.starter_baseline_voltage_v");
		float starter_min = device_state_get_float("power_monitor.starter_min_voltage_v");
		float starter_max = device_state_get_float("power_monitor.starter_max_voltage_v");

		bar_graph_gauge_configure_advanced(
			&s_voltage_gauge, // gauge pointer
			BAR_GRAPH_MODE_BIPOLAR, // graph mode
			starter_baseline, starter_min, starter_max, // bounds: baseline, min, max
			"", "", "", PALETTE_WARM_WHITE, // title, unit, y-axis unit, color
			false, true, false // Show title, Show Y-axis, Show Border
		);

		// Set update interval
		bar_graph_gauge_set_update_interval(&s_voltage_gauge, 33); // 30 FPS for high performance

		// Update labels and ticks
		bar_graph_gauge_update_labels_and_ticks(&s_voltage_gauge);
	}

	// Set timeline duration for the gauge
	int timeline_duration = device_state_get_int("power_monitor.gauge_timeline_settings.starter_voltage.current_view");
	uint32_t timeline_duration_ms = timeline_duration * 1000;
	bar_graph_gauge_set_timeline_duration(&s_voltage_gauge, timeline_duration_ms);

	// Set update interval for the gauge (no rate limiting - gauge calculates its own rate)
	bar_graph_gauge_set_update_interval(&s_voltage_gauge, 0);

	// Render complete
}

void power_monitor_starter_voltage_view_update_data(void)
{
	// Always update data

	// Get LERP data for smooth display values
	lerp_power_monitor_data_t lerp_data;
	lerp_data_get_current(&lerp_data);

	// Update numerical value display (right-aligned)
	if (s_voltage_value_label) {
		char value_text[32];
		snprintf(value_text, sizeof(value_text), "%.1f",
			lerp_value_get_display(&lerp_data.starter_voltage));
		lv_label_set_text(s_voltage_value_label, value_text);
	}

	// Update bar gauge with voltage data
	if (s_voltage_gauge.initialized) {

		bar_graph_gauge_add_data_point(
			&s_voltage_gauge,
			lerp_value_get_display(&lerp_data.starter_voltage)
		);
	}
}

void power_monitor_reset_starter_voltage_static_gauge(void)
{
	printf("[I] starter_voltage_view: === RESETTING STARTER VOLTAGE STATIC GAUGE ===\n");

	// Reset static gauge variable to prevent memory conflicts
	memset(&s_voltage_gauge, 0, sizeof(bar_graph_gauge_t));

	// Reset label pointers
	s_voltage_value_label = NULL;
	s_voltage_title_label = NULL;

	printf("[I] starter_voltage_view: Starter voltage static gauge reset complete\n");
}

void power_monitor_starter_voltage_view_apply_alert_flashing(const power_monitor_data_t* data,
	int starter_lo, int starter_hi, int house_lo, int house_hi,
	int solar_lo, int solar_hi, bool blink_on)
{
	// Apply alert flashing to the voltage value
	if (s_voltage_value_label && data) {
		// Get current lerp data for voltage value
		lerp_power_monitor_data_t lerp_data;
		lerp_data_get_current(&lerp_data);
		float current_voltage = lerp_value_get_display(&lerp_data.starter_voltage);

		if (blink_on && (current_voltage < starter_lo || current_voltage > starter_hi)) {
			lv_obj_set_style_text_color(s_voltage_value_label, PALETTE_YELLOW, 0);
		} else {
			lv_obj_set_style_text_color(s_voltage_value_label, PALETTE_WHITE, 0);
		}
	}
}

// Update starter voltage view gauge configuration when range values change
void power_monitor_starter_voltage_view_update_configuration(void)
{
	if (!s_voltage_gauge.initialized) {
		return;
	}

	// Read actual gauge configuration values from device state
	float starter_baseline = device_state_get_float("power_monitor.starter_baseline_voltage_v");
	float starter_min = device_state_get_float("power_monitor.starter_min_voltage_v");
	float starter_max = device_state_get_float("power_monitor.starter_max_voltage_v");

	// Update gauge configuration
	bar_graph_gauge_configure_advanced(
		&s_voltage_gauge, // gauge pointer
		BAR_GRAPH_MODE_BIPOLAR, // graph mode
		starter_baseline, starter_min, starter_max, // bounds: baseline, min, max
		"", "", "", PALETTE_WARM_WHITE, // title, unit, y-axis unit, color
		false, true, false // Show title, Show Y-axis, Show Border
	);

	bar_graph_gauge_update_labels_and_ticks(&s_voltage_gauge);
}

