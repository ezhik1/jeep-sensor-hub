#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "solar_power_view.h"

#include "../../power-monitor.h"
#include "../../../shared/views/single_value_bar_graph_view/single_value_bar_graph_view.h"
#include "../../../shared/palette.h"
#include "../../../shared/utils/warning_icon/warning_icon.h"

#include "../../../../fonts/lv_font_zector_72.h"

#include "../../../../data/lerp_data/lerp_data.h"
#include "../../../../state/device_state.h"
#include "../../../shared/utils/number_formatting/number_formatting.h"

static const char *TAG = "solar_power_view";

// State for the generic view (accessible from power-monitor.c)
single_value_bar_graph_view_state_t* single_view_solar_power = NULL;

static float calculate_wattage(float voltage, float current) {
	return voltage * current;
}

// Helper function to compute power bounds from voltage and current sensor values
static void compute_solar_power_bounds(float* min_power, float* baseline_power, float* max_power)
{
	char path[128];

	// Get voltage bounds
	snprintf(path, sizeof(path), "power_monitor.solar_min_voltage_v");
	float voltage_min = device_state_get_float(path);
	if (voltage_min == 0.0f) voltage_min = 0.0f; // Default fallback

	snprintf(path, sizeof(path), "power_monitor.solar_baseline_voltage_v");
	float voltage_baseline = device_state_get_float(path);
	if (voltage_baseline == 0.0f) voltage_baseline = 0.0f; // Default fallback

	snprintf(path, sizeof(path), "power_monitor.solar_max_voltage_v");
	float voltage_max = device_state_get_float(path);
	if (voltage_max == 0.0f) voltage_max = 22.0f; // Default fallback

	// Get current bounds
	snprintf(path, sizeof(path), "power_monitor.solar_min_current_a");
	float current_min = device_state_get_float(path);
	if (current_min == 0.0f) current_min = 0.0f; // Default fallback

	snprintf(path, sizeof(path), "power_monitor.solar_baseline_current_a");
	float current_baseline = device_state_get_float(path);
	if (current_baseline == 0.0f) current_baseline = 0.0f; // Default fallback

	snprintf(path, sizeof(path), "power_monitor.solar_max_current_a");
	float current_max = device_state_get_float(path);
	if (current_max == 0.0f) current_max = 18.0f; // Default fallback

	// Compute power bounds: P = V × A
	*min_power = voltage_min * current_min;
	*baseline_power = voltage_baseline * current_baseline;
	*max_power = voltage_max * current_max;

	printf("[D] solar_power_view: Computed power bounds: min=%.1fW, baseline=%.1fW, max=%.1fW\n",
		*min_power, *baseline_power, *max_power);
}

void power_monitor_solar_power_view_render(lv_obj_t *container)
{
	printf("[D] solar_power_view_render: Starting\n");
	if (!container || !lv_obj_is_valid(container)) {
		return;
	}

	// Clean up existing state if any
	if (single_view_solar_power && single_view_solar_power->initialized) {
		single_value_bar_graph_view_destroy(single_view_solar_power);
		single_view_solar_power = NULL;
	}

	// Clear any existing children in the container (after state cleanup)
	lv_obj_clean(container);

	// Calculate power bounds from voltage and current settings
	float min_power, baseline_power, max_power;
	compute_solar_power_bounds(&min_power, &baseline_power, &max_power);

	// Create configuration for solar_power view
	single_value_bar_graph_view_config_t config = {
		.title = "SOLAR\nPOWER",
		.unit = "(W)",
		.bar_graph_color = PALETTE_WARM_WHITE,
		.bar_mode = BAR_GRAPH_MODE_POSITIVE_ONLY,
		.baseline_value = baseline_power,
		.min_value = min_power,
		.max_value = max_power,
		.number_config = {
			.label = NULL, // Will be set by the view
			.font = &lv_font_zector_72,
			.color = PALETTE_WARM_WHITE,
			.warning_color = PALETTE_YELLOW,
			.error_color = PALETTE_RED,
			.show_warning = true,
			.show_error = false,
			.warning_icon_size = WARNING_ICON_SIZE_50,
			.number_alignment = LABEL_ALIGN_RIGHT,
			.warning_alignment = LABEL_ALIGN_CENTER
		}
	};

	// Clean up existing state if any
	if (single_view_solar_power) {

		single_value_bar_graph_view_destroy(single_view_solar_power);
		free(single_view_solar_power);
		single_view_solar_power = NULL;
	}

	// Create the view using the shared component
	single_view_solar_power = single_value_bar_graph_view_create(container, &config);
	if (!single_view_solar_power) {

		return;
	}

	// Update the gauge pointer in the power monitor system
	power_monitor_update_single_view_gauge_pointer();
}

void power_monitor_solar_power_view_update_data(void)
{
	if (!single_view_solar_power || !single_view_solar_power->initialized) {

		return;
	}

	// Get current data from LERP data
	lerp_power_monitor_data_t lerp_data;
	lerp_data_get_current(&lerp_data);

	float value = calculate_wattage(lerp_value_get_display(&lerp_data.solar_voltage), lerp_value_get_display(&lerp_data.solar_current));

	// Get error state from power monitor data
	power_monitor_data_t* power_data = power_monitor_get_data();
	bool has_error = power_data ? false : false;

	// Update the generic view (which adds data to its gauge)
	single_value_bar_graph_view_update_data(single_view_solar_power, value, has_error);
}

void power_monitor_reset_solar_power_static_gauge(void)
{
	// Clean up the generic view state
	if (single_view_solar_power) {
		if (single_view_solar_power->initialized) {
			single_value_bar_graph_view_destroy(single_view_solar_power);
		}
		single_view_solar_power = NULL;
	}
}
