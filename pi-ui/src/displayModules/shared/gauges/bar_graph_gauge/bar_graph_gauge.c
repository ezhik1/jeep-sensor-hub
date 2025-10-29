#include <stdio.h>
#include "bar_graph_gauge.h"
#include "../../palette.h"
#include "../../../../app_data_store.h"
#include "../../../../../lvgl/src/misc/lv_text_private.h"
#include "../../utils/number_formatting/number_formatting.h"

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

static const char *TAG = "bar_graph_gauge";

// Forward declarations
static void bar_graph_gauge_shift_one_px(bar_graph_gauge_t *gauge);
static void bar_graph_gauge_tick_cb(lv_timer_t *timer);


void bar_graph_gauge_init(

	bar_graph_gauge_t *gauge,
	lv_obj_t *parent,
	int x,
	int y,
	int width,
	int height,
	int bar_width,
	int bar_gap
){
	if (!gauge || !parent) {
		printf("[E] bar_graph_gauge: Invalid parameters: gauge=%p, parent=%p\n", gauge, parent);
		return;
	}

	// Check if parent is valid
	if (!lv_obj_is_valid(parent)) {
		printf("[E] bar_graph_gauge: Parent object is not valid\n");
		return;
	}

	memset(gauge, 0, sizeof(bar_graph_gauge_t));

	gauge->parent = parent;
	gauge->x = x;
	gauge->y = y;
	gauge->width = width;
	gauge->height = height;
	gauge->show_title = true; // Default to true, can be overridden by configure_advanced
	gauge->show_y_axis = true; // Enable Y-axis by default for detail screen gauges
	gauge->show_border = false; // Default to no border, can be overridden by configure_advanced
	gauge->timeline_duration_ms = 1000; // Default one second timeline
	gauge->data_added = false; // No data added yet
	gauge->bar_width = bar_width;
	gauge->bar_gap = bar_gap;
	gauge->mode = BAR_GRAPH_MODE_POSITIVE_ONLY;
	gauge->baseline_value = 0.0f;
	gauge->init_min_value = 0.0f;
	gauge->init_max_value = 1.0f;
	gauge->range_values_changed = true; // Initial values need label update
	// gauge->canvas_padding = gauge->show_y_axis ? 20 : 0; // Y-axis labels container is 20px wide for canvas calculation
	gauge->cached_draw_height = gauge->height;

	gauge->bar_color = PALETTE_WHITE; // Default to WHITE

	// No local data storage - gauge renders from persistent history
	gauge->history_type = -1;  // Not linked to any history by default
	gauge->last_rendered_head = -1;  // Haven't rendered anything yet
	gauge->last_render_time_ms = 0;  // No render yet
	gauge->last_update_ms = 0;  // No update yet
	gauge->just_seeded = false;  // Not seeded yet

	// Smooth scrolling init
	gauge->scroll_offset_px = 0;
	gauge->prev_value = 0.0f;
	gauge->next_value = 0.0f;
	gauge->last_tick_ms = 0;
	gauge->pixels_per_second = 0.0f; // set later from timeline
	gauge->pixel_accumulator = 0.0f;
	// Default to a modest animation so it works out-of-the-box
	gauge->animation_duration_ms = 300;
	gauge->animation_cutover_ms = 100; // if updates faster than 150ms, jump full bar
	gauge->anim_start_ms = 0;
	gauge->anim_end_ms = 0;
	gauge->anim_pixels_moved = 0;
	gauge->animating = false;
	gauge->has_pending_sample = false;
	gauge->pending_value = 0.0f;
	gauge->anim_px_accum = 0.0f;
	gauge->anim_progress = 0.0f;
	gauge->bar_draw_value_valid = false;
	gauge->bar_draw_value = 0.0f;
	gauge->cutover_jump_active = false;

	// MAIN Gauge Container
	gauge->container = lv_obj_create(parent);

	if( width > 0 && height > 0 ){

		lv_obj_set_size(gauge->container, width, height);
	} else {

		lv_obj_set_size(gauge->container, LV_PCT(100), LV_PCT(100));
	}
	lv_obj_set_style_pad_all(gauge->container, 0, 0);
	lv_obj_set_style_bg_color(gauge->container, PALETTE_BLACK, 0);
	lv_obj_set_style_border_width(gauge->container, gauge->show_border ? 1 : 0, 0);
	lv_obj_set_style_border_color(gauge->container, gauge->show_border ? PALETTE_WHITE : PALETTE_BLACK, 0);

	lv_obj_set_style_radius(gauge->container, 0, 0); // No border radius
	lv_obj_add_flag(gauge->container, LV_OBJ_FLAG_CLICKABLE); // Make clickable for touch event bubbling
	lv_obj_add_flag(gauge->container, LV_OBJ_FLAG_EVENT_BUBBLE); // Allow events to bubble up
	lv_obj_clear_flag(gauge->container, LV_OBJ_FLAG_SCROLLABLE);

	// Flexbox layout
	lv_obj_set_layout(gauge->container, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(gauge->container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_style_flex_grow(gauge->container, 1, 0);
	lv_obj_set_flex_align(gauge->container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_gap(gauge->container, 0, 0); // NO gap between content and title
	lv_obj_set_style_pad_row(gauge->container, 0, 0); // NO row gap
	lv_obj_set_style_pad_column(gauge->container, 0, 0); // NO column gap

	// Content Container
	gauge->content_container = lv_obj_create(gauge->container);

	lv_obj_set_size(gauge->content_container, LV_PCT(99), gauge->show_title ? LV_PCT(90) : LV_PCT(98));

	lv_obj_set_style_bg_opa(gauge->content_container, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(gauge->content_container, PALETTE_BLACK, 0);
	lv_obj_set_style_border_width(gauge->content_container, 0, 0); // No border
	lv_obj_set_style_radius(gauge->content_container, 0, 0); // No border radius
	lv_obj_set_style_pad_all(gauge->content_container, 0, 0);

	// lv_obj_set_style_margin_top(gauge->content_container, 2, 0); // Move content down by 2px
	lv_obj_clear_flag(gauge->content_container, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(gauge->content_container, LV_OBJ_FLAG_EVENT_BUBBLE); // Allow events to bubble up
	lv_obj_clear_flag(gauge->content_container, LV_OBJ_FLAG_SCROLLABLE);

	// Flexbox layout
	lv_obj_set_layout(gauge->content_container, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(gauge->content_container, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(gauge->content_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_gap(gauge->content_container, 5, 0);

	// Create Y-axis labels container (left side)
	gauge->labels_container = lv_obj_create(gauge->content_container);

	lv_obj_set_size(gauge->labels_container, LV_SIZE_CONTENT, LV_PCT(100));

	// Style
	lv_obj_set_style_pad_all(gauge->labels_container, 0, 0);
	lv_obj_set_style_margin_top(gauge->labels_container, 0, 0);

	// Flexbox layout
	lv_obj_set_layout(gauge->labels_container, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(gauge->labels_container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(gauge->labels_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);

	// Style
	lv_obj_set_style_radius(gauge->labels_container, 0, 0); // No border

	// Create canvas container inside the wrapper
	gauge->canvas_container = lv_obj_create(gauge->content_container);

	lv_obj_set_size(gauge->canvas_container, LV_PCT(100), LV_PCT(100));

	// Style
	lv_obj_set_style_bg_color(gauge->canvas_container, PALETTE_BLACK, 0);
	lv_obj_set_style_border_width(gauge->canvas_container, 0, 0); // No border
	lv_obj_set_style_radius(gauge->canvas_container, 0, 0); // No border radius
	lv_obj_set_style_pad_all(gauge->canvas_container, 0, 0);
	lv_obj_set_style_margin_top(gauge->canvas_container, 0, 0);

	// Flexbox layout
	lv_obj_set_style_flex_grow(gauge->canvas_container, 1, 0);
	lv_obj_set_style_bg_opa(gauge->canvas_container, LV_OPA_COVER, 0);


	lv_obj_clear_flag(gauge->canvas_container, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(gauge->canvas_container, LV_OBJ_FLAG_EVENT_BUBBLE); // Allow events to bubble up
	lv_obj_clear_flag(gauge->canvas_container, LV_OBJ_FLAG_SCROLLABLE);


	gauge->canvas = lv_canvas_create(gauge->canvas_container);

	lv_obj_set_style_border_width(gauge->canvas, 0, 0); // No border
	lv_obj_set_style_radius(gauge->canvas, 0, 0); // No border radius

	// Ensure canvas is not clickable but allows event bubbling
	lv_obj_clear_flag(gauge->canvas, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(gauge->canvas, LV_OBJ_FLAG_EVENT_BUBBLE); // Allow events to bubble up

	gauge->initialized = true;

	// Animation Timer for Gauge Canvas
	if (!gauge->smooth_timer) {

		gauge->smooth_timer = lv_timer_create( bar_graph_gauge_tick_cb, 16, gauge ); // ~60Hz
	}


}

void bar_graph_gauge_configure_advanced(

	bar_graph_gauge_t *gauge,
	bar_graph_mode_t mode,
	float baseline_value,
	float min_val,
	float max_val,
	const char *title,
	const char *unit,
	const char *y_axis_unit,
	lv_color_t color,
	bool show_title,
	bool show_y_axis,
	bool show_border
){

	// Validate baseline is within min/max range for bipolar mode
	if (
		mode == BAR_GRAPH_MODE_BIPOLAR &&
		( baseline_value < min_val || baseline_value > max_val )
	){

		// Set baseline to middle of min/max range if outside bounds
		baseline_value = (min_val + max_val) / 2.0f;
	}

	if( gauge->width == 0 || gauge->height == 0 ){

		lv_obj_update_layout(gauge->container);
		gauge->width = lv_obj_get_width(gauge->container);
		gauge->height = lv_obj_get_height(gauge->container);
	}

	// Apply mode and ranges
	gauge->mode = mode;
	gauge->baseline_value = baseline_value;
	gauge->init_min_value = min_val;
	gauge->init_max_value = max_val;
	gauge->min_value = min_val;
	gauge->max_value = max_val;
	gauge->show_title = show_title;
	gauge->show_y_axis = show_y_axis;
	gauge->show_border = show_border;
	gauge->bar_color = color;

	// Cache the range for performance
	gauge->cached_range = gauge->max_value - gauge->min_value;

	// Update border styling
	if (gauge->show_border) {

		lv_obj_set_style_border_width(gauge->container, 1, 0);
		lv_obj_set_style_border_color(gauge->container, PALETTE_WHITE, 0);
		lv_obj_set_style_radius(gauge->container, 4, 0);

		lv_obj_set_size(gauge->labels_container, LV_SIZE_CONTENT, LV_PCT(99));
		lv_obj_set_size(gauge->canvas_container, LV_PCT(100), LV_PCT(99));
		lv_obj_set_style_margin_top(gauge->canvas_container, 4, 0);
		lv_obj_set_style_margin_top(gauge->labels_container, 4, 0);
	}

	// Create Y-axis labels if they don't exist but are now enabled
	if (gauge->show_y_axis) {

		lv_obj_set_style_flex_grow(gauge->labels_container, 0, 0);
		lv_obj_set_style_bg_opa(gauge->labels_container, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(gauge->labels_container, PALETTE_BLACK, 0);
		lv_obj_set_style_border_width(gauge->labels_container, 0, 0); // No border
		lv_obj_set_style_pad_all(gauge->labels_container, 0, 0);

		lv_obj_clear_flag(gauge->labels_container, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(gauge->labels_container, LV_OBJ_FLAG_SCROLLABLE);

		// Create Y-axis labels
		// Max Label
		gauge->max_label = lv_label_create(gauge->labels_container);
		lv_obj_set_style_text_font(gauge->max_label, &lv_font_montserrat_12, 0);
		lv_label_set_text(gauge->max_label, "1"); // DEBUG: Set initial text
		lv_obj_set_style_text_color(gauge->max_label, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->max_label, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(gauge->max_label, PALETTE_BLACK, 0);
		lv_obj_set_style_border_width(gauge->max_label, 0, 0); // No border
		lv_obj_set_style_radius(gauge->max_label, 0, 0); // No border radius
		lv_obj_clear_flag(gauge->max_label, LV_OBJ_FLAG_CLICKABLE);

		// Right align text for better negative value display
		lv_obj_set_style_text_align(gauge->max_label, LV_TEXT_ALIGN_RIGHT, 0);

		// Center Label
		gauge->center_label = lv_label_create(gauge->labels_container);
		lv_obj_set_style_text_font(gauge->center_label, &lv_font_montserrat_12, 0);
		lv_label_set_text(gauge->center_label, "0"); // DEBUG: Set initial text
		lv_obj_set_style_text_color(gauge->center_label, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->center_label, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(gauge->center_label, PALETTE_BLACK, 0);
		lv_obj_set_style_radius(gauge->center_label, 0, 0); // No border radius
		lv_obj_set_style_border_width(gauge->center_label, 0, 0);
		lv_obj_clear_flag(gauge->center_label, LV_OBJ_FLAG_CLICKABLE);

		// Right align text for better negative value display
		lv_obj_set_style_text_align(gauge->center_label, LV_TEXT_ALIGN_RIGHT, 0);

		// Min Label
		gauge->min_label = lv_label_create(gauge->labels_container);
		lv_obj_set_style_text_font(gauge->min_label, &lv_font_montserrat_12, 0);
		lv_label_set_text(gauge->min_label, "-1"); // DEBUG: Set initial text
		lv_obj_set_style_text_color(gauge->min_label, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->min_label, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(gauge->min_label, PALETTE_BLACK, 0);
		lv_obj_set_style_radius(gauge->min_label, 0, 0); // No border radius
		lv_obj_set_style_border_width(gauge->min_label, 0, 0);
		lv_obj_clear_flag(gauge->min_label, LV_OBJ_FLAG_CLICKABLE);
		// Right align text for better negative value display

		lv_obj_set_style_text_align(gauge->min_label, LV_TEXT_ALIGN_RIGHT, 0);
	}else{

		gauge->labels_container = NULL;
	}

	// Y-axis labels will be updated when range values change
	bar_graph_gauge_update_y_axis_labels(gauge);

	lv_obj_update_layout(gauge->canvas_container);

	// Size the canvas to match its container after layout
	int canvas_width = lv_obj_get_width(gauge->canvas_container);
	int canvas_height = lv_obj_get_height(gauge->canvas_container);

	gauge->cached_draw_width = canvas_width - 4;
	gauge->cached_draw_height = gauge->show_border ? canvas_height - 4 : canvas_height;

	lv_obj_set_size(gauge->canvas, canvas_width, gauge->cached_draw_height);
	lv_obj_align_to(gauge->canvas, gauge->canvas_container, LV_ALIGN_LEFT_MID, 0, 0);
	lv_obj_update_layout(gauge->canvas);

	// Now set up the canvas buffer with the correct size
	gauge->canvas_buffer = malloc(gauge->cached_draw_width * gauge->cached_draw_height * sizeof(lv_color_t));
	lv_canvas_set_buffer(
		gauge->canvas, gauge->canvas_buffer,
		gauge->cached_draw_width, gauge->cached_draw_height,
		LV_COLOR_FORMAT_RGB888
	);
	lv_canvas_fill_bg(gauge->canvas, PALETTE_BLACK, LV_OPA_COVER);

	lv_obj_update_layout(gauge->content_container);
	lv_obj_update_layout(gauge->labels_container);
	lv_obj_update_layout(gauge->parent);

	// Y-Axis Indicator Lines
	if (gauge->show_y_axis) {

		lv_obj_update_layout(gauge->canvas_container);

		int indicator_width = 1;
		int tick_width = 3;
		lv_coord_t canvas_height = lv_obj_get_height(gauge->canvas_container);

		// Vertical line - positioned at the left edge of the canvas
		gauge->indicator_vertical_line = lv_obj_create(gauge->content_container);
		lv_obj_set_size(gauge->indicator_vertical_line, indicator_width, canvas_height);
		lv_obj_set_style_bg_color(gauge->indicator_vertical_line, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->indicator_vertical_line, LV_OPA_COVER, 0);
		lv_obj_set_style_border_width(gauge->indicator_vertical_line, 0, 0);

		lv_obj_clear_flag(gauge->indicator_vertical_line, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(gauge->indicator_vertical_line, LV_OBJ_FLAG_IGNORE_LAYOUT);
		lv_obj_align_to(
			gauge->indicator_vertical_line, gauge->canvas_container,
			LV_ALIGN_OUT_LEFT_MID, 0, 0
		);
		lv_obj_add_flag(gauge->indicator_vertical_line, LV_OBJ_FLAG_IGNORE_LAYOUT);

		// Top horizontal line - positioned at the top of the canvas
		gauge->indicator_top_line = lv_obj_create(gauge->content_container);
		lv_obj_set_size(gauge->indicator_top_line, tick_width, indicator_width );
		lv_obj_set_style_bg_color(gauge->indicator_top_line, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->indicator_top_line, LV_OPA_COVER, 0);
		lv_obj_set_style_border_width(gauge->indicator_top_line, 0, 0);

		lv_obj_clear_flag(gauge->indicator_top_line, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(gauge->indicator_top_line, LV_OBJ_FLAG_IGNORE_LAYOUT);
		lv_obj_align_to(
			gauge->indicator_top_line, gauge->canvas_container,
			LV_ALIGN_OUT_TOP_LEFT, 0, +indicator_width
		);

		// Middle horizontal line - positioned at the middle of the canvas
		gauge->indicator_middle_line = lv_obj_create(gauge->content_container);
		lv_obj_set_size(gauge->indicator_middle_line, tick_width * 2, indicator_width );
		lv_obj_set_style_bg_color(gauge->indicator_middle_line, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->indicator_middle_line, LV_OPA_COVER, 0);
		lv_obj_set_style_border_width(gauge->indicator_middle_line, 0, 0);

		lv_obj_clear_flag(gauge->indicator_middle_line, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(gauge->indicator_middle_line, LV_OBJ_FLAG_IGNORE_LAYOUT);
		lv_obj_align_to(
			gauge->indicator_middle_line, gauge->canvas_container,
			LV_ALIGN_OUT_LEFT_MID, tick_width - 1, 0
		);

		// Bottom horizontal line - positioned at the bottom of the canvas
		gauge->indicator_bottom_line = lv_obj_create(gauge->content_container);
		lv_obj_set_size(gauge->indicator_bottom_line, tick_width, indicator_width );
		lv_obj_set_style_bg_color(gauge->indicator_bottom_line, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->indicator_bottom_line, LV_OPA_COVER, 0);
		lv_obj_set_style_border_width(gauge->indicator_bottom_line, 0, 0);

		lv_obj_clear_flag(gauge->indicator_bottom_line, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_add_flag(gauge->indicator_bottom_line, LV_OBJ_FLAG_IGNORE_LAYOUT);
		lv_obj_align_to(
			gauge->indicator_bottom_line, gauge->canvas_container,
			LV_ALIGN_OUT_BOTTOM_LEFT, 0, -indicator_width
		);
	} else {

		gauge->indicator_container = NULL;
		gauge->indicator_vertical_line = NULL;
		gauge->indicator_top_line = NULL;
		gauge->indicator_middle_line = NULL;
		gauge->indicator_bottom_line = NULL;
	}

	// Update title with unit
	if( gauge->show_title ){

		// Create as child of the gauge container to avoid conflicts with other gauges
		gauge->title_label = lv_label_create(gauge->parent);

		char title_with_unit[64];

		lv_obj_set_style_text_font(gauge->title_label, &lv_font_montserrat_12, 0);
		lv_label_set_text(gauge->title_label, "SOME VALUE"); // DEBUG: Set initial text
		lv_obj_set_style_text_color(gauge->title_label, PALETTE_WHITE, 0);
		lv_obj_set_style_text_align(gauge->title_label, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_set_style_bg_color(gauge->title_label, PALETTE_BLACK, 0); // Black background to obscure border
		lv_obj_set_style_bg_opa(gauge->title_label, LV_OPA_COVER, 0);
		lv_obj_set_style_pad_left(gauge->title_label, 8, 0);
		lv_obj_set_style_pad_right(gauge->title_label, 8, 0);
		lv_obj_set_style_pad_top(gauge->title_label, 1, 0);
		lv_obj_set_style_pad_bottom(gauge->title_label, 1, 0);
		lv_obj_set_style_border_width(gauge->title_label, 0, 0); // No border
		lv_obj_clear_flag(gauge->title_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(gauge->title_label, LV_OBJ_FLAG_SCROLLABLE);

		if (unit) {
			snprintf(title_with_unit, sizeof(title_with_unit), "%s (%s)", title, unit);
		} else {
			strncpy(title_with_unit, title, sizeof(title_with_unit) - 1);
			title_with_unit[sizeof(title_with_unit) - 1] = '\0';
		}

		lv_label_set_text(gauge->title_label, title_with_unit);

		// Position relative to the gauge container (bottom-right corner)
		lv_obj_align_to(gauge->title_label, gauge->container, LV_ALIGN_BOTTOM_RIGHT, -20, 10);

		// Move to foreground to ensure visibility
		lv_obj_move_foreground(gauge->title_label);
	}
}


// Force complete current animation by fast-forwarding to end state
void bar_graph_gauge_force_complete_animation(bar_graph_gauge_t *gauge)
{
	if (!gauge || !gauge->animating) return;

	// Calculate how many pixels we need to advance to complete the animation
	int bar_spacing = gauge->bar_width + gauge->bar_gap;
	int remaining_px = bar_spacing - gauge->scroll_offset_px;

	if (remaining_px > 0) {
		// Fast-forward by completing all remaining pixel shifts
		for (int step = 0; step < remaining_px; step++) {
			bar_graph_gauge_shift_one_px(gauge);
		}
	}

	// Complete the animation state
	gauge->animating = false;
	gauge->has_pending_sample = false;
	gauge->anim_progress = 1.0f;
	gauge->bar_draw_value_valid = false;

	// Update last_rendered_head to reflect the completed animation
	if (gauge->history_type >= 0) {
		app_data_store_t* store = app_data_store_get();
		if (store && gauge->history_type < POWER_MONITOR_GAUGE_COUNT) {
			persistent_gauge_history_t* gauge_data_history = &store->power_monitor_gauge_histories[gauge->history_type];
			gauge->last_rendered_head = gauge_data_history->head;
		}
	}
}

static void bar_graph_gauge_shift_one_px(bar_graph_gauge_t *gauge)
{
	int canvas_width = gauge->cached_draw_width;
	int top_y = 2;
	int bottom_y = gauge->cached_draw_height - 5;
	int h = bottom_y - top_y + 1;
	int bar_area_width = canvas_width;

	// shift left by 1 pixel across visible rows
	for (int row = 0; row < h; row++) {
		int actual_row = top_y + row;
		memmove(
			&gauge->canvas_buffer[actual_row * canvas_width],
			&gauge->canvas_buffer[actual_row * canvas_width + 1],
			(bar_area_width - 1) * sizeof(lv_color_t)
		);
		// Clear rightmost pixel
		gauge->canvas_buffer[actual_row * canvas_width + (bar_area_width - 1)] = PALETTE_BLACK;
	}

	// Debug: show current scroll state before drawing
	// logging removed

	// Draw rightmost column using time-based easing per bar: constant height for all columns in this bar
	int x = bar_area_width - 1; // rightmost column index
	int bar_start = gauge->bar_gap;
	int bar_end = gauge->bar_gap + gauge->bar_width; // exclusive
	if (gauge->scroll_offset_px >= bar_start && gauge->scroll_offset_px < bar_end) {
		// First column of this bar span: compute and cache a uniform value from time-based easing
		if (!gauge->bar_draw_value_valid || gauge->scroll_offset_px == bar_start) {
			float t_time = gauge->anim_progress;
			if (t_time < 0.0f) t_time = 0.0f; if (t_time > 1.0f) t_time = 1.0f;
			float v = gauge->prev_value + (gauge->next_value - gauge->prev_value) * t_time;
			// Clamp
			if (v < gauge->init_min_value) v = gauge->init_min_value;
			if (v > gauge->init_max_value) v = gauge->init_max_value;
			gauge->bar_draw_value = v;
			gauge->bar_draw_value_valid = true;
			// logging removed
		}

		float val = gauge->bar_draw_value;

		// Clamp to visible range
		if (val < gauge->init_min_value) val = gauge->init_min_value;
		if (val > gauge->init_max_value) val = gauge->init_max_value;

		int baseline_y;
		float scale = 1.0f;
		float scale_min = 1.0f, scale_max = 1.0f;
		if (gauge->mode == BAR_GRAPH_MODE_BIPOLAR) {
			float dist_min = gauge->baseline_value - gauge->init_min_value;
			float dist_max = gauge->init_max_value - gauge->baseline_value;
			scale_min = (dist_min > 0) ? (float)(h - 2) / (2.0f * dist_min) : 1.0f;
			scale_max = (dist_max > 0) ? (float)(h - 2) / (2.0f * dist_max) : 1.0f;
			baseline_y = h / 2;
		} else {
			float range = gauge->init_max_value - gauge->init_min_value;
			scale = (float)(h - 2) / (range > 0 ? range : 1.0f);
			baseline_y = h - 1;
		}

		int y1, y2;
		if (gauge->mode == BAR_GRAPH_MODE_POSITIVE_ONLY) {
			int bar_height = (int)((val - gauge->init_min_value) * scale);
			y1 = h - bar_height;
			y2 = h;
		} else {
			if (val >= gauge->baseline_value) {
				int bar_height = (int)((val - gauge->baseline_value) * scale_max);
				y1 = baseline_y - bar_height;
				y2 = baseline_y;
			} else {
				int bar_height = (int)((gauge->baseline_value - val) * scale_min);
				y1 = baseline_y;
				y2 = baseline_y + bar_height;
			}
		}

		// Detailed diagnostics for skewed height investigation
		// logging removed

		int y_start = top_y + y1;
		int y_end = top_y + y2;
		if (y_start < top_y) y_start = top_y;
		if (y_end > top_y + h) y_end = top_y + h;
		// logging removed
		for (int yy = y_start; yy < y_end; yy++) {
			gauge->canvas_buffer[yy * canvas_width + x] = gauge->bar_color;
		}
		// If we just drew the last column of the bar span, clear cache for next bar
		if (gauge->scroll_offset_px + 1 >= bar_end) {
			gauge->bar_draw_value_valid = false;
		}
	}

	gauge->scroll_offset_px++;
}

static void bar_graph_gauge_tick_cb(lv_timer_t *timer)
{
	bar_graph_gauge_t *gauge = (bar_graph_gauge_t*)lv_timer_get_user_data(timer);
	if (!gauge || !gauge->initialized) return;

	// Safety: ensure LVGL objects and buffers are valid before any canvas ops
	if (!gauge->container || !lv_obj_is_valid(gauge->container) ||
		!gauge->canvas_container || !lv_obj_is_valid(gauge->canvas_container) ||
		!gauge->canvas || !lv_obj_is_valid(gauge->canvas) ||
		!gauge->canvas_buffer) {
		if (gauge->smooth_timer) {
			lv_timer_del(gauge->smooth_timer);
			gauge->smooth_timer = NULL;
		}
		gauge->animating = false;
		return;
	}

	// Discrete animation mode: advance proportional to elapsed time in this animation
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint32_t now_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	uint32_t elapsed_ms = (gauge->last_tick_ms == 0) ? 0 : (now_ms - gauge->last_tick_ms);
	gauge->last_tick_ms = now_ms;

	if (!gauge->animating || gauge->animation_duration_ms == 0) {
		return;
	}

	int bar_spacing = gauge->bar_width + gauge->bar_gap;
	uint32_t anim_total = gauge->animation_duration_ms;
	uint32_t anim_elapsed = (now_ms > gauge->anim_start_ms) ? (now_ms - gauge->anim_start_ms) : 0;
	if (anim_elapsed > anim_total) anim_elapsed = anim_total;

	// Accumulate fractional pixel progress based on time slice
	float step_px = ((float)bar_spacing) * ((float)elapsed_ms / (float)anim_total);
	gauge->anim_px_accum += step_px;
	int advance_px = (int)gauge->anim_px_accum;
	if (advance_px <= 0) {
		// logging removed
		return;
	}
	gauge->anim_px_accum -= (float)advance_px;

	// Do not attempt to move past the end of this animation
	int remaining_px = bar_spacing - gauge->scroll_offset_px;
	if (remaining_px <= 0) {
		// finalize below
		advance_px = 0;
	} else if (advance_px > remaining_px) {
		advance_px = remaining_px;
	}

	// logging removed

	for (int step = 0; step < advance_px; step++) {
		if (gauge->scroll_offset_px >= bar_spacing) break; // safety
		bar_graph_gauge_shift_one_px(gauge);
		gauge->anim_pixels_moved++;
	}

	if (gauge->scroll_offset_px >= bar_spacing) {
		gauge->animating = false;
		gauge->has_pending_sample = false;

		// Update last_rendered_head to reflect the new data
		if (gauge->history_type >= 0) {
			app_data_store_t* store = app_data_store_get();
		if (store && gauge->history_type < POWER_MONITOR_GAUGE_COUNT) {
			persistent_gauge_history_t* gauge_data_history = &store->power_monitor_gauge_histories[gauge->history_type];
			gauge->last_rendered_head = gauge_data_history->head;
		}
		}
	}
}

// Set which persistent history this gauge should render from
void bar_graph_gauge_set_history_type(bar_graph_gauge_t *gauge, int history_type)
{
	if (!gauge) return;
	gauge->history_type = history_type;
}

void bar_graph_gauge_add_data_point(bar_graph_gauge_t *gauge, void* gauge_data_history_ptr)
{
	// Safety check: don't access uninitialized gauge
	if (!gauge || !gauge->initialized) return;

	// Safety check: don't access null history
	if (!gauge_data_history_ptr) {
		printf("[W] bar_graph_gauge: add_data_point called with null history\n");
		return;
	}

	// Cast to the actual type
	persistent_gauge_history_t* gauge_data_history = (persistent_gauge_history_t*)gauge_data_history_ptr;


	// Calculate how many new samples based on head movement
	int new_samples = 0;
	if (gauge->last_rendered_head == -1) {
		// First render - draw everything
		new_samples = gauge_data_history->max_count;
	} else {
		// Calculate circular distance from last rendered head
		new_samples = (gauge_data_history->head - gauge->last_rendered_head + gauge_data_history->max_count) % gauge_data_history->max_count;
	}

	// First render: draw all historical data
	if (gauge->last_rendered_head == -1) {

		// last_rendered_head is set by draw_all_data
		bar_graph_gauge_draw_all_data(gauge, gauge_data_history);
		return;
	}

	// Handle new samples with animation
	if (new_samples > 0) {

		// If we're currently animating, fast-forward to completion before processing new data
		if (gauge->animating) {
			bar_graph_gauge_force_complete_animation(gauge);
		}

		// Get the latest value from history for animation
		float latest_value = gauge_data_history->values[ gauge_data_history->head ];
		if( latest_value < gauge->init_min_value) latest_value = gauge->init_min_value;
		if( latest_value > gauge->init_max_value) latest_value = gauge->init_max_value;

		// Store previous value for smooth transition
		gauge->prev_value = gauge->next_value;
		gauge->next_value = latest_value;

		// Check if we should use cutover jump (immediate shift) or smooth animation
		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		uint32_t now_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
		uint32_t since_last_ms = (gauge->last_update_ms == 0) ? 0 : (now_ms - gauge->last_update_ms);
		gauge->last_update_ms = now_ms;

		bool per_sample_cutover = (since_last_ms <= gauge->animation_cutover_ms);

		if (per_sample_cutover || gauge->animation_duration_ms == 0) {

			// Immediate shift - no animation
			int bar_spacing = gauge->bar_width + gauge->bar_gap;
			int canvas_width = gauge->cached_draw_width;
			int top_y = 2;
			int bottom_y = gauge->cached_draw_height - 5;
			int h = bottom_y - top_y + 1;
			int shift_amount = bar_spacing * new_samples;

			// Shift canvas left
			for (int row = 0; row < h; row++) {

				int actual_row = top_y + row;

				if (shift_amount < canvas_width) {

					memmove(
						&gauge->canvas_buffer[actual_row * canvas_width],
						&gauge->canvas_buffer[actual_row * canvas_width + shift_amount],
						(canvas_width - shift_amount) * sizeof(lv_color_t)
					);

					// Clear the rightmost area
					memset(
						&gauge->canvas_buffer[actual_row * canvas_width + (canvas_width - shift_amount)],
						0,
						shift_amount * sizeof(lv_color_t)
					);
				} else {

					// Shift amount >= canvas width, just clear entire row
					memset(&gauge->canvas_buffer[actual_row * canvas_width], 0, canvas_width * sizeof(lv_color_t));
				}
			}

			// Draw the new bars on the right
			for (int i = 0; i < new_samples; i++) {

				int offset = new_samples - 1 - i;
				int hist_index = (gauge_data_history->head - offset + gauge_data_history->max_count) % gauge_data_history->max_count;
				float val = gauge_data_history->values[hist_index];
				if (val < gauge->init_min_value) val = gauge->init_min_value;
				if (val > gauge->init_max_value) val = gauge->init_max_value;

				// Calculate bar position (from right edge)
				int x_start = canvas_width - gauge->bar_width - (i * bar_spacing);
				int x_end = x_start + gauge->bar_width;
				if (x_start < 0 || x_start >= canvas_width) continue;
				if (x_end > canvas_width) x_end = canvas_width;

				// Calculate bar height
				int baseline_y, y1, y2;
				float scale, scale_min = 1.0f, scale_max = 1.0f;

				if (gauge->mode == BAR_GRAPH_MODE_BIPOLAR) {

					float dist_min = gauge->baseline_value - gauge->init_min_value;
					float dist_max = gauge->init_max_value - gauge->baseline_value;
					scale_min = (dist_min > 0) ? (float)(h - 2) / (2.0f * dist_min) : 1.0f;
					scale_max = (dist_max > 0) ? (float)(h - 2) / (2.0f * dist_max) : 1.0f;
					baseline_y = h / 2;
				} else {

					float range = gauge->init_max_value - gauge->init_min_value;
					scale = (float)(h - 2) / (range > 0 ? range : 1.0f);
					baseline_y = h - 1;
				}

				if (gauge->mode == BAR_GRAPH_MODE_POSITIVE_ONLY) {

					int bar_height = (int)((val - gauge->init_min_value) * scale);
					y1 = h - bar_height;
					y2 = h;
				} else {

					if (val >= gauge->baseline_value) {

						int bar_height = (int)((val - gauge->baseline_value) * scale_max);
						y1 = baseline_y - bar_height;
						y2 = baseline_y;
					} else {

						int bar_height = (int)((gauge->baseline_value - val) * scale_min);
						y1 = baseline_y;
						y2 = baseline_y + bar_height;
					}
				}

				// Draw the bar
				int y_start = top_y + y1;
				int y_end = top_y + y2;
				if (y_start < top_y) y_start = top_y;
				if (y_end > top_y + h) y_end = top_y + h;
				if (y_end > y_start) {

					lv_color_t color = gauge->bar_color;

					for (int yy = y_start; yy < y_end; yy++) {

						lv_color_t *row_ptr = &gauge->canvas_buffer[yy * canvas_width];

						for (int xx = x_start; xx < x_end; xx++) {

							row_ptr[xx] = color;
						}
					}
				}
			}

			gauge->last_rendered_head = gauge_data_history->head;
			gauge->data_added = true;
		} else {

			// Start smooth animation
			gauge->animating = true;
			gauge->has_pending_sample = true;
			gauge->pending_value = latest_value;
			gauge->anim_start_ms = now_ms;
			gauge->anim_end_ms = now_ms + gauge->animation_duration_ms;
			gauge->scroll_offset_px = 0;
			gauge->anim_px_accum = 0.0f;
			gauge->anim_pixels_moved = 0;
			gauge->bar_draw_value_valid = false;
			gauge->anim_progress = 0.0f;
		}
	}

}

void bar_graph_gauge_update_y_axis_labels(bar_graph_gauge_t *gauge)
{

	// Create label text (without the '-' since we're using range rectangles now)
	char max_text[16], center_text[16], min_text[16];

	if (gauge->mode == BAR_GRAPH_MODE_BIPOLAR) {
		// For bipolar mode, show max, baseline, and min values
		format_value_with_magnitude(gauge->init_max_value, max_text, sizeof(max_text));
		format_value_with_magnitude(gauge->baseline_value, center_text, sizeof(center_text));
		format_value_with_magnitude(gauge->init_min_value, min_text, sizeof(min_text));
	} else {
		// For positive-only mode, show max, middle, and min values
		float middle_value = (gauge->init_min_value + gauge->init_max_value) / 2.0f;
		format_value_with_magnitude(gauge->init_max_value, max_text, sizeof(max_text));
		format_value_with_magnitude(middle_value, center_text, sizeof(center_text));
		format_value_with_magnitude(gauge->init_min_value, min_text, sizeof(min_text));
	}

	if( gauge->show_y_axis ) {
		lv_coord_t max_width = 0;
		lv_coord_t width;
		lv_coord_t height = lv_font_get_line_height(&lv_font_montserrat_12);

		// Set label text (flexbox will handle positioning) - only if labels exist
		if (gauge->max_label) lv_label_set_text(gauge->max_label, max_text);
		if (gauge->center_label) lv_label_set_text(gauge->center_label, center_text);
		if (gauge->min_label) lv_label_set_text(gauge->min_label, min_text);

		// Get text from labels and calculate width
		const char* max_text_ptr = lv_label_get_text(gauge->max_label);
		if (max_text_ptr) {
			lv_text_attributes_t attr = {0};
			width = lv_text_get_width(max_text_ptr, strlen(max_text_ptr), &lv_font_montserrat_12, &attr);
			if (width > max_width) max_width = width;
		}

		const char* center_text_ptr = lv_label_get_text(gauge->center_label);
		if (center_text_ptr) {
			lv_text_attributes_t attr = {0};
			width = lv_text_get_width(center_text_ptr, strlen(center_text_ptr), &lv_font_montserrat_12, &attr);
			if (width > max_width) max_width = width;
		}

		const char* min_text_ptr = lv_label_get_text(gauge->min_label);
		if (min_text_ptr) {
			lv_text_attributes_t attr = {0};
			width = lv_text_get_width(min_text_ptr, strlen(min_text_ptr), &lv_font_montserrat_12, &attr);
			if (width > max_width) max_width = width;
		}

		/* Step 2: Give all labels that same width */
		lv_obj_set_width(gauge->max_label, max_width);
		lv_obj_set_width(gauge->center_label, max_width);
		lv_obj_set_width(gauge->min_label, max_width);

		// lv_obj_set_height(gauge->max_label, height);
		// lv_obj_set_height(gauge->center_label, height);
		// lv_obj_set_height(gauge->min_label, height);

		// lv_obj_set_style_translate_y(gauge->max_label, -height / 4, 0);
		// lv_obj_set_style_translate_y(gauge->center_label, -height / 4, 0);
		// lv_obj_set_style_translate_y(gauge->min_label, -height / 4, 0);


		lv_obj_update_layout(gauge->labels_container);
	}

	// Mark that labels have been updated
	gauge->range_values_changed = false;
}

// Draw all historical data to canvas at once using provided head snapshot
void bar_graph_gauge_draw_all_data_snapshot(bar_graph_gauge_t *gauge, int head_snapshot, void* gauge_data_history_ptr)
{
	if (!gauge || !gauge->initialized || !gauge->canvas_buffer) return;

	// Use the provided history
	persistent_gauge_history_t* gauge_data_history = NULL;
	if (gauge_data_history_ptr) {
		gauge_data_history = (persistent_gauge_history_t*)gauge_data_history_ptr;
	}

	// If no persistent history available or no real data, nothing to draw
	if (!gauge_data_history) {
		// Clear canvas
		int canvas_width = gauge->cached_draw_width;
		int top_y = 2;
		int bottom_y = gauge->cached_draw_height - 5;
		int h = bottom_y - top_y + 1;
		for (int row = 0; row < h; row++) {
			int actual_row = top_y + row;
			memset(&gauge->canvas_buffer[actual_row * canvas_width], 0, canvas_width * sizeof(lv_color_t));
		}
		return;
	}

	if (!gauge_data_history->has_real_data) {
		// Clear canvas
		int canvas_width = gauge->cached_draw_width;
		int top_y = 2;
		int bottom_y = gauge->cached_draw_height - 5;
		int h = bottom_y - top_y + 1;
		for (int row = 0; row < h; row++) {
			int actual_row = top_y + row;
			memset(&gauge->canvas_buffer[actual_row * canvas_width], 0, canvas_width * sizeof(lv_color_t));
		}
		return;
	}

	// Use the actual canvas width for buffer operations
	int canvas_width = gauge->cached_draw_width;
	// Adjust drawing area to match L shape boundaries
	int top_y = 2; // Match L shape top line
	int bottom_y = gauge->cached_draw_height - 5; // Match L shape bottom line
	int h = bottom_y - top_y + 1; // Effective drawing height between L shape lines
	int bar_spacing = (gauge->bar_width + gauge->bar_gap);

	// Calculate how many bars actually fit in the canvas width
	int max_bars_that_fit = canvas_width / bar_spacing;
	// Count how many real data points we have
	int real_data_count = 0;
	for (int i = 0; i < gauge_data_history->max_count; i++) {
		if (!isnan(gauge_data_history->values[i])) {
			real_data_count++;
		}
	}
	// Only draw as many bars as we have real data for
	int actual_bars_to_draw = (real_data_count < max_bars_that_fit) ? real_data_count : max_bars_that_fit;

	// Clear the entire canvas first
	for (int row = 0; row < h; row++) {
		int actual_row = top_y + row;
		memset(&gauge->canvas_buffer[actual_row * canvas_width], 0, canvas_width * sizeof(lv_color_t));
	}

	// Draw all bars from left to right (most recent data on the right)
	// printf("[D] bar_graph_gauge: Drawing %d bars, head_snapshot=%d\n", actual_bars_to_draw, head_snapshot);
	for (int bar_index = 0; bar_index < actual_bars_to_draw; bar_index++) {
		// Calculate which history entry to use from ring buffer
		// Walk backwards from SNAPSHOT head to get the most recent N bars
		int offset = actual_bars_to_draw - 1 - bar_index;
		int hist_index = (head_snapshot - offset + gauge_data_history->max_count) % gauge_data_history->max_count;
		float val = gauge_data_history->values[hist_index];

		// printf("[D] bar_graph_gauge: bar %d: hist_index=%d, val=%.2f\n", bar_index, hist_index, val);

		// Skip drawing if value is NaN (empty/uninitialized)
		if (isnan(val)) {
			// printf("[D] bar_graph_gauge: Skipping bar %d - NaN value\n", bar_index);
			continue;
		}

		// Clamp value to the visible range
		if (val < gauge->init_min_value) val = gauge->init_min_value;
		if (val > gauge->init_max_value) val = gauge->init_max_value;

		// Calculate bar position (from right edge)
		int x_start = canvas_width - gauge->bar_width - (bar_index * bar_spacing);
		int x_end = x_start + gauge->bar_width;
		if (x_start >= canvas_width) break; // Don't draw beyond canvas
		if (x_end > canvas_width) x_end = canvas_width;


		// Calculate bar height based on mode
	int baseline_y;
	float scale;
	float scale_min = 1.0f;
	float scale_max = 1.0f;

	if (gauge->mode == BAR_GRAPH_MODE_BIPOLAR) {
		float dist_min = gauge->baseline_value - gauge->init_min_value;
		float dist_max = gauge->init_max_value - gauge->baseline_value;
		scale_min = (dist_min > 0) ? (float)(h - 2) / (2.0f * dist_min) : 1.0f;
		scale_max = (dist_max > 0) ? (float)(h - 2) / (2.0f * dist_max) : 1.0f;
			baseline_y = h / 2;
	} else {
		float range = gauge->init_max_value - gauge->init_min_value;
			scale = (float)(h - 2) / (range > 0 ? range : 1.0f);
		baseline_y = h - 1;
	}

	int y1, y2;
	if (gauge->mode == BAR_GRAPH_MODE_POSITIVE_ONLY) {
		int bar_height = (int)((val - gauge->init_min_value) * scale);
		y1 = h - bar_height;
		y2 = h;
	} else {
		if (val >= gauge->baseline_value) {
			int bar_height = (int)((val - gauge->baseline_value) * scale_max);
			y1 = baseline_y - bar_height;
			y2 = baseline_y;
		} else {
			int bar_height = (int)((gauge->baseline_value - val) * scale_min);
			y1 = baseline_y;
			y2 = baseline_y + bar_height;
		}
	}

		// Draw the bar
	int y_start = top_y + y1;
		int y_end = top_y + y2;
	if (y_start < top_y) y_start = top_y;
	if (y_end > top_y + h) y_end = top_y + h;
	if (y_end > y_start) {
		lv_color_t color = gauge->bar_color;
		for (int yy = y_start; yy < y_end; yy++) {
			lv_color_t *row_ptr = &gauge->canvas_buffer[yy * canvas_width];
			for (int xx = x_start; xx < x_end; xx++) {
				row_ptr[xx] = color;
			}
		}
	}
	}
}

// Draw all historical data
void bar_graph_gauge_draw_all_data(bar_graph_gauge_t *gauge, void* gauge_data_history_ptr)
{
	if (!gauge || !gauge->initialized) return;

	// Snapshot head ONCE before drawing
	int head_at_draw_start = -1;
	if (gauge_data_history_ptr) {
		persistent_gauge_history_t* gauge_data_history = (persistent_gauge_history_t*)gauge_data_history_ptr;
		head_at_draw_start = gauge_data_history->head;
	}

	// Draw using the snapshot we took
	bar_graph_gauge_draw_all_data_snapshot(gauge, head_at_draw_start, gauge_data_history_ptr);

	// Set tracking to the head we captured BEFORE drawing
	gauge->last_rendered_head = head_at_draw_start;
}

void bar_graph_gauge_cleanup(bar_graph_gauge_t *gauge)
{
	if (!gauge) return;
	if (!gauge->initialized) return;

	// Mark as not initialized to prevent double cleanup
	gauge->initialized = false;

	// Delete timer FIRST to prevent ticks during teardown
	if (gauge->smooth_timer) {
		lv_timer_del(gauge->smooth_timer);
		gauge->smooth_timer = NULL;
	}

	// Free canvas buffer if it exists
	if (gauge->canvas_buffer) {

		free(gauge->canvas_buffer);
		gauge->canvas_buffer = NULL;
	}

	// Delete LVGL objects to prevent memory leaks
	if (gauge->container && lv_obj_is_valid(gauge->container)) {

		lv_obj_del(gauge->container);
	}

	// Reset all pointers to NULL
	gauge->container = NULL;
	gauge->canvas = NULL;
	gauge->content_container = NULL;
	gauge->labels_container = NULL;
	gauge->canvas_container = NULL;
	gauge->title_label = NULL;
	gauge->max_label = NULL;
	gauge->center_label = NULL;
	gauge->min_label = NULL;
	gauge->initialized = false;
}

void bar_graph_gauge_set_animation_duration(bar_graph_gauge_t *gauge, uint32_t duration_ms)
{
	if (!gauge) return;
	gauge->animation_duration_ms = duration_ms;
	// logging removed
}

void bar_graph_gauge_set_timeline_duration(bar_graph_gauge_t *gauge, uint32_t duration_ms)
{
	if (!gauge) return;

	gauge->timeline_duration_ms = duration_ms;

	// Derive pixels per second so that a full bar spacing traverses in one data interval
	int canvas_width = gauge->cached_draw_width > 0 ? gauge->cached_draw_width : (gauge->width - (gauge->show_y_axis ? 22 : 0));
	int bar_spacing = gauge->bar_width + gauge->bar_gap;
	int total_bars = (bar_spacing > 0) ? (canvas_width / bar_spacing) : 0;
	if (duration_ms > 0 && total_bars > 0) {
		float data_interval_ms = (float)duration_ms / (float)total_bars;
		gauge->pixels_per_second = (float)bar_spacing / (data_interval_ms / 1000.0f);
		if (gauge->pixels_per_second < 1.0f) gauge->pixels_per_second = 1.0f;
		if (gauge->pixels_per_second > 1000.0f) gauge->pixels_per_second = 1000.0f;
		// Cutover: if updates are very frequent, mark for full-distance jump per sample
		gauge->cutover_jump_active = ((uint32_t)data_interval_ms <= gauge->animation_cutover_ms);
	}
	else {
		// For discrete animation mode, pixels_per_second is unused; keep a sane default
		gauge->pixels_per_second = (float)(bar_spacing) * 60.0f;
	}

}
