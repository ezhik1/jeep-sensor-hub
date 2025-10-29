#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "house_current_view.h"

#include "../../power-monitor.h"
#include "../../../shared/views/single_value_bar_graph_view/single_value_bar_graph_view.h"
#include "../../../shared/gauges/bar_graph_gauge/bar_graph_gauge.h"
#include "../../../shared/palette.h"
#include "../../../shared/utils/warning_icon/warning_icon.h"

#include "../../../../fonts/lv_font_zector_72.h"

#include "../../../../data/lerp_data/lerp_data.h"
#include "../../../../state/device_state.h"
#include "../../../shared/utils/number_formatting/number_formatting.h"

static const char *TAG = "house_current_view";

// State for the generic view (accessible from power-monitor.c)
single_value_bar_graph_view_state_t* single_view_house_current = NULL;

void power_monitor_house_current_view_render(lv_obj_t *container)
{
	printf("[D] house_current_view_render: Starting\n");
	if (!container || !lv_obj_is_valid(container)) {
		return;
	}

	// Clean up existing state if any
	if (single_view_house_current) {
		if (single_view_house_current->initialized) {
			// Clean up gauge first (frees canvas buffer and timer)
			bar_graph_gauge_cleanup(&single_view_house_current->gauge);
			// Then destroy the view (destroys() frees the struct memory)
			single_value_bar_graph_view_destroy(single_view_house_current);
		} else {
			// If not initialized, just free the memory directly
			free(single_view_house_current);
		}
		single_view_house_current = NULL;
	}

	// Clear any existing children in the container (after state cleanup)
	lv_obj_clean(container);

	// Create configuration for house_current view
	single_value_bar_graph_view_config_t config = {
		.title = "HOUSE\nCURRENT",
		.unit = "(A)",
		.bar_graph_color = PALETTE_WARM_WHITE,
		.bar_mode = BAR_GRAPH_MODE_BIPOLAR,
		.baseline_value = 0.0f,
		.min_value = device_state_get_float("power_monitor.house_min_current_a"),
		.max_value = device_state_get_float("power_monitor.house_max_current_a"),
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

	// Create the view using the shared component
	single_view_house_current = single_value_bar_graph_view_create(container, &config);
	if (!single_view_house_current) {

		return;
	}

	// Update the gauge pointer in the power monitor system
	power_monitor_update_single_view_gauge_pointer();
}

void power_monitor_house_current_view_update_data(void)
{
	if (!single_view_house_current || !single_view_house_current->initialized) {

		return;
	}

	// Get current data from LERP data
	lerp_power_monitor_data_t lerp_data;
	lerp_data_get_current(&lerp_data);

	float value = lerp_value_get_display(&lerp_data.house_current);

	// Get error state from power monitor data
	power_monitor_data_t* power_data = power_monitor_get_data();
	bool has_error = power_data ? power_data->house_battery.current.error : false;

	// Update the generic view (which adds data to its gauge)
	single_value_bar_graph_view_update_data(single_view_house_current, value, has_error);
}

void power_monitor_reset_house_current_static_gauge(void)
{
	// Clean up the generic view state
	if (single_view_house_current) {
		if (single_view_house_current->initialized) {
			// Clean up gauge first (frees canvas buffer and timer)
			bar_graph_gauge_cleanup(&single_view_house_current->gauge);
			// Then destroy the view (destroy() already frees the memory)
			single_value_bar_graph_view_destroy(single_view_house_current);
		} else {
			// If not initialized, free directly
			free(single_view_house_current);
		}
		single_view_house_current = NULL;
	}
}
