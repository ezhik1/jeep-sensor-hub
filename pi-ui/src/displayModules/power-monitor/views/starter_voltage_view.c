#include <stdio.h>
#include "starter_voltage_view.h"
#include "../../../state/device_state.h"
#include "../power-monitor.h"
#include "../../shared/gauges/bar_graph_gauge.h"
#include "../../../data/lerp_data/lerp_data.h"
#include "../../../fonts/lv_font_zector_72.h"

#include <stdio.h>
#include <string.h>

static const char *TAG = "starter_voltage_view";

// Static bar graph gauge for this view (single gauge)
static bar_graph_gauge_t s_voltage_gauge = {0};

// Static numeric value label
static lv_obj_t* s_voltage_value_label = NULL;
static lv_obj_t* s_voltage_title_label = NULL;

void power_monitor_starter_voltage_view_render(lv_obj_t *container)
{
	printf("[I] starter_voltage_view: === STARTER VOLTAGE VIEW RENDER START ===\n");
		printf("[I] starter_voltage_view: Container: %p\n", container);

	if (!container) {
		printf("[E] starter_voltage_view: Container is NULL\n");
		return;
	}

	if (!lv_obj_is_valid(container)) {
		printf("[E] starter_voltage_view: Container is not valid\n");
		return;
	}

	printf("[I] starter_voltage_view: Container validation passed\n");

	// Always create fresh content
	printf("[I] starter_voltage_view: Creating fresh starter voltage view content\n");

		printf("[I] starter_voltage_view: Creating starter voltage view in container: %p\n", container);

	// Always create fresh gauge

	// Always create fresh labels

	// Container is already cleaned by power_monitor_create_current_view_content
	// No need to clean again here

	// Force container to be visible and sized
	lv_obj_clear_flag(container, LV_OBJ_FLAG_HIDDEN);

	// Get container dimensions
	lv_coord_t container_width = lv_obj_get_width(container);
	lv_coord_t container_height = lv_obj_get_height(container);

		printf("[I] starter_voltage_view: Starter voltage view container dimensions: %dx%d\n", container_width, container_height);

	// If container is still 0x0 after layout updates, this indicates a problem
	if (container_width == 0 || container_height == 0) {
		printf("[E] starter_voltage_view: Container has invalid dimensions %dx%d - layout calculation failed\n", container_width, container_height);
		// Force a layout update and try again
		lv_obj_update_layout(container);
		container_width = lv_obj_get_width(container);
		container_height = lv_obj_get_height(container);
		printf("[I] starter_voltage_view: After forced layout update: %dx%d\n", container_width, container_height);

		// If still invalid, use fallback but log as error
		if (container_width == 0 || container_height == 0) {
			printf("[E] starter_voltage_view: Layout calculation completely failed, using fallback dimensions\n");
			container_width = 238;  // Same as home screen module
			container_height = 189; // Same as home screen module
			lv_obj_set_size(container, container_width, container_height);
		}
	}

	// Set container background to black (border is handled by parent container)
	lv_obj_set_style_bg_color(container, lv_color_hex(0x000000), 0);
	lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
	// Don't modify border_width - let parent container handle it
	lv_obj_set_style_pad_all(container, 0, 0);
	lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

	int voltage_height = container_height / 3;
	int gauge_height = container_height - voltage_height;

		printf("[I] starter_voltage_view: Layout: voltage_height=%d, gauge_height=%d\n", voltage_height, gauge_height);

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

		lv_obj_align(s_voltage_value_label, LV_ALIGN_TOP_RIGHT, -10, top_offset);
		printf("[I] starter_voltage_view: Positioned numeric value at fixed top_offset=%d (container_height=%d)\n", top_offset, container_height);

		lv_obj_clear_flag(s_voltage_value_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(s_voltage_value_label, LV_OBJ_FLAG_EVENT_BUBBLE);
	}

	lv_obj_t* gauge_container = lv_obj_create(container);
	if (gauge_container) {
		lv_obj_set_size(gauge_container, container_width, gauge_height);
		lv_obj_align(gauge_container, LV_ALIGN_BOTTOM_MID, 0, 0);
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

	// Initialize the bar graph gauge with full container size (flexbox will handle layout)
		printf("[I] starter_voltage_view: Initializing bar graph gauge in container: %p\n", gauge_container);
	printf("[I] starter_voltage_view: About to call bar_graph_gauge_init...\n");
	bar_graph_gauge_init(&s_voltage_gauge, gauge_container,
		0, 0, container_width, gauge_height);
	printf("[I] starter_voltage_view: bar_graph_gauge_init completed\n");

		// Check if gauge was initialized successfully
		if (!s_voltage_gauge.initialized) {
			printf("[E] starter_voltage_view: Failed to initialize bar graph gauge\n");
			return;
		}

		// Configure gauge for voltage display
		float starter_baseline = device_state_get_float("power_monitor.starter_baseline_voltage_v");
		float starter_min = device_state_get_float("power_monitor.starter_min_voltage_v");
		float starter_max = device_state_get_float("power_monitor.starter_max_voltage_v");

		printf("[I] starter_voltage_view: Configuring gauge: baseline=%.1f, min=%.1f, max=%.1f\n", starter_baseline, starter_min, starter_max);
		bar_graph_gauge_configure_advanced(&s_voltage_gauge,
			BAR_GRAPH_MODE_BIPOLAR, starter_baseline, starter_min, starter_max,
			"", "", "", 0x00FF00, false, true, false); // No labels, just bars and scale, no border for current view

		// Set bar width and gap to match other gauges
		s_voltage_gauge.bar_width = 3;
		s_voltage_gauge.bar_gap = 1;

		// Set update interval
		bar_graph_gauge_set_update_interval(&s_voltage_gauge, 33); // 30 FPS for high performance

		// Update labels and ticks
		bar_graph_gauge_update_labels_and_ticks(&s_voltage_gauge);

		printf("[I] starter_voltage_view: Bar gauge initialized: container=%dx%d\n", container_width - 10, gauge_height - 10);
	}

	// Set timeline duration for the gauge
	int timeline_duration = device_state_get_int("power_monitor.gauge_timeline_settings.starter_voltage.current_view");
	uint32_t timeline_duration_ms = timeline_duration * 1000;
	bar_graph_gauge_set_timeline_duration(&s_voltage_gauge, timeline_duration_ms);

	// Set update interval for the gauge (no rate limiting - gauge calculates its own rate)
	bar_graph_gauge_set_update_interval(&s_voltage_gauge, 0);

	printf("[I] starter_voltage_view: Gauge timeline set to %d ms\n", timeline_duration_ms);

	// View content created successfully

	// View rendered successfully
	printf("[I] starter_voltage_view: Starter voltage view rendered successfully\n");
	printf("[I] starter_voltage_view: === STARTER VOLTAGE VIEW RENDER COMPLETE ===\n");

	// Log memory usage
	FILE* meminfo = fopen("/proc/self/status", "r");
	if (meminfo) {
		char line[256];
		while (fgets(line, sizeof(line), meminfo)) {
			if (strncmp(line, "VmRSS:", 6) == 0) {
				printf("[I] starter_voltage_view: Memory usage: %s\n", line);
				break;
			}
		}
		fclose(meminfo);
	}
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
		bar_graph_gauge_add_data_point(&s_voltage_gauge,
			lerp_value_get_display(&lerp_data.starter_voltage));
	}
}


const char* power_monitor_starter_voltage_view_get_title(void)
{
	return "Starter Voltage";
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
			lv_obj_set_style_text_color(s_voltage_value_label, lv_color_hex(0xFF0000), 0);
		} else {
			lv_obj_set_style_text_color(s_voltage_value_label, lv_color_hex(0x00FF00), 0);
		}
	}
}

