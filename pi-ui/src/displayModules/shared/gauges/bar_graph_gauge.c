#include <stdio.h>
#include "bar_graph_gauge.h"
#include "../palette.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static const char *TAG = "bar_graph_gauge";

void bar_graph_gauge_init(
	bar_graph_gauge_t *gauge,
	lv_obj_t *parent,
	int x,
	int y,
	int width,
	int height,
	int bar_width,
	int bar_gap
)
{
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

	gauge->x = x;
	gauge->y = y;
	gauge->width = width;
	gauge->height = height;
	gauge->show_title = true; // Default to true, can be overridden by configure_advanced
	gauge->show_y_axis = true; // Enable Y-axis by default for detail screen gauges
	gauge->show_border = false; // Default to no border, can be overridden by configure_advanced
	gauge->update_interval_ms = 0; // No rate limiting
	gauge->timeline_duration_ms = 1000; // Default one second timeline
	gauge->data_added = false; // No data added yet
	gauge->bar_width = bar_width;
	gauge->bar_gap = bar_gap;
	gauge->mode = BAR_GRAPH_MODE_POSITIVE_ONLY;
	gauge->baseline_value = 0.0f;
	gauge->init_min_value = 0.0f;
	gauge->init_max_value = 1.0f;
	gauge->range_values_changed = true; // Initial values need label update
	gauge->canvas_padding = gauge->show_y_axis ? 20 : 0; // Y-axis labels container is 20px wide for canvas calculation

	gauge->bar_color = PALETTE_WHITE; // Default to WHITE
	gauge->max_data_points = (gauge->width - gauge->canvas_padding) / (gauge->bar_width + gauge->bar_gap);

	if (gauge->max_data_points < 5) gauge->max_data_points = 5;
	if (gauge->max_data_points > 200) gauge->max_data_points = 200;

	// Data points calculated
	gauge->data_points = malloc(gauge->max_data_points * sizeof(float));

	memset(gauge->data_points, 0, gauge->max_data_points * sizeof(float));
	gauge->head = -1;

	// MAIN Gauge Container
	gauge->container = lv_obj_create(parent);
	lv_obj_set_size(gauge->container, width, height);

	lv_obj_set_style_pad_all(gauge->container, 0, 0);
	lv_obj_set_style_pad_left(gauge->container, 0, 0);
	lv_obj_set_style_pad_right(gauge->container, 0, 0);
	lv_obj_set_style_pad_top(gauge->container, 0, 0);
	lv_obj_set_style_pad_bottom(gauge->container, 0, 0);
	lv_obj_set_style_bg_color(gauge->container, PALETTE_BLACK, 0);
	lv_obj_set_style_border_width(gauge->container, gauge->show_border ? 1 : 0, 0);
	lv_obj_set_style_border_color(gauge->container, gauge->show_border ? PALETTE_WHITE : PALETTE_BLACK, 0);

	lv_obj_set_style_radius(gauge->container, 0, 0); // No border radius
	lv_obj_add_flag(gauge->container, LV_OBJ_FLAG_CLICKABLE); // Make clickable for touch event bubbling
	lv_obj_add_flag(gauge->container, LV_OBJ_FLAG_EVENT_BUBBLE); // Allow events to bubble up
	lv_obj_clear_flag(gauge->container, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_set_flex_flow(gauge->container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(gauge->container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_gap(gauge->container, 0, 0); // NO gap between content and title
	lv_obj_set_style_pad_row(gauge->container, 0, 0); // NO row gap
	lv_obj_set_style_pad_column(gauge->container, 0, 0); // NO column gap

	gauge->content_container = lv_obj_create(gauge->container);

	int content_height = gauge->height - 15;
	lv_obj_set_size(gauge->content_container, gauge->width, content_height);

	lv_obj_set_style_bg_opa(gauge->content_container, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(gauge->content_container, PALETTE_BLACK, 0);
	lv_obj_set_style_border_width(gauge->content_container, 0, 0); // No border
	lv_obj_set_style_pad_all(gauge->content_container, 0, 0);
	lv_obj_set_style_pad_left(gauge->content_container, 0, 0);
	lv_obj_set_style_pad_right(gauge->content_container, 0, 0);
	lv_obj_set_style_pad_top(gauge->content_container, 0, 0);
	lv_obj_set_style_pad_bottom(gauge->content_container, 0, 0);
	lv_obj_set_style_margin_top(gauge->content_container, 4, 0); // Move content down by 2px
	lv_obj_clear_flag(gauge->content_container, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(gauge->content_container, LV_OBJ_FLAG_EVENT_BUBBLE); // Allow events to bubble up
	lv_obj_clear_flag(gauge->content_container, LV_OBJ_FLAG_SCROLLABLE);

	// Create Y-axis labels container (left side)
	if (gauge->show_y_axis) {
		// Safety check: ensure content_container is valid
		if (!gauge->content_container || !lv_obj_is_valid(gauge->content_container)) {
			printf("[E] bar_graph_gauge: Cannot create labels container - content_container is NULL or invalid\n");
			return;
		}

		gauge->labels_container = lv_obj_create(gauge->content_container);

		lv_obj_set_size(gauge->labels_container, 22, content_height); // 22px wide (20px + 2px for negative values)
		lv_obj_set_style_flex_grow(gauge->labels_container, 0, 0);

		lv_obj_set_style_bg_opa(gauge->labels_container, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(gauge->labels_container, PALETTE_BLACK, 0);
		lv_obj_set_style_border_width(gauge->labels_container, 0, 0); // No border
		lv_obj_set_style_pad_all(gauge->labels_container, 0, 0);
		lv_obj_set_style_pad_left(gauge->labels_container, 0, 0);
		lv_obj_set_style_pad_right(gauge->labels_container, 0, 0);
		lv_obj_set_style_pad_top(gauge->labels_container, 0, 0);
		lv_obj_set_style_pad_bottom(gauge->labels_container, 0, 0);
		lv_obj_clear_flag(gauge->labels_container, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(gauge->labels_container, LV_OBJ_FLAG_SCROLLABLE);

		// FLEXBOX LAYOUT: Labels container (vertical distribution, right-aligned)
		lv_obj_set_flex_flow(gauge->labels_container, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(gauge->labels_container, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER);
		// Remove all padding for precise control
		lv_obj_set_style_pad_all(gauge->labels_container, 0, 0);
		lv_obj_set_style_pad_gap(gauge->labels_container, 0, 0);
		lv_obj_set_style_pad_row(gauge->labels_container, 0, 0);
		lv_obj_set_style_pad_column(gauge->labels_container, 0, 0);

		// Max Label Container - positioned at top
		lv_obj_t* max_container = lv_obj_create(gauge->labels_container);

		lv_obj_set_size(max_container, LV_PCT(100), 20); // Fixed height for proper spacing
		lv_obj_set_style_flex_grow(max_container, 0, 0);
		lv_obj_set_style_bg_opa(max_container, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(max_container, PALETTE_BLACK, 0);
		lv_obj_set_style_border_width(max_container, 0, 0); // No border
		lv_obj_set_style_pad_all(max_container, 0, 0); // No padding
		lv_obj_clear_flag(max_container, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(max_container, LV_OBJ_FLAG_SCROLLABLE);
		// Align content to right side of container
		lv_obj_set_flex_flow(max_container, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(max_container, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
		lv_obj_set_style_pad_right(max_container, 2, 0); // Move left by 2px from right edge

		// Max Label (inside the container)
		gauge->max_label = lv_label_create(max_container);
		lv_obj_set_style_text_font(gauge->max_label, &lv_font_montserrat_12, 0);
		lv_label_set_text(gauge->max_label, "MAX"); // DEBUG: Set initial text
		lv_obj_set_style_text_color(gauge->max_label, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->max_label, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(gauge->max_label, PALETTE_BLACK, 0);
		lv_obj_set_style_border_width(gauge->max_label, 0, 0); // No border
		lv_obj_set_style_radius(gauge->max_label, 0, 0); // No border radius
		lv_obj_clear_flag(gauge->max_label, LV_OBJ_FLAG_CLICKABLE);
		// Right align text for better negative value display
		lv_obj_set_style_text_align(gauge->max_label, LV_TEXT_ALIGN_RIGHT, 0);

		// Center Label Container
		lv_obj_t* center_container = lv_obj_create(gauge->labels_container);
		lv_obj_set_size(center_container, LV_PCT(100), LV_SIZE_CONTENT); // Let it grow
		lv_obj_set_style_flex_grow(center_container, 1, 0); // Make it grow to fill space
		lv_obj_set_style_bg_opa(center_container, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(center_container, PALETTE_BLACK, 0);
		lv_obj_set_style_border_width(center_container, 0, 0); // No border
		lv_obj_set_style_pad_all(center_container, 0, 0); // No padding
		lv_obj_clear_flag(center_container, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(center_container, LV_OBJ_FLAG_SCROLLABLE);
		// Align content to right side of container, center vertically
		lv_obj_set_flex_flow(center_container, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(center_container, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		lv_obj_set_style_pad_right(center_container, 2, 0); // Move left by 2px from right edge

		// Center Label (inside the container)
		gauge->center_label = lv_label_create(center_container);
		lv_obj_set_style_text_font(gauge->center_label, &lv_font_montserrat_12, 0);
		lv_label_set_text(gauge->center_label, "CEN"); // DEBUG: Set initial text
		lv_obj_set_style_text_color(gauge->center_label, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->center_label, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(gauge->center_label, PALETTE_BLACK, 0);
		lv_obj_set_style_border_width(gauge->center_label, 0, 0); // No border
		lv_obj_set_style_radius(gauge->center_label, 0, 0); // No border radius
		lv_obj_clear_flag(gauge->center_label, LV_OBJ_FLAG_CLICKABLE);
		// Right align text for better negative value display
		lv_obj_set_style_text_align(gauge->center_label, LV_TEXT_ALIGN_RIGHT, 0);

		// Min Label Container - positioned at bottom
		lv_obj_t* min_container = lv_obj_create(gauge->labels_container);
		lv_obj_set_size(min_container, LV_PCT(100), 20); // Fixed height for proper spacing
		lv_obj_set_style_flex_grow(min_container, 0, 0);
		lv_obj_set_style_bg_opa(min_container, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(min_container, PALETTE_BLACK, 0);
		lv_obj_set_style_border_width(min_container, 0, 0); // No border
		lv_obj_set_style_pad_all(min_container, 0, 0); // No padding
		lv_obj_clear_flag(min_container, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(min_container, LV_OBJ_FLAG_SCROLLABLE);
		// Align content to right side and bottom of container
		lv_obj_set_flex_flow(min_container, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(min_container, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
		lv_obj_set_style_pad_right(min_container, 2, 0); // Move left by 2px from right edge

		// Min Label (inside the container)
		gauge->min_label = lv_label_create(min_container);
		lv_obj_set_style_text_font(gauge->min_label, &lv_font_montserrat_12, 0);
		lv_label_set_text(gauge->min_label, "MIN"); // DEBUG: Set initial text
		lv_obj_set_style_text_color(gauge->min_label, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->min_label, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(gauge->min_label, PALETTE_BLACK, 0);
		lv_obj_set_style_border_width(gauge->min_label, 0, 0); // No border
		lv_obj_set_style_radius(gauge->min_label, 0, 0); // No border radius
		lv_obj_clear_flag(gauge->min_label, LV_OBJ_FLAG_CLICKABLE);
		// Right align text for better negative value display
		lv_obj_set_style_text_align(gauge->min_label, LV_TEXT_ALIGN_RIGHT, 0);

	} else {
		// No Y-axis
		gauge->labels_container = NULL;
		gauge->max_label = NULL;
		gauge->center_label = NULL;
		gauge->min_label = NULL;
	}

	int wrapper_width = gauge->show_y_axis ? gauge->width - 22 : gauge->width; // 22px for labels

	lv_obj_t* canvas_wrapper = lv_obj_create(gauge->content_container);
	lv_obj_set_size(canvas_wrapper, wrapper_width, content_height);
	lv_obj_set_style_flex_grow(canvas_wrapper, 1, 0); // Allow wrapper to grow and fill remaining space
	lv_obj_set_style_bg_opa(canvas_wrapper, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(canvas_wrapper, PALETTE_BLACK, 0);
	lv_obj_set_style_border_width(canvas_wrapper, 0, 0);
	lv_obj_set_style_pad_all(canvas_wrapper, 0, 0);
	lv_obj_clear_flag(canvas_wrapper, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(canvas_wrapper, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_align_to(canvas_wrapper, gauge->content_container, LV_ALIGN_RIGHT_MID, 0, 0);

	// Center the canvas within the wrapper
	lv_obj_set_flex_flow(canvas_wrapper, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(canvas_wrapper, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// Create canvas container inside the wrapper
	gauge->canvas_container = lv_obj_create(canvas_wrapper);

	// Calculate canvas container width based on whether Y-axis is shown
	int side_padding = 8; // 8px padding on each side
	int canvas_container_width = gauge->show_y_axis ? gauge->width - 22 - side_padding : gauge->width - side_padding;

	lv_obj_set_size(gauge->canvas_container, canvas_container_width, content_height);
	lv_obj_set_style_flex_grow(gauge->canvas_container, 0, 0);
	lv_obj_set_style_bg_opa(gauge->canvas_container, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(gauge->canvas_container, PALETTE_BLACK, 0);
	lv_obj_set_style_border_width(gauge->canvas_container, 0, 0); // No border
	lv_obj_set_style_pad_all(gauge->canvas_container, 0, 0);
	lv_obj_set_style_pad_left(gauge->canvas_container, 0, 0);
	lv_obj_set_style_pad_right(gauge->canvas_container, 0, 0);
	lv_obj_set_style_pad_top(gauge->canvas_container, 0, 0);
	lv_obj_set_style_pad_bottom(gauge->canvas_container, 0, 0);
	lv_obj_clear_flag(gauge->canvas_container, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(gauge->canvas_container, LV_OBJ_FLAG_EVENT_BUBBLE); // Allow events to bubble up
	lv_obj_clear_flag(gauge->canvas_container, LV_OBJ_FLAG_SCROLLABLE);

	gauge->cached_draw_width = canvas_container_width; // Match canvas container width
	gauge->cached_draw_height = content_height; // Reduced height to accommodate inline title

	gauge->canvas = lv_canvas_create(gauge->canvas_container);

	lv_obj_set_size(gauge->canvas, canvas_container_width, content_height); // Match container size

	// Let flexbox handle positioning - canvas fills its container
	lv_obj_set_style_border_width(gauge->canvas, 0, 0); // No border
	lv_obj_set_style_radius(gauge->canvas, 0, 0); // No border radius

	// Ensure canvas is not clickable but allows event bubbling
	lv_obj_clear_flag(gauge->canvas, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(gauge->canvas, LV_OBJ_FLAG_EVENT_BUBBLE); // Allow events to bubble up



	// Calculate buffer size and log it
	size_t buffer_size = gauge->cached_draw_width * gauge->cached_draw_height * sizeof(lv_color_t);
	gauge->canvas_buffer = malloc(buffer_size);

	lv_canvas_set_buffer(
		gauge->canvas, gauge->canvas_buffer,
		gauge->cached_draw_width, gauge->cached_draw_height,
		LV_COLOR_FORMAT_RGB888
	);

	lv_canvas_fill_bg(gauge->canvas, PALETTE_BLACK, LV_OPA_COVER);

	// Create indicator lines as direct children of gauge container (positioned relative to canvas)
	if (gauge->show_y_axis) {

		lv_obj_update_layout(gauge->canvas_container);

		int indicator_width = 1;
		int tick_width = 3;
		lv_coord_t canvas_height = lv_obj_get_height(gauge->canvas_container);

		// Vertical line
		gauge->indicator_vertical_line = lv_obj_create(gauge->content_container);
		lv_obj_set_size(gauge->indicator_vertical_line, indicator_width, canvas_height);
		lv_obj_set_style_bg_color(gauge->indicator_vertical_line, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->indicator_vertical_line, LV_OPA_COVER, 0);
		lv_obj_set_style_border_width(gauge->indicator_vertical_line, 0, 0);
		lv_obj_clear_flag(gauge->indicator_vertical_line, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_align_to(
			gauge->indicator_vertical_line, gauge->canvas_container,
			LV_ALIGN_OUT_LEFT_MID, -tick_width, 0
		);

		// Top horizontal line
		gauge->indicator_top_line = lv_obj_create(gauge->content_container);
		lv_obj_set_size(gauge->indicator_top_line, tick_width, indicator_width );
		lv_obj_set_style_bg_color(gauge->indicator_top_line, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->indicator_top_line, LV_OPA_COVER, 0);
		lv_obj_set_style_border_width(gauge->indicator_top_line, 0, 0);
		lv_obj_clear_flag(gauge->indicator_top_line, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_align_to(
			gauge->indicator_top_line, gauge->canvas_container,
			LV_ALIGN_OUT_TOP_LEFT, -tick_width, +indicator_width
		);

		// Middle horizontal line
		gauge->indicator_middle_line = lv_obj_create(gauge->content_container);
		lv_obj_set_size(gauge->indicator_middle_line, tick_width * 2, indicator_width );
		lv_obj_set_style_bg_color(gauge->indicator_middle_line, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->indicator_middle_line, LV_OPA_COVER, 0);
		lv_obj_set_style_border_width(gauge->indicator_middle_line, 0, 0);
		lv_obj_clear_flag(gauge->indicator_middle_line, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_align_to(
			gauge->indicator_middle_line, gauge->canvas_container,
			LV_ALIGN_OUT_LEFT_MID, tick_width / 2, 0
		);

		// Bottom horizontal line
		gauge->indicator_bottom_line = lv_obj_create(gauge->content_container);
		lv_obj_set_size(gauge->indicator_bottom_line, tick_width, indicator_width );
		lv_obj_set_style_bg_color(gauge->indicator_bottom_line, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->indicator_bottom_line, LV_OPA_COVER, 0);
		lv_obj_set_style_border_width(gauge->indicator_bottom_line, 0, 0);
		lv_obj_clear_flag(gauge->indicator_bottom_line, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_align_to(
			gauge->indicator_bottom_line, gauge->canvas_container,
			LV_ALIGN_OUT_BOTTOM_LEFT, -tick_width, -indicator_width
		);
	} else {

		gauge->indicator_container = NULL;
		gauge->indicator_vertical_line = NULL;
		gauge->indicator_top_line = NULL;
		gauge->indicator_middle_line = NULL;
		gauge->indicator_bottom_line = NULL;
	}

	// Title Label - positioned inline with gauge container, aligned bottom right (like raw values section)
	if (gauge->show_title) {

		gauge->title_label = lv_label_create(parent); // Create as sibling to gauge container, not child
		lv_obj_set_style_text_font(gauge->title_label, &lv_font_montserrat_12, 0);
		lv_label_set_text(gauge->title_label, "CABIN VOLTAGE (V)"); // DEBUG: Set initial text
		lv_obj_set_style_text_color(gauge->title_label, PALETTE_WHITE, 0);
		lv_obj_set_style_text_align(gauge->title_label, LV_TEXT_ALIGN_RIGHT, 0); // Right align text
		lv_obj_set_style_bg_color(gauge->title_label, lv_color_hex(0x000000), 0); // Black background to obscure border
		lv_obj_set_style_bg_opa(gauge->title_label, LV_OPA_COVER, 0);
		lv_obj_set_style_pad_left(gauge->title_label, 8, 0);
		lv_obj_set_style_pad_right(gauge->title_label, 8, 0);
		lv_obj_set_style_pad_top(gauge->title_label, 1, 0);
		lv_obj_set_style_pad_bottom(gauge->title_label, 1, 0);
		lv_obj_set_style_border_width(gauge->title_label, 0, 0); // No border
		lv_obj_clear_flag(gauge->title_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(gauge->title_label, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_flag(gauge->title_label, LV_OBJ_FLAG_FLOATING); // Float above layout

		// Position inline with gauge container, 10px up and 10px right from bottom-right corner
		lv_obj_align_to(gauge->title_label, gauge->container, LV_ALIGN_BOTTOM_RIGHT, -50, 8);
	} else {

		gauge->title_label = NULL;
	}

	gauge->initialized = true;
}

void bar_graph_gauge_add_data_point(bar_graph_gauge_t *gauge, float value)
{

	// Timeline control: only add data based on timeline duration
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint32_t current_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000; // Convert to milliseconds

	// Calculate how often to add data points based on timeline duration
	// We need to add enough data points to fill the gauge over the timeline duration
	int canvas_width = gauge->cached_draw_width;
	int bar_spacing = gauge->bar_width + gauge->bar_gap;
	int total_bars = canvas_width / bar_spacing;

	uint32_t data_interval_ms = 0;

	if (gauge->timeline_duration_ms > 0 && total_bars > 0) {
		// Calculate how often to add a data point to fill the gauge over the timeline duration
		data_interval_ms = gauge->timeline_duration_ms / total_bars;
	}

	// Check if enough time has passed to add a new data point
	if (data_interval_ms > 0 &&
		(current_time - gauge->last_data_time) < data_interval_ms) {
		// Not enough time has passed, accumulate this value for averaging
		gauge->accumulated_value += value;
		gauge->sample_count++;
		return;
	}

	// Time interval has passed, calculate average if we have accumulated samples
	float final_value = value;
	if (gauge->sample_count > 0) {
		// Calculate average of accumulated values plus current value
		// Use double precision for calculation, then convert to float
		double average = (gauge->accumulated_value + (double)value) / (gauge->sample_count + 1);
		final_value = (float)average;

		// Reset accumulation for next interval
		gauge->accumulated_value = 0.0;
		gauge->sample_count = 0;
	}

	// Update last data time
	gauge->last_data_time = current_time;

	// Advance head in circular buffer
	if (gauge->head == -1) {

		gauge->head = 0;  // first sample
	} else {

		gauge->head = (gauge->head + 1) % gauge->max_data_points;
	}

	// Store averaged data at the new head position
	gauge->data_points[gauge->head] = final_value;

	// Mark that data was added
	gauge->data_added = true;

	// Use efficient incremental update instead of expensive full redraw
	bar_graph_gauge_update_canvas(gauge);
}

// Background feed: update FIFO and head without canvas draw
void bar_graph_gauge_push_data(bar_graph_gauge_t *gauge, float value)
{
	if (!gauge || !gauge->initialized || !gauge->data_points ) return;

	// Advance head
	gauge->head = (gauge->head + 1) % gauge->max_data_points;
	gauge->data_points[gauge->head] = value;
	// Mark data added but skip canvas ops
	gauge->data_added = true;
}

void bar_graph_gauge_update_labels_and_ticks(bar_graph_gauge_t *gauge)
{

	// Calculate the relative positions within the gauge container (top, middle, bottom)
	int bar_graph_height = gauge->cached_draw_height;
	int bar_graph_top = 0; // Top of bar graph area (relative to gauge container) - use full height
	int bar_graph_center = bar_graph_height / 2; // Center of bar graph area
	int bar_graph_bottom = bar_graph_height; // Bottom of bar graph area

	// Position labels at the calculated positions (relative to gauge container)
	int label_x = 1; // 1px from left edge of gauge (tucked to bounding box)

	// Get label height to properly center them vertically
	lv_coord_t label_height = lv_font_get_line_height(&lv_font_montserrat_12);

	// Create label text (without the '-' since we're using range rectangles now)
	char max_text[16], center_text[16], min_text[16];

	if (gauge->mode == BAR_GRAPH_MODE_BIPOLAR) {
		// For bipolar mode, show max, baseline, and min values
		snprintf(max_text, sizeof(max_text), "%.0f", gauge->init_max_value);
		snprintf(center_text, sizeof(center_text), "%.0f", gauge->baseline_value);
		snprintf(min_text, sizeof(min_text), "%.0f", gauge->init_min_value);
	} else {
		// For positive-only mode, show max, middle, and min values
		float middle_value = (gauge->init_min_value + gauge->init_max_value) / 2.0f;
		snprintf(max_text, sizeof(max_text), "%.0f", gauge->init_max_value);
		snprintf(center_text, sizeof(center_text), "%.0f", middle_value);
		snprintf(min_text, sizeof(min_text), "%.0f", gauge->init_min_value);
	}

	// Set label text (flexbox will handle positioning) - only if labels exist
	if (gauge->max_label) lv_label_set_text(gauge->max_label, max_text);
	if (gauge->center_label) lv_label_set_text(gauge->center_label, center_text);
	if (gauge->min_label) lv_label_set_text(gauge->min_label, min_text);

	// Position range rectangles to overlap with the canvas area (only if they exist)
	int canvas_height = gauge->cached_draw_height;

	// Position range rectangles absolutely to overlap columns (relative to canvas container)
	if (gauge->max_range_rect) lv_obj_align(gauge->max_range_rect, LV_ALIGN_TOP_LEFT, 0, 0);
	if (gauge->center_range_rect) lv_obj_align(gauge->center_range_rect, LV_ALIGN_TOP_LEFT, 0, canvas_height / 2);
	if (gauge->min_range_rect) lv_obj_align(gauge->min_range_rect, LV_ALIGN_TOP_LEFT, 0, canvas_height - 1);

	// Mark that labels have been updated
	gauge->range_values_changed = false;
}

void bar_graph_gauge_update_canvas(bar_graph_gauge_t *gauge)
{

	// Use the actual canvas width for buffer operations (this matches the allocated buffer)
	int canvas_width = gauge->cached_draw_width;
	// Adjust drawing area to match L shape boundaries
	int top_y = 2; // Match L shape top line
	int bottom_y = gauge->cached_draw_height - 5; // Match L shape bottom line
	int h = bottom_y - top_y + 1; // Effective drawing height between L shape lines
	int bar_spacing = (gauge->bar_width + gauge->bar_gap);

	// Calculate how many bars actually fit in the canvas width
	int max_bars_that_fit = canvas_width / (gauge->bar_width + gauge->bar_gap);
	int actual_bars_to_draw = (gauge->max_data_points < max_bars_that_fit) ? gauge->max_data_points : max_bars_that_fit;

	// Ensure we don't exceed canvas boundaries
	if (actual_bars_to_draw * (gauge->bar_width + gauge->bar_gap) > canvas_width) {
		actual_bars_to_draw = canvas_width / (gauge->bar_width + gauge->bar_gap);
	}

	int bar_area_width = canvas_width;

	// Shift canvas left by bar_spacing pixels
	for (int row = 0; row < h; row++) {

		int actual_row = top_y + row; // Offset to match L shape boundaries
		// Shift within the bar drawing area
		memmove(
			&gauge->canvas_buffer[actual_row * canvas_width], // dest (start at beginning of row)
			&gauge->canvas_buffer[actual_row * canvas_width + bar_spacing], // src (start at bar_spacing offset)
			(bar_area_width - bar_spacing) * sizeof(lv_color_t) // bytes (bar area width minus one bar)
		);

		// Clear the rightmost bar_spacing pixels in the bar drawing area
		for (int col = bar_area_width - bar_spacing; col < bar_area_width; col++) {
			gauge->canvas_buffer[actual_row * canvas_width + col] = PALETTE_BLACK;
		}
	}

	// Canvas should auto-refresh after drawing operations

	// Draw the latest bar on the right
	int index = gauge->head;
	float val = gauge->data_points[ index ];
	// Clamp value to the visible range (between L shape lines)
	if (val < gauge->init_min_value) val = gauge->init_min_value;
	if (val > gauge->init_max_value) val = gauge->init_max_value;

	int baseline_y;
	float scale;
	float scale_min = 1.0f;
	float scale_max = 1.0f;

	if (gauge->mode == BAR_GRAPH_MODE_BIPOLAR) {
		float dist_min = gauge->baseline_value - gauge->init_min_value;
		float dist_max = gauge->init_max_value - gauge->baseline_value;

		// Use the actual range for each direction, not the maximum distance
		// This ensures proper scaling for both above and below baseline
		scale_min = (dist_min > 0) ? (float)(h - 2) / (2.0f * dist_min) : 1.0f;
		scale_max = (dist_max > 0) ? (float)(h - 2) / (2.0f * dist_max) : 1.0f;

		baseline_y = h / 2; // Center the baseline in the middle of the drawing area
	} else {
		float range = gauge->init_max_value - gauge->init_min_value;
		scale = (float)(h - 2) / range;
		baseline_y = h - 1;
		// Scale calculated
	}

	int y1, y2;
	if (gauge->mode == BAR_GRAPH_MODE_POSITIVE_ONLY) {
		// Positive only: bars go from min to max (bottom to top)
		int bar_height = (int)((val - gauge->init_min_value) * scale);
		y1 = h - bar_height;
		y2 = h;
	} else {
		// Bipolar: bars centered on baseline
		if (val >= gauge->baseline_value) {
			// Use scale_max for values above baseline
			int bar_height = (int)((val - gauge->baseline_value) * scale_max);
			y1 = baseline_y - bar_height;
			y2 = baseline_y;
		} else {
			// Use scale_min for values below baseline
			int bar_height = (int)((gauge->baseline_value - val) * scale_min);
			y1 = baseline_y;
			y2 = baseline_y + bar_height;
		}
	}

	lv_draw_rect_dsc_t rect_dsc;
	lv_draw_rect_dsc_init(&rect_dsc);
	rect_dsc.bg_color = gauge->bar_color;
	rect_dsc.bg_opa = LV_OPA_COVER;

	// Draw new bar at rightmost position within bar area
	// Create area for rectangle (adjust coordinates for L shape offset)
	lv_area_t rect_area;
	rect_area.x1 = bar_area_width - bar_spacing;
	rect_area.y1 = top_y + y1; // Offset to match L shape boundaries
	rect_area.x2 = rect_area.x1 + gauge->bar_width - 1;
	rect_area.y2 = top_y + y2 - 1; // Offset to match L shape boundaries

	// Initialize canvas layer and draw rectangle
	lv_layer_t layer;
	lv_canvas_init_layer(gauge->canvas, &layer);
	lv_draw_rect(&layer, &rect_dsc, &rect_area);
	lv_canvas_finish_layer(gauge->canvas, &layer);
}

// // Redraw entire canvas from buffer, right-aligning the latest head
// void bar_graph_gauge_redraw_full(bar_graph_gauge_t *gauge)
// {
// 	if (!gauge || !gauge->initialized || !gauge->data_points) return;

// 	int canvas_width = gauge->cached_draw_width;
// 	// Adjust drawing area to match L shape boundaries
// 	int top_y = 2; // Match L shape top line
// 	int bottom_y = gauge->cached_draw_height - 5; // Match L shape bottom line
// 	int h = bottom_y - top_y + 1; // Effective drawing height between L shape lines
// 	int bar_spacing = gauge->bar_width + gauge->bar_gap;
// 	int max_bars_that_fit = canvas_width / (gauge->bar_width + gauge->bar_gap);
// 	int count = (gauge->max_data_points < max_bars_that_fit) ? gauge->max_data_points : max_bars_that_fit;
// 	int bar_area_width = canvas_width; // Use full canvas width for bar area

// 	// Clear canvas (only the drawing area between L shape lines)
// 	for (int row = 0; row < h; row++) {

// 		int actual_row = top_y + row; // Offset to match L shape boundaries

// 		for (int col = 0; col < canvas_width; col++) {

// 			gauge->canvas_buffer[actual_row * canvas_width + col] = PALETTE_BLACK;
// 		}
// 	}

// 	// Precompute scale
// 	float scale;
// 	int baseline_y;

// 	if (gauge->mode == BAR_GRAPH_MODE_BIPOLAR) {

// 		float dist_min = gauge->baseline_value - gauge->init_min_value;
// 		float dist_max = gauge->init_max_value - gauge->baseline_value;
// 		float max_dist = (dist_min > dist_max) ? dist_min : dist_max;

// 		if (max_dist <= 0) max_dist = 1;
// 		scale = (float)(h - 2) / (2.0f * max_dist);
// 		baseline_y = h - 1 - (int)((gauge->baseline_value - gauge->init_min_value) * scale);
// 	} else {

// 		scale = (float)(h - 2) / (gauge->init_max_value - gauge->init_min_value);
// 		baseline_y = h - 1;
// 	}

// 	lv_draw_rect_dsc_t rect_dsc;
// 	lv_draw_rect_dsc_init(&rect_dsc);
// 	rect_dsc.bg_color = gauge->bar_color;
// 	rect_dsc.bg_opa = LV_OPA_COVER;

// 	// Draw bars from oldest to newest into the right-aligned area
// 	int x = bar_area_width - bar_spacing;
// 	int index = gauge->head;

// 	for (int i = 0; i < count; i++) {

// 		// walk backward through data points
// 		float val = gauge->data_points[index];
// 		if (val < gauge->init_min_value) val = gauge->init_min_value;
// 		if (val > gauge->init_max_value) val = gauge->init_max_value;

// 		int y1, y2;

// 		if (gauge->mode == BAR_GRAPH_MODE_POSITIVE_ONLY) {

// 			int bar_height = (int)((val - gauge->init_min_value) * scale);
// 			y1 = h - bar_height;
// 			y2 = h;
// 		} else {

// 			if (val >= gauge->baseline_value) {

// 				int bar_height = (int)((val - gauge->baseline_value) * scale);
// 				y1 = baseline_y - bar_height;
// 				y2 = baseline_y;
// 			} else {

// 				int bar_height = (int)((gauge->baseline_value - val) * scale);
// 				y1 = baseline_y;
// 				y2 = baseline_y + bar_height;
// 			}
// 		}

// 		// Create area for rectangle (adjust coordinates for L shape offset)
// 		lv_area_t rect_area;
// 		rect_area.x1 = x;
// 		rect_area.y1 = top_y + y1; // Offset to match L shape boundaries
// 		rect_area.x2 = x + gauge->bar_width - 1;
// 		rect_area.y2 = top_y + y2 - 1; // Offset to match L shape boundaries

// 		// Initialize canvas layer and draw rectangle
// 		lv_layer_t layer;
// 		lv_canvas_init_layer(gauge->canvas, &layer);
// 		lv_draw_rect(&layer, &rect_dsc, &rect_area);
// 		lv_canvas_finish_layer(gauge->canvas, &layer);
// 		x -= bar_spacing;

// 		if (--index < 0) index = gauge->max_data_points - 1;
// 		if (x < 0) break;
// 	}
// }

void bar_graph_gauge_cleanup(bar_graph_gauge_t *gauge)
{
	if (!gauge) return;
	if (!gauge->initialized) return;

	// Mark as not initialized to prevent double cleanup
	gauge->initialized = false;

	// Free canvas buffer if it exists
	if (gauge->canvas_buffer) {

		free(gauge->canvas_buffer);
		gauge->canvas_buffer = NULL;
	}

	// Free data points if they exist
	if (gauge->data_points) {

		free(gauge->data_points);
		gauge->data_points = NULL;
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
	gauge->max_range_rect = NULL;
	gauge->center_range_rect = NULL;
	gauge->min_range_rect = NULL;
	gauge->initialized = false;
}

void bar_graph_gauge_set_update_interval(bar_graph_gauge_t *gauge, uint32_t interval_ms)
{
	if (!gauge) return;
	gauge->update_interval_ms = interval_ms;

	// Initialize last_data_time if not set
	if (gauge->last_data_time == 0) {

		struct timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		gauge->last_data_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000; // Convert to milliseconds
	}
}

void bar_graph_gauge_set_timeline_duration(bar_graph_gauge_t *gauge, uint32_t duration_ms)
{
	if (!gauge) return;

	gauge->timeline_duration_ms = duration_ms;
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

	// Update title with unit
	if (title && gauge->title_label) {

		char title_with_unit[64];

		if (unit) {

			snprintf(title_with_unit, sizeof(title_with_unit), "%s (%s)", title, unit);
		} else {

			strncpy(title_with_unit, title, sizeof(title_with_unit) - 1);
			title_with_unit[sizeof(title_with_unit) - 1] = '\0';
		}

		lv_label_set_text(gauge->title_label, title_with_unit);
	}

	// Apply show_title visibility (title is now positioned outside, no canvas reflow needed)
	if (gauge->title_label) {

		if (gauge->show_title) {

			lv_obj_clear_flag(gauge->title_label, LV_OBJ_FLAG_HIDDEN);
		} else {

			lv_obj_add_flag(gauge->title_label, LV_OBJ_FLAG_HIDDEN);
		}

		// Update canvas metrics (reserve space for inline title)
		// Canvas width should match the canvas container width, not the full gauge width
		// Add padding on both sides for even spacing
		int side_padding = 8; // 8px padding on each side
		int canvas_container_width = gauge->show_y_axis ? gauge->width - 22 - (side_padding * 2) : gauge->width - (side_padding * 2);
		gauge->cached_draw_width = canvas_container_width;
		gauge->cached_draw_height = gauge->height - 15; // Reserve space for title

		// Update content container height (reserve space for inline title)
		int content_height = gauge->height - 15;
		lv_obj_set_size(gauge->content_container, gauge->width, content_height);
		lv_obj_set_size(gauge->labels_container, 22, content_height);

		// Update border styling based on show_border setting
		lv_obj_set_style_border_width(gauge->container, gauge->show_border ? 1 : 0, 0);
		if (gauge->show_border) {

			lv_obj_set_style_border_color(gauge->container, PALETTE_WHITE, 0);
			lv_obj_set_style_radius(gauge->container, 4, 0);
		}

		lv_obj_set_size(gauge->canvas_container, gauge->width - 20, content_height);
		lv_obj_set_size(gauge->canvas, gauge->width - 20, content_height);

		// Recreate buffer to match new size
		if (gauge->canvas_buffer) {

			free(gauge->canvas_buffer);
			gauge->canvas_buffer = NULL;
		}

		gauge->canvas_buffer = malloc(gauge->cached_draw_width * gauge->cached_draw_height * sizeof(lv_color_t));


		lv_canvas_set_buffer(gauge->canvas, gauge->canvas_buffer,
			gauge->cached_draw_width, gauge->cached_draw_height,
			LV_COLOR_FORMAT_RGB888);
		lv_canvas_fill_bg(gauge->canvas, PALETTE_BLACK, LV_OPA_COVER);
	}

	// Ensure canvas leaves space for Y-axis labels when enabled
	uint32_t new_padding = gauge->show_y_axis ? 20 : 0; // Reduced padding to use more space

	if (new_padding != gauge->canvas_padding) {

		gauge->canvas_padding = new_padding;
		// Recompute drawable width and realign canvas
		// Canvas width should match the canvas container width, not the full gauge width
		// Add padding on both sides for even spacing
		int side_padding = 8; // 8px padding on each side
		int canvas_container_width = gauge->show_y_axis ? gauge->width - 22 - (side_padding * 2) : gauge->width - (side_padding * 2);
		gauge->cached_draw_width = canvas_container_width;

		if (gauge->cached_draw_width < 0) gauge->cached_draw_width = 0;

		// Update canvas container size to match the new width
		lv_obj_set_size(gauge->canvas_container, canvas_container_width, gauge->cached_draw_height);

		// Recreate canvas buffer to match new width
		if (gauge->canvas_buffer) {

			free(gauge->canvas_buffer);
			gauge->canvas_buffer = NULL;
		}

		gauge->canvas_buffer = malloc(gauge->cached_draw_width * gauge->cached_draw_height * sizeof(lv_color_t));

		lv_canvas_set_buffer(
			gauge->canvas, gauge->canvas_buffer,
			gauge->cached_draw_width, gauge->cached_draw_height,
			LV_COLOR_FORMAT_RGB888
		);

		lv_canvas_fill_bg(gauge->canvas, PALETTE_BLACK, LV_OPA_COVER);
	}

	// Create Y-axis labels if they don't exist but are now enabled
	if (gauge->show_y_axis && !gauge->labels_container) {

		// Create Y-axis labels container (left side) - use same width as initial creation
		gauge->labels_container = lv_obj_create(gauge->content_container);
		lv_obj_set_size(gauge->labels_container, 22, gauge->height - 20); // Match initial creation width and height
		lv_obj_set_style_bg_opa(gauge->labels_container, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(gauge->labels_container, PALETTE_BLACK, 0);
		lv_obj_set_style_border_width(gauge->labels_container, 0, 0); // No border
		lv_obj_set_style_pad_all(gauge->labels_container, 0, 0);
		lv_obj_set_style_pad_left(gauge->labels_container, 0, 0);
		lv_obj_set_style_pad_right(gauge->labels_container, 0, 0);
		lv_obj_set_style_pad_top(gauge->labels_container, 0, 0);
		lv_obj_set_style_pad_bottom(gauge->labels_container, 0, 0);
		lv_obj_clear_flag(gauge->labels_container, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(gauge->labels_container, LV_OBJ_FLAG_SCROLLABLE);

		// FLEXBOX LAYOUT: Labels container (vertical distribution, right-aligned)
		lv_obj_set_flex_flow(gauge->labels_container, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(gauge->labels_container, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER);

		// Create Y-axis labels
		// Max Label
		gauge->max_label = lv_label_create(gauge->labels_container);
		lv_obj_set_style_text_font(gauge->max_label, &lv_font_montserrat_12, 0);
		lv_label_set_text(gauge->max_label, "MAX"); // DEBUG: Set initial text
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
		lv_label_set_text(gauge->center_label, "CEN"); // DEBUG: Set initial text
		lv_obj_set_style_text_color(gauge->center_label, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->center_label, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(gauge->center_label, PALETTE_BLACK, 0);
		lv_obj_set_style_border_width(gauge->center_label, 0, 0); // No border
		lv_obj_set_style_radius(gauge->center_label, 0, 0); // No border radius
		lv_obj_clear_flag(gauge->center_label, LV_OBJ_FLAG_CLICKABLE);
		// Right align text for better negative value display
		lv_obj_set_style_text_align(gauge->center_label, LV_TEXT_ALIGN_RIGHT, 0);

		// Min Label
		gauge->min_label = lv_label_create(gauge->labels_container);
		lv_obj_set_style_text_font(gauge->min_label, &lv_font_montserrat_12, 0);
		lv_label_set_text(gauge->min_label, "MIN"); // DEBUG: Set initial text
		lv_obj_set_style_text_color(gauge->min_label, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_opa(gauge->min_label, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(gauge->min_label, PALETTE_BLACK, 0);
		lv_obj_set_style_border_width(gauge->min_label, 0, 0); // No border
		lv_obj_set_style_radius(gauge->min_label, 0, 0); // No border radius
		lv_obj_clear_flag(gauge->min_label, LV_OBJ_FLAG_CLICKABLE);
		// Right align text for better negative value display
		lv_obj_set_style_text_align(gauge->min_label, LV_TEXT_ALIGN_RIGHT, 0);
	}

	// Y-axis labels will be updated when range values change
	bar_graph_gauge_update_labels_and_ticks(gauge);
}
