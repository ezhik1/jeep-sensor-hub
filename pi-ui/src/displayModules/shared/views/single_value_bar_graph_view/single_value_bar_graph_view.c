#include "single_value_bar_graph_view.h"
#include "../../gauges/bar_graph_gauge/bar_graph_gauge.h"
#include "../../palette.h"
#include "../../../../fonts/lv_font_noplato_14.h"
#include "../../../../state/device_state.h"
#include "../../../../app_data_store.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

single_value_bar_graph_view_state_t* single_value_bar_graph_view_create(lv_obj_t* parent, const single_value_bar_graph_view_config_t* config)
{

	if (!parent || !config) {

		return NULL;
	}

	single_value_bar_graph_view_state_t* base_view = malloc(sizeof(single_value_bar_graph_view_state_t));

	if (!base_view) {

		return NULL;
	}

	// Initialize state
	memset(base_view, 0, sizeof(single_value_bar_graph_view_state_t));

	// Get container dimensions
	lv_coord_t container_width = lv_obj_get_width(parent);
	lv_coord_t container_height = lv_obj_get_height(parent);

	// Check if container is valid
	if (!lv_obj_is_valid(parent)) {

		free(base_view);
		return NULL;
	}

	// Calculate row heights (1/3 for top, 2/3 for bottom)
	int voltage_height = container_height / 3;
	int gauge_height = container_height - voltage_height;

	// Set container background to black (border is handled by parent container)
	lv_obj_set_style_bg_color(parent, lv_color_hex(0x000000), 0);
	lv_obj_set_style_bg_opa(parent, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_all(parent, 0, 0);
	lv_obj_clear_flag(parent, LV_OBJ_FLAG_SCROLLABLE);

	// Create title/unit container for vertical stacking
	base_view->title_container = lv_obj_create(parent);
	if (base_view->title_container) {
		// Position at top-left with some padding
		lv_obj_align(base_view->title_container, LV_ALIGN_TOP_LEFT, 10, 5);
		// Set size to accommodate title and unit
		lv_obj_set_size(base_view->title_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		// Configure container
		lv_obj_set_style_bg_opa(base_view->title_container, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(base_view->title_container, 0, 0);
		lv_obj_set_style_pad_all(base_view->title_container, 0, 0);
		lv_obj_clear_flag(base_view->title_container, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_clear_flag(base_view->title_container, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(base_view->title_container, LV_OBJ_FLAG_EVENT_BUBBLE);

		// Set up vertical flex layout
		lv_obj_set_flex_flow(base_view->title_container, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(base_view->title_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
		lv_obj_set_style_pad_gap(base_view->title_container, 5, 0); // 5px gap between title and unit
	}

	// Create title label
	base_view->title_label = lv_label_create(base_view->title_container);
	if (base_view->title_label) {
		lv_label_set_text(base_view->title_label, config->title);
		lv_obj_set_style_text_color(base_view->title_label, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_text_font(base_view->title_label, &lv_font_montserrat_12, 0);
		lv_obj_set_style_text_align(base_view->title_label, LV_TEXT_ALIGN_LEFT, 0);
		lv_obj_clear_flag(base_view->title_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(base_view->title_label, LV_OBJ_FLAG_EVENT_BUBBLE);
	}

	// Create unit label
	base_view->unit_label = lv_label_create(base_view->title_container);
	if (base_view->unit_label) {
		lv_label_set_text(base_view->unit_label, config->unit);
		lv_obj_set_style_text_color(base_view->unit_label, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_text_font(base_view->unit_label, &lv_font_montserrat_12, 0);
		lv_obj_set_style_text_align(base_view->unit_label, LV_TEXT_ALIGN_LEFT, 0);
		lv_obj_clear_flag(base_view->unit_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(base_view->unit_label, LV_OBJ_FLAG_EVENT_BUBBLE);
	}

	// Create value label
	base_view->value_label = lv_label_create(parent);
	if (base_view->value_label) {
		lv_label_set_text(base_view->value_label, "12.6");
		lv_obj_set_style_text_color(base_view->value_label, config->number_config.color, 0);
		lv_obj_set_style_text_font(base_view->value_label, config->number_config.font, 0);

		// Position in top-right corner with fixed positioning for stability
		lv_coord_t top_offset = 15; // Fixed 15px from top for consistent positioning
		lv_obj_align(base_view->value_label, LV_ALIGN_TOP_RIGHT, -10, top_offset);

		// Allow layout to settle
		lv_obj_update_layout(parent);

		lv_obj_clear_flag(base_view->value_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(base_view->value_label, LV_OBJ_FLAG_EVENT_BUBBLE);
	}

	// Create gauge container
	base_view->gauge_container = lv_obj_create(parent);
	if (base_view->gauge_container) {
		// Configure gauge container
		lv_obj_set_size(base_view->gauge_container, container_width, gauge_height);

		// Ensure layout is ready for alignment
		lv_obj_update_layout(parent);

		// Try bottom-left alignment first
		lv_obj_align(base_view->gauge_container, LV_ALIGN_BOTTOM_LEFT, 0, 0);

		// Then center it horizontally
		lv_coord_t center_x = (container_width - container_width) / 2; // Should be 0 since gauge width = container width
		lv_obj_set_x(base_view->gauge_container, center_x);

		// Allow alignment to settle
		lv_obj_update_layout(parent);
		lv_obj_set_style_bg_opa(base_view->gauge_container, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(base_view->gauge_container, 0, 0);
		lv_obj_set_style_pad_all(base_view->gauge_container, 2, 0); // Reduced padding to match power grid
		lv_obj_clear_flag(base_view->gauge_container, LV_OBJ_FLAG_SCROLLABLE);
		// Make gauge_container clickable for touch events to bubble up
		lv_obj_add_flag(base_view->gauge_container, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(base_view->gauge_container, LV_OBJ_FLAG_EVENT_BUBBLE);

		// Initialize bar graph gauge

		memset(&base_view->gauge, 0, sizeof(bar_graph_gauge_t));


		bar_graph_gauge_init(
			&base_view->gauge, base_view->gauge_container,
			0, 0, container_width, gauge_height, 2, 3
		);


		// Check if gauge was initialized successfully
		if (!base_view->gauge.initialized) {

			return base_view;
		}

		// Configure gauge for voltage display using config values
		bar_graph_gauge_configure_advanced(
			&base_view->gauge, // gauge pointer
			config->bar_mode, // graph mode
			config->baseline_value, config->min_value, config->max_value, // bounds: baseline, min, max
			"", "", "", config->bar_graph_color, // title, unit, y-axis unit, color
			false, true, false // Show title, Show Y-axis, Show Border
		);

		// Update labels and ticks
		bar_graph_gauge_update_labels_and_ticks(&base_view->gauge);

	}

	// Store config for later use
	base_view->container = parent;
	base_view->number_config = config->number_config;
	base_view->initialized = true;

	return base_view;
}

void single_value_bar_graph_view_destroy(single_value_bar_graph_view_state_t* base_view)
{
	if (!base_view) return;

	memset(base_view, 0, sizeof(single_value_bar_graph_view_state_t));
	free(base_view);
}

void single_value_bar_graph_view_update_data(single_value_bar_graph_view_state_t* base_view, float value, bool has_error)
{
	if (!base_view || !base_view->initialized) return;

	// Update numerical value display (right-aligned)
	if (base_view->value_label) {

		char value_text[32];
		snprintf(value_text, sizeof(value_text), "%.1f", value);
		lv_label_set_text(base_view->value_label, value_text);

		// Set color based on error state
		if (has_error) {
			lv_obj_set_style_text_color(base_view->value_label, base_view->number_config.error_color, 0);
		} else {
			lv_obj_set_style_text_color(base_view->value_label, base_view->number_config.color, 0);
		}
	}

	// Data sampling and gauge updates are now handled centrally in power_monitor_update_all_gauge_histories()
	// This function only handles UI-specific updates for the single value bar graph view
}

void single_value_bar_graph_view_render(single_value_bar_graph_view_state_t* base_view)
{
	if (!base_view || !base_view->initialized) return;

	// Force layout update
	lv_obj_update_layout(base_view->container);
}

void single_value_bar_graph_view_apply_alert_flashing(single_value_bar_graph_view_state_t* base_view, float value, float low_threshold, float high_threshold, bool blink_on)
{
	if (!base_view || !base_view->value_label) return;

	if (blink_on && (value < low_threshold || value > high_threshold)) {

		lv_obj_set_style_text_color(base_view->value_label, PALETTE_YELLOW, 0);
	} else {

		lv_obj_set_style_text_color(base_view->value_label, PALETTE_WHITE, 0);
	}
}

void single_value_bar_graph_view_update_configuration(single_value_bar_graph_view_state_t* base_view, float baseline, float min_val, float max_val)
{
	if (!base_view || !base_view->gauge.initialized) return;

	// Update gauge configuration
	bar_graph_gauge_configure_advanced(
		&base_view->gauge, // gauge pointer
		base_view->gauge.mode, // graph mode
		baseline, min_val, max_val, // bounds: baseline, min, max
		"", "", "", base_view->gauge.bar_color, // title, unit, y-axis unit, color
		false, true, false // Show title, Show Y-axis, Show Border
	);

	bar_graph_gauge_update_labels_and_ticks(&base_view->gauge);
}