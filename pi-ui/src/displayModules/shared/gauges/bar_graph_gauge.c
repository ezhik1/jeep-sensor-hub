#include "bar_graph_gauge.h"
#include "esp_compat.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "bar_graph_gauge";

// Draw range indicator lines on the canvas
static void bar_graph_gauge_draw_range_indicator(bar_graph_gauge_t *gauge)
{
	if (!gauge || !gauge->canvas_buffer || !gauge->show_y_axis) {
		ESP_LOGW(TAG, "Cannot draw range indicator: gauge=%p, buffer=%p, show_y_axis=%d",
			gauge, gauge ? gauge->canvas_buffer : NULL, gauge ? gauge->show_y_axis : 0);
		return;
	}

	int canvas_width = gauge->cached_draw_width;
	int canvas_height = gauge->cached_draw_height;

	// Safety check for valid dimensions
	if (canvas_width <= 0 || canvas_height <= 0) {
		ESP_LOGW(TAG, "Invalid canvas dimensions: %dx%d", canvas_width, canvas_height);
		return;
	}

	// Draw thicker lines for better visibility - optimized for performance
	int line_width = 4;
	lv_color_t white_color = lv_color_hex(0xFFFFFF);

	// Top line (max value) - horizontal line at top, tucked in by 2px
	int top_y = 2; // Tuck in by 2px from top
	if (top_y < canvas_height && line_width <= canvas_width) {
		// Optimized: unroll the loop for better performance
		gauge->canvas_buffer[top_y * canvas_width + 0] = white_color;
		gauge->canvas_buffer[top_y * canvas_width + 1] = white_color;
		gauge->canvas_buffer[top_y * canvas_width + 2] = white_color;
		gauge->canvas_buffer[top_y * canvas_width + 3] = white_color;
	}

	// Middle line (center/baseline value) - horizontal line at middle
	int middle_y = canvas_height / 2;
	if (middle_y < canvas_height && line_width <= canvas_width) {
		// Optimized: unroll the loop for better performance
		gauge->canvas_buffer[middle_y * canvas_width + 0] = white_color;
		gauge->canvas_buffer[middle_y * canvas_width + 1] = white_color;
		gauge->canvas_buffer[middle_y * canvas_width + 2] = white_color;
		gauge->canvas_buffer[middle_y * canvas_width + 3] = white_color;
	}

	// Bottom line (min value) - horizontal line at bottom, tucked in by 4px
	int bottom_y = canvas_height - 5; // Tuck in by 4px
	if (bottom_y < canvas_height && line_width <= canvas_width) {
		// Optimized: unroll the loop for better performance
		gauge->canvas_buffer[bottom_y * canvas_width + 0] = white_color;
		gauge->canvas_buffer[bottom_y * canvas_width + 1] = white_color;
		gauge->canvas_buffer[bottom_y * canvas_width + 2] = white_color;
		gauge->canvas_buffer[bottom_y * canvas_width + 3] = white_color;
	}

	// Vertical line connecting them - on the left edge, from top_y to bottom_y
	// Optimized: draw vertical line using direct memory access
	if (top_y < canvas_height && bottom_y < canvas_height) {
		for (int y = top_y; y <= bottom_y; y++) {
			gauge->canvas_buffer[y * canvas_width] = white_color;
		}
	}
}
void bar_graph_gauge_init(
	bar_graph_gauge_t *gauge,
	lv_obj_t *parent,
	int x,
	int y,
	int width,
	int height
)
{
	if (!gauge || !parent) {
		ESP_LOGE(TAG, "Invalid parameters: gauge=%p, parent=%p", gauge, parent);
		return;
	}

	// Check if parent is valid
	if (!lv_obj_is_valid(parent)) {
		ESP_LOGE(TAG, "Parent object is not valid");
		return;
	}

	ESP_LOGI(TAG, "Initializing gauge: parent=%p, size=%dx%d", parent, width, height);

	memset(gauge, 0, sizeof(bar_graph_gauge_t));

	gauge->x = x;
	gauge->y = y;
	gauge->width = width;
	gauge->height = height;
	gauge->show_title = true; // Default to true, can be overridden by configure_advanced
	gauge->show_y_axis = true; // Enable Y-axis by default for detail screen gauges
	gauge->show_border = false; // Default to no border, can be overridden by configure_advanced
	gauge->update_interval_ms = 0; // No rate limiting
	gauge->data_added = false; // No data added yet
	gauge->bar_width = 6; // Wider bars for visibility
	gauge->bar_gap = 2;   // 2px gap between bars
	// Defaults; actual mode/range will be set by configure_advanced
	gauge->mode = BAR_GRAPH_MODE_POSITIVE_ONLY;
	gauge->baseline_value = 0.0f;
	gauge->init_min_value = 0.0f;
	gauge->init_max_value = 1.0f;
	gauge->range_values_changed = true; // Initial values need label update
	gauge->canvas_padding = gauge->show_y_axis ? 20 : 0; // Y-axis labels container is 20px wide for canvas calculation


	gauge->cached_bar_color = lv_color_hex(0x00FF00); // Normal green color
	gauge->max_data_points = (gauge->width - gauge->canvas_padding) / (gauge->bar_width + gauge->bar_gap);
	if (gauge->max_data_points < 5) gauge->max_data_points = 5;
	if (gauge->max_data_points > 200) gauge->max_data_points = 200;


	// Data points calculated

	gauge->data_points = malloc(gauge->max_data_points * sizeof(float));
	if (!gauge->data_points) {
		ESP_LOGE(TAG, "Failed to allocate data points array");
		return;
	}
	memset(gauge->data_points, 0, gauge->max_data_points * sizeof(float));
	gauge->head = -1;

	// MAIN Gauge Container
	gauge->container = lv_obj_create(parent);
	lv_obj_set_size(gauge->container, width, height);
	// Don't set position for flexbox layout - let parent handle positioning
	ESP_LOGI(TAG, "Gauge container created with size: %dx%d", width, height);
	lv_obj_set_style_pad_all(gauge->container, 0, 0);
	lv_obj_set_style_pad_left(gauge->container, 0, 0);
	lv_obj_set_style_pad_right(gauge->container, 0, 0);
	lv_obj_set_style_pad_top(gauge->container, 0, 0);
	lv_obj_set_style_pad_bottom(gauge->container, 0, 0);
	lv_obj_set_style_bg_color(gauge->container, lv_color_black(), 0);
	lv_obj_set_style_border_width(gauge->container, gauge->show_border ? 1 : 0, 0); // Conditional border
	if (gauge->show_border) {
		lv_obj_set_style_border_color(gauge->container, lv_color_hex(0xFFFFFF), 0); // White border
	}
	lv_obj_set_style_radius(gauge->container, 0, 0); // No border radius
	lv_obj_add_flag(gauge->container, LV_OBJ_FLAG_CLICKABLE); // Make clickable for touch event bubbling
	lv_obj_add_flag(gauge->container, LV_OBJ_FLAG_EVENT_BUBBLE); // Allow events to bubble up
	lv_obj_clear_flag(gauge->container, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_set_flex_flow(gauge->container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(gauge->container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_gap(gauge->container, 0, 0); // NO gap between content and title
	lv_obj_set_style_pad_row(gauge->container, 0, 0); // NO row gap
	lv_obj_set_style_pad_column(gauge->container, 0, 0); // NO column gap
	ESP_LOGI(TAG, "Gauge container ID: %p", gauge->container);

	// Create Graph Container (orange border) - contains Y-axis + Canvas
	gauge->content_container = lv_obj_create(gauge->container);
	// Reserve space for inline title label (reduce height by 15px to accommodate title)
	int content_height = gauge->height - 15;
	lv_obj_set_size(gauge->content_container, gauge->width, content_height);
	// Let flexbox handle positioning - content_container will be first child
	ESP_LOGI(TAG, "Content container created: width=%d, height=%d, gauge_height=%d, show_title=%d",
		gauge->width, content_height, gauge->height, gauge->show_title);
	lv_obj_set_style_bg_opa(gauge->content_container, LV_OPA_TRANSP, 0);
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

	// FLEXBOX LAYOUT: Graph container (horizontal: Y-axis labels + Canvas)
	lv_obj_set_flex_flow(gauge->content_container, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(gauge->content_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_gap(gauge->content_container, 0, 0); // NO gap between Y-axis and canvas
	lv_obj_set_style_pad_row(gauge->content_container, 0, 0); // NO row gap
	lv_obj_set_style_pad_column(gauge->content_container, 0, 0); // NO column gap
	ESP_LOGI(TAG, "Graph container ID: %p", gauge->content_container);

	// Create Y-axis labels container (left side)
	if (gauge->show_y_axis) {
		// Safety check: ensure content_container is valid
		if (!gauge->content_container || !lv_obj_is_valid(gauge->content_container)) {
			ESP_LOGE(TAG, "Cannot create labels container - content_container is NULL or invalid");
			return;
		}

		gauge->labels_container = lv_obj_create(gauge->content_container);
		if (!gauge->labels_container) {
			ESP_LOGE(TAG, "Failed to create labels container");
			return;
		}

		lv_obj_set_size(gauge->labels_container, 22, content_height); // 22px wide (20px + 2px for negative values)
		// Prevent Y-axis labels container from growing beyond its specified width
		lv_obj_set_style_flex_grow(gauge->labels_container, 0, 0);
		ESP_LOGI(TAG, "Y-axis labels container created: width=22, height=%d, gauge_height=%d",
			content_height, gauge->height);
		lv_obj_set_style_bg_opa(gauge->labels_container, LV_OPA_TRANSP, 0);
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
		ESP_LOGI(TAG, "Labels container ID: %p", gauge->labels_container);

		// Max Label Container - positioned at top
		lv_obj_t* max_container = lv_obj_create(gauge->labels_container);
		if (!max_container) {
			ESP_LOGE(TAG, "Failed to create max_container");
			return;
		}
		lv_obj_set_size(max_container, LV_PCT(100), 20); // Fixed height for proper spacing
		lv_obj_set_style_flex_grow(max_container, 0, 0);
		lv_obj_set_style_bg_opa(max_container, LV_OPA_TRANSP, 0); // Transparent background
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
		lv_obj_set_style_text_color(gauge->max_label, lv_color_hex(0xFFFFFF), 0); // White text for visibility
		lv_obj_set_style_bg_opa(gauge->max_label, LV_OPA_TRANSP, 0); // Transparent background
		lv_obj_set_style_border_width(gauge->max_label, 0, 0); // No border
		lv_obj_set_style_radius(gauge->max_label, 0, 0); // No border radius
		lv_obj_clear_flag(gauge->max_label, LV_OBJ_FLAG_CLICKABLE);
		// Right align text for better negative value display
		lv_obj_set_style_text_align(gauge->max_label, LV_TEXT_ALIGN_RIGHT, 0);

		// Center Label Container
		lv_obj_t* center_container = lv_obj_create(gauge->labels_container);
		lv_obj_set_size(center_container, LV_PCT(100), LV_SIZE_CONTENT); // Let it grow
		lv_obj_set_style_flex_grow(center_container, 1, 0); // Make it grow to fill space
		lv_obj_set_style_bg_opa(center_container, LV_OPA_TRANSP, 0); // Transparent background
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
		lv_obj_set_style_text_color(gauge->center_label, lv_color_hex(0xFFFFFF), 0); // White text for visibility
		lv_obj_set_style_bg_opa(gauge->center_label, LV_OPA_TRANSP, 0); // Transparent background
		lv_obj_set_style_border_width(gauge->center_label, 0, 0); // No border
		lv_obj_set_style_radius(gauge->center_label, 0, 0); // No border radius
		lv_obj_clear_flag(gauge->center_label, LV_OBJ_FLAG_CLICKABLE);
		// Right align text for better negative value display
		lv_obj_set_style_text_align(gauge->center_label, LV_TEXT_ALIGN_RIGHT, 0);

		// Min Label Container - positioned at bottom
		lv_obj_t* min_container = lv_obj_create(gauge->labels_container);
		lv_obj_set_size(min_container, LV_PCT(100), 20); // Fixed height for proper spacing
		lv_obj_set_style_flex_grow(min_container, 0, 0);
		lv_obj_set_style_bg_opa(min_container, LV_OPA_TRANSP, 0); // Transparent background
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
		lv_obj_set_style_text_color(gauge->min_label, lv_color_hex(0xFFFFFF), 0); // White text for visibility
		lv_obj_set_style_bg_opa(gauge->min_label, LV_OPA_TRANSP, 0); // Transparent background
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

	// Create canvas container (right side of content area, next to Y-axis labels) - red border
	// Safety check: ensure content_container is valid
	if (!gauge->content_container || !lv_obj_is_valid(gauge->content_container)) {
		ESP_LOGE(TAG, "Cannot create canvas container - content_container is NULL or invalid");
		return;
	}

	gauge->canvas_container = lv_obj_create(gauge->content_container);
	if (!gauge->canvas_container) {
		ESP_LOGE(TAG, "Failed to create canvas container");
		return;
	}

	// Calculate canvas container width based on whether Y-axis is shown
	int canvas_container_width = gauge->show_y_axis ? gauge->width - 20 : gauge->width;
	ESP_LOGI(TAG, "Canvas container width calculation: show_y_axis=%d, gauge_width=%d, canvas_width=%d",
		gauge->show_y_axis, gauge->width, canvas_container_width);
	lv_obj_set_size(gauge->canvas_container, canvas_container_width, content_height);
	// Prevent canvas container from growing beyond its specified width
	lv_obj_set_style_flex_grow(gauge->canvas_container, 0, 0);
	// Let flexbox handle positioning - canvas will be positioned next to Y-axis labels
	ESP_LOGI(TAG, "Canvas container created: width=%d, height=%d, show_y_axis=%d, gauge_width=%d",
		canvas_container_width, content_height, gauge->show_y_axis, gauge->width);
	lv_obj_set_style_bg_opa(gauge->canvas_container, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(gauge->canvas_container, 0, 0); // No border
	lv_obj_set_style_pad_all(gauge->canvas_container, 0, 0);
	lv_obj_set_style_pad_left(gauge->canvas_container, 0, 0);
	lv_obj_set_style_pad_right(gauge->canvas_container, 0, 0);
	lv_obj_set_style_pad_top(gauge->canvas_container, 0, 0);
	lv_obj_set_style_pad_bottom(gauge->canvas_container, 0, 0);
	lv_obj_clear_flag(gauge->canvas_container, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(gauge->canvas_container, LV_OBJ_FLAG_EVENT_BUBBLE); // Allow events to bubble up
	lv_obj_clear_flag(gauge->canvas_container, LV_OBJ_FLAG_SCROLLABLE);
	ESP_LOGI(TAG, "Canvas container ID: %p", gauge->canvas_container);

	// Create canvas
	gauge->cached_draw_width = canvas_container_width; // Match canvas container width
	gauge->cached_draw_height = content_height; // Reduced height to accommodate inline title

	// Safety check: ensure canvas_container is valid
	if (!gauge->canvas_container || !lv_obj_is_valid(gauge->canvas_container)) {
		ESP_LOGE(TAG, "Cannot create canvas - canvas_container is NULL or invalid");
		return;
	}

	gauge->canvas = lv_canvas_create(gauge->canvas_container);
	if (!gauge->canvas) {
		ESP_LOGE(TAG, "Failed to create canvas");
		return;
	}
	lv_obj_set_size(gauge->canvas, canvas_container_width, content_height); // Match container size
	ESP_LOGI(TAG, "CANVAS CREATED: container_width=%d, canvas_size=%dx%d, gauge_width=%d",
		canvas_container_width, canvas_container_width, content_height, gauge->width);
	// Let flexbox handle positioning - canvas fills its container
	lv_obj_set_style_border_width(gauge->canvas, 0, 0); // No border
	lv_obj_set_style_radius(gauge->canvas, 0, 0); // No border radius

	// Ensure canvas is not clickable but allows event bubbling
	lv_obj_clear_flag(gauge->canvas, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(gauge->canvas, LV_OBJ_FLAG_EVENT_BUBBLE); // Allow events to bubble up



	// Calculate buffer size and log it
	size_t buffer_size = gauge->cached_draw_width * gauge->cached_draw_height * sizeof(lv_color_t);
	ESP_LOGI(TAG, "Allocating canvas buffer: %dx%d = %zu bytes",
		gauge->cached_draw_width, gauge->cached_draw_height, buffer_size);

	gauge->canvas_buffer = malloc(buffer_size);

	if (!gauge->canvas_buffer) {
		ESP_LOGE(TAG, "Failed to allocate canvas buffer of %zu bytes", buffer_size);
		free(gauge->data_points);
		gauge->data_points = NULL;
		return;
	}

	ESP_LOGI(TAG, "Canvas buffer allocated successfully: %p", gauge->canvas_buffer);
	lv_canvas_set_buffer(
		gauge->canvas, gauge->canvas_buffer,
						 gauge->cached_draw_width, gauge->cached_draw_height,
		LV_COLOR_FORMAT_RGB888
	);
	lv_canvas_fill_bg(gauge->canvas, lv_color_black(), LV_OPA_COVER);

	// Draw L-shapes once during initialization for better performance
	// These never change, so no need to redraw them on every data update
	if (gauge->show_y_axis) {
		bar_graph_gauge_draw_range_indicator(gauge);
	}

	// Title Label - positioned inline with gauge container, aligned bottom right (like raw values section)
	if (gauge->show_title) {
		gauge->title_label = lv_label_create(parent); // Create as sibling to gauge container, not child
		lv_obj_set_style_text_font(gauge->title_label, &lv_font_montserrat_12, 0);
		lv_label_set_text(gauge->title_label, "CABIN VOLTAGE (V)"); // DEBUG: Set initial text
		lv_obj_set_style_text_color(gauge->title_label, lv_color_hex(0xFFFFFF), 0); // White text for visibility
		lv_obj_set_style_text_align(gauge->title_label, LV_TEXT_ALIGN_RIGHT, 0); // Right align text
		lv_obj_set_style_bg_color(gauge->title_label, lv_color_hex(0x000000), 0); // Black background to obscure border
		lv_obj_set_style_bg_opa(gauge->title_label, LV_OPA_COVER, 0);
		lv_obj_set_style_pad_left(gauge->title_label, 8, 0);
		lv_obj_set_style_pad_right(gauge->title_label, 8, 0);
		lv_obj_set_style_pad_top(gauge->title_label, 2, 0);
		lv_obj_set_style_pad_bottom(gauge->title_label, 2, 0);
		lv_obj_set_style_border_width(gauge->title_label, 0, 0); // No border
		lv_obj_clear_flag(gauge->title_label, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(gauge->title_label, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_flag(gauge->title_label, LV_OBJ_FLAG_FLOATING); // Float above layout

		// Position inline with gauge container, 10px up and 10px right from bottom-right corner
		lv_obj_align_to(gauge->title_label, gauge->container, LV_ALIGN_BOTTOM_RIGHT, -50, 8);
		ESP_LOGI(TAG, "Title label ID: %p (positioned inline bottom-right)", gauge->title_label);
	} else {
		gauge->title_label = NULL;
	}

	// Canvas is ready for bar graph drawing
	ESP_LOGI(TAG, "Canvas initialized: container=%dx%d, canvas=%dx%d",
		gauge->width, gauge->height, gauge->cached_draw_width, gauge->cached_draw_height);


	gauge->initialized = true;
	ESP_LOGI(TAG, "Gauge initialization completed successfully: %p", gauge);

	// Canvas is ready for bar graph drawing
	// Bar graph gauge initialized
}

void bar_graph_gauge_add_data_point(bar_graph_gauge_t *gauge, float value)
{
	if (!gauge || !gauge->initialized || !gauge->data_points ) {
		ESP_LOGW(TAG, "Cannot add data point: gauge=%p, initialized=%d, data_points=%p",
			gauge, gauge ? gauge->initialized : 0, gauge ? gauge->data_points : NULL);
		return;
	}

	// Rate limiting: only add data if enough time has passed
	uint32_t current_time = esp_timer_get_time() / 1000; // Convert to milliseconds
	if (gauge->update_interval_ms > 0 &&
		(current_time - gauge->last_data_time) < gauge->update_interval_ms) {
		// Not enough time has passed, skip this data point
		return;
	}

	// Update last data time
	gauge->last_data_time = current_time;

	// Advance head in circular buffer
	if (gauge->head == -1) {
		gauge->head = 0;  // first sample
	} else {
		gauge->head = (gauge->head + 1) % gauge->max_data_points;
	}

	// Store new data at the new head position
	gauge->data_points[gauge->head] = value;

	// Mark that data was added
	gauge->data_added = true;

	// Data point added

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
	if (!gauge) {
		ESP_LOGE(TAG, "bar_graph_gauge_update_labels_and_ticks: gauge is NULL");
		return;
	}

	if (!gauge->initialized) {
		ESP_LOGW(TAG, "bar_graph_gauge_update_labels_and_ticks: gauge not initialized, skipping");
		return;
	}

	if (!gauge->show_y_axis) {
		ESP_LOGE(TAG, "bar_graph_gauge_update_labels_and_ticks: show_y_axis is NULL");
		return;
	}


	// Update labels and ticks (removed range_values_changed check for initialization)
	// if (!gauge->range_values_changed) {
	//	ESP_LOGD(TAG, "bar_graph_gauge_update_labels_and_ticks: range values unchanged, skipping");
	//	return;
	// }


	// Use the known gauge dimensions for relative positioning within the container
	// int gauge_width = gauge->width;
	// int gauge_height = gauge->height;

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
	if (!gauge || !gauge->initialized || !gauge->data_points) {
		ESP_LOGW(TAG, "Cannot update canvas: gauge=%p, initialized=%d, data_points=%p",
			gauge, gauge ? gauge->initialized : 0, gauge ? gauge->data_points : NULL);
		return;
	}

	// Updating canvas

	// Use the actual canvas width for buffer operations (this matches the allocated buffer)
	int canvas_width = gauge->cached_draw_width;
	// Adjust drawing area to match L shape boundaries
	int top_y = 2; // Match L shape top line
	int bottom_y = gauge->cached_draw_height - 5; // Match L shape bottom line
	int h = bottom_y - top_y + 1; // Effective drawing height between L shape lines
	int bar_spacing = gauge->bar_width + gauge->bar_gap;

	// Calculate how many bars actually fit in the canvas width
	int max_bars_that_fit = canvas_width / (gauge->bar_width + gauge->bar_gap);
	int actual_bars_to_draw = (gauge->max_data_points < max_bars_that_fit) ? gauge->max_data_points : max_bars_that_fit;

	// Ensure we don't exceed canvas boundaries
	if (actual_bars_to_draw * (gauge->bar_width + gauge->bar_gap) > canvas_width) {
		actual_bars_to_draw = canvas_width / (gauge->bar_width + gauge->bar_gap);
	}

	int bar_area_width = canvas_width; // Use full canvas width for bar area

	// Bar calculations complete

	// Shift canvas left by bar_spacing pixels (Y-axis is outside canvas, so use full width)
	for (int row = 0; row < h; row++) {
		int actual_row = top_y + row; // Offset to match L shape boundaries
		// Shift within the bar drawing area
		memmove(
			&gauge->canvas_buffer[actual_row * canvas_width],                              // dest (start at beginning of row)
			&gauge->canvas_buffer[actual_row * canvas_width + bar_spacing],                // src (start at bar_spacing offset)
			(bar_area_width - bar_spacing) * sizeof(lv_color_t)                    // bytes (bar area width minus one bar)
		);
		// Clear the rightmost bar_spacing pixels in the bar drawing area
		for (int col = bar_area_width - bar_spacing; col < bar_area_width; col++) {
			gauge->canvas_buffer[actual_row * canvas_width + col] = lv_color_black();
		}
	}

	// Canvas should auto-refresh after drawing operations

	// Draw the latest bar on the right
	int idx = gauge->head;
	float val = gauge->data_points[idx];
	// Clamp value to the visible range (between L shape lines)
	if (val < gauge->init_min_value) val = gauge->init_min_value;
	if (val > gauge->init_max_value) val = gauge->init_max_value;

	int baseline_y;
	float scale;

	if (gauge->mode == BAR_GRAPH_MODE_BIPOLAR) {
		float dist_min = gauge->baseline_value - gauge->init_min_value;
		float dist_max = gauge->init_max_value - gauge->baseline_value;
		float max_dist = (dist_min > dist_max) ? dist_min : dist_max;
		if (max_dist <= 0) max_dist = 1;
		scale = (float)(h - 2) / (2.0f * max_dist);
		baseline_y = h - 1 - (int)((gauge->baseline_value - gauge->init_min_value) * scale);
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
			int bar_height = (int)((val - gauge->baseline_value) * scale);
			y1 = baseline_y - bar_height;
			y2 = baseline_y;
		} else {
			int bar_height = (int)((gauge->baseline_value - val) * scale);
			y1 = baseline_y;
			y2 = baseline_y + bar_height;
		}
	}

	lv_draw_rect_dsc_t rect_dsc;
	lv_draw_rect_dsc_init(&rect_dsc);
	rect_dsc.bg_color = gauge->cached_bar_color;
	rect_dsc.bg_opa = LV_OPA_COVER;

	// Draw new bar at rightmost position within bar area
	// Drawing bar
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

	// Redraw L-shapes after drawing bars to ensure they remain visible
	// This is necessary because the canvas operations might affect the L-shapes
	if (gauge->show_y_axis) {
		bar_graph_gauge_draw_range_indicator(gauge);
	}

	// Canvas should auto-refresh after drawing operations
}

// Redraw entire canvas from buffer, right-aligning the latest head
void bar_graph_gauge_redraw_full(bar_graph_gauge_t *gauge)
{
	if (!gauge || !gauge->initialized || !gauge->data_points) return;

	int canvas_width = gauge->cached_draw_width;
	// Adjust drawing area to match L shape boundaries
	int top_y = 2; // Match L shape top line
	int bottom_y = gauge->cached_draw_height - 5; // Match L shape bottom line
	int h = bottom_y - top_y + 1; // Effective drawing height between L shape lines
	int bar_spacing = gauge->bar_width + gauge->bar_gap;
	int max_bars_that_fit = canvas_width / (gauge->bar_width + gauge->bar_gap);
	int count = (gauge->max_data_points < max_bars_that_fit) ? gauge->max_data_points : max_bars_that_fit;
	int bar_area_width = canvas_width; // Use full canvas width for bar area

	// Clear canvas (only the drawing area between L shape lines)
	for (int row = 0; row < h; row++) {
		int actual_row = top_y + row; // Offset to match L shape boundaries
		for (int col = 0; col < canvas_width; col++) {
			gauge->canvas_buffer[actual_row * canvas_width + col] = lv_color_black();
		}
	}

	// Precompute scale
	float scale;
	int baseline_y;
	if (gauge->mode == BAR_GRAPH_MODE_BIPOLAR) {
		float dist_min = gauge->baseline_value - gauge->init_min_value;
		float dist_max = gauge->init_max_value - gauge->baseline_value;
		float max_dist = (dist_min > dist_max) ? dist_min : dist_max;
		if (max_dist <= 0) max_dist = 1;
		scale = (float)(h - 2) / (2.0f * max_dist);
		baseline_y = h - 1 - (int)((gauge->baseline_value - gauge->init_min_value) * scale);
	} else {
		scale = (float)(h - 2) / (gauge->init_max_value - gauge->init_min_value);
		baseline_y = h - 1;
	}

	lv_draw_rect_dsc_t rect_dsc;
	lv_draw_rect_dsc_init(&rect_dsc);
	rect_dsc.bg_color = gauge->cached_bar_color;
	rect_dsc.bg_opa = LV_OPA_COVER;

	// Draw bars from oldest to newest into the right-aligned area
	int x = bar_area_width - bar_spacing;
	int idx = gauge->head;
	for (int i = 0; i < count; i++) {
		// walk backward through data points
		float val = gauge->data_points[idx];
		if (val < gauge->init_min_value) val = gauge->init_min_value;
		if (val > gauge->init_max_value) val = gauge->init_max_value;

		int y1, y2;
		if (gauge->mode == BAR_GRAPH_MODE_POSITIVE_ONLY) {
			int bar_height = (int)((val - gauge->init_min_value) * scale);
			y1 = h - bar_height;
			y2 = h;
		} else {
			if (val >= gauge->baseline_value) {
				int bar_height = (int)((val - gauge->baseline_value) * scale);
				y1 = baseline_y - bar_height;
				y2 = baseline_y;
			} else {
				int bar_height = (int)((gauge->baseline_value - val) * scale);
				y1 = baseline_y;
				y2 = baseline_y + bar_height;
			}
		}

				// Create area for rectangle (adjust coordinates for L shape offset)
				lv_area_t rect_area;
				rect_area.x1 = x;
				rect_area.y1 = top_y + y1; // Offset to match L shape boundaries
				rect_area.x2 = x + gauge->bar_width - 1;
				rect_area.y2 = top_y + y2 - 1; // Offset to match L shape boundaries

				// Initialize canvas layer and draw rectangle
				lv_layer_t layer;
				lv_canvas_init_layer(gauge->canvas, &layer);
				lv_draw_rect(&layer, &rect_dsc, &rect_area);
				lv_canvas_finish_layer(gauge->canvas, &layer);
		x -= bar_spacing;
		if (--idx < 0) idx = gauge->max_data_points - 1;
		if (x < 0) break;
	}

	// Redraw L-shapes after full redraw to ensure they remain visible
	if (gauge->show_y_axis) {
		bar_graph_gauge_draw_range_indicator(gauge);
	}
}

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
		gauge->last_data_time = esp_timer_get_time() / 1000; // Convert to milliseconds
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
	uint32_t color,
	bool show_title,
	bool show_y_axis,
	bool show_border
){
	if (!gauge || !gauge->initialized) {
		ESP_LOGE(TAG, "bar_graph_gauge_configure_advanced: gauge=%p, initialized=%d", gauge, gauge ? gauge->initialized : 0);
		return;
	}

	// Validate baseline is within min/max range for bipolar mode
	if (mode == BAR_GRAPH_MODE_BIPOLAR) {
		if (baseline_value < min_val || baseline_value > max_val) {
			// Set baseline to middle of min/max range if outside bounds
			baseline_value = (min_val + max_val) / 2.0f;
		}
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

	// Store color for drawing and cache the LVGL color
	gauge->color = color;
	// Convert to 16-bit color for better performance
	gauge->cached_bar_color = lv_color_hex(color);

	// Cache the range for performance
	gauge->cached_range = gauge->max_value - gauge->min_value;

	// Y-axis labels will be updated when range values change

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
		gauge->cached_draw_width = gauge->width - gauge->canvas_padding;
		gauge->cached_draw_height = gauge->height - 15; // Reserve space for title

		// Update content container height (reserve space for inline title)
		int content_height = gauge->height - 15;
		lv_obj_set_size(gauge->content_container, gauge->width, content_height);
		lv_obj_set_size(gauge->labels_container, 22, content_height);

		// Update border styling based on show_border setting
		lv_obj_set_style_border_width(gauge->container, gauge->show_border ? 1 : 0, 0);
		if (gauge->show_border) {
			lv_obj_set_style_border_color(gauge->container, lv_color_hex(0xFFFFFF), 0);
			lv_obj_set_style_radius(gauge->container, 4, 0);
		}
		lv_obj_set_size(gauge->canvas_container, gauge->width - 20, content_height);
		lv_obj_set_size(gauge->canvas, gauge->width - 20, content_height);

		// DISABLED: Canvas resizing conflicts with flexbox layout
		// lv_obj_set_size(gauge->canvas, gauge->cached_draw_width, gauge->cached_draw_height);
		// lv_obj_align(gauge->canvas, LV_ALIGN_TOP_LEFT, gauge->canvas_padding, title_space);

		// Recreate buffer to match new size
		if (gauge->canvas_buffer) {

			free(gauge->canvas_buffer);
			gauge->canvas_buffer = NULL;
		}

		gauge->canvas_buffer = malloc(gauge->cached_draw_width * gauge->cached_draw_height * sizeof(lv_color_t));

		if (!gauge->canvas_buffer) {

			ESP_LOGE(TAG, "Failed to reallocate canvas buffer (title toggle)");
			return;
		}

		lv_canvas_set_buffer(gauge->canvas, gauge->canvas_buffer,
			gauge->cached_draw_width, gauge->cached_draw_height,
			LV_COLOR_FORMAT_RGB888);
		lv_canvas_fill_bg(gauge->canvas, lv_color_black(), LV_OPA_COVER);

		// Redraw L-shapes after canvas buffer recreation
		if (gauge->show_y_axis) {
			bar_graph_gauge_draw_range_indicator(gauge);
		}
	}

	// Ensure canvas leaves space for Y-axis labels when enabled
	uint32_t new_padding = gauge->show_y_axis ? 20 : 0; // Reduced padding to use more space
	if (new_padding != gauge->canvas_padding) {
		gauge->canvas_padding = new_padding;
		// Recompute drawable width and realign canvas
		gauge->cached_draw_width = gauge->width - gauge->canvas_padding;
		if (gauge->cached_draw_width < 0) gauge->cached_draw_width = 0;
		// DISABLED: Canvas resizing conflicts with flexbox layout
		// lv_obj_set_size(gauge->canvas, gauge->cached_draw_width, gauge->cached_draw_height);
		// lv_obj_align(gauge->canvas, LV_ALIGN_TOP_LEFT, gauge->canvas_padding, 0);

		// Recreate canvas buffer to match new width
		if (gauge->canvas_buffer) {
			free(gauge->canvas_buffer);
			gauge->canvas_buffer = NULL;
		}
		gauge->canvas_buffer = malloc(gauge->cached_draw_width * gauge->cached_draw_height * sizeof(lv_color_t));
		if (!gauge->canvas_buffer) {
			ESP_LOGE(TAG, "Failed to reallocate canvas buffer");
			return;
		}
		lv_canvas_set_buffer(gauge->canvas, gauge->canvas_buffer,
			gauge->cached_draw_width, gauge->cached_draw_height,
			LV_COLOR_FORMAT_RGB888);
		lv_canvas_fill_bg(gauge->canvas, lv_color_black(), LV_OPA_COVER);

		// Redraw L-shapes after canvas buffer recreation
		if (gauge->show_y_axis) {
			bar_graph_gauge_draw_range_indicator(gauge);
		}
	}

	// Create Y-axis labels if they don't exist but are now enabled
	if (gauge->show_y_axis && !gauge->labels_container) {
		ESP_LOGI(TAG, "Creating Y-axis labels for gauge %p", gauge);
		// Create Y-axis labels container (left side) - use same width as initial creation
		gauge->labels_container = lv_obj_create(gauge->content_container);
		lv_obj_set_size(gauge->labels_container, 22, gauge->height - 20); // Match initial creation width and height
		lv_obj_set_style_bg_opa(gauge->labels_container, LV_OPA_TRANSP, 0);
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
		ESP_LOGI(TAG, "Labels container created during configuration: %p", gauge->labels_container);

		// Create Y-axis labels
		// Max Label
		gauge->max_label = lv_label_create(gauge->labels_container);
		lv_obj_set_style_text_font(gauge->max_label, &lv_font_montserrat_12, 0);
		lv_label_set_text(gauge->max_label, "MAX"); // DEBUG: Set initial text
		lv_obj_set_style_text_color(gauge->max_label, lv_color_hex(0xFFFFFF), 0); // White text for visibility
		lv_obj_set_style_bg_opa(gauge->max_label, LV_OPA_TRANSP, 0); // Transparent background
		lv_obj_set_style_border_width(gauge->max_label, 0, 0); // No border
		lv_obj_set_style_radius(gauge->max_label, 0, 0); // No border radius
		lv_obj_clear_flag(gauge->max_label, LV_OBJ_FLAG_CLICKABLE);
		// Right align text for better negative value display
		lv_obj_set_style_text_align(gauge->max_label, LV_TEXT_ALIGN_RIGHT, 0);

		// Center Label
		gauge->center_label = lv_label_create(gauge->labels_container);
		lv_obj_set_style_text_font(gauge->center_label, &lv_font_montserrat_12, 0);
		lv_label_set_text(gauge->center_label, "CEN"); // DEBUG: Set initial text
		lv_obj_set_style_text_color(gauge->center_label, lv_color_hex(0xFFFFFF), 0); // White text for visibility
		lv_obj_set_style_bg_opa(gauge->center_label, LV_OPA_TRANSP, 0); // Transparent background
		lv_obj_set_style_border_width(gauge->center_label, 0, 0); // No border
		lv_obj_set_style_radius(gauge->center_label, 0, 0); // No border radius
		lv_obj_clear_flag(gauge->center_label, LV_OBJ_FLAG_CLICKABLE);
		// Right align text for better negative value display
		lv_obj_set_style_text_align(gauge->center_label, LV_TEXT_ALIGN_RIGHT, 0);

		// Min Label
		gauge->min_label = lv_label_create(gauge->labels_container);
		lv_obj_set_style_text_font(gauge->min_label, &lv_font_montserrat_12, 0);
		lv_label_set_text(gauge->min_label, "MIN"); // DEBUG: Set initial text
		lv_obj_set_style_text_color(gauge->min_label, lv_color_hex(0xFFFFFF), 0); // White text for visibility
		lv_obj_set_style_bg_opa(gauge->min_label, LV_OPA_TRANSP, 0); // Transparent background
		lv_obj_set_style_border_width(gauge->min_label, 0, 0); // No border
		lv_obj_set_style_radius(gauge->min_label, 0, 0); // No border radius
		lv_obj_clear_flag(gauge->min_label, LV_OBJ_FLAG_CLICKABLE);
		// Right align text for better negative value display
		lv_obj_set_style_text_align(gauge->min_label, LV_TEXT_ALIGN_RIGHT, 0);

		// Canvas container size is already set correctly in init - don't resize here
		// Flexbox will handle the layout automatically
	}

	// Y-axis labels will be updated when range values change
	bar_graph_gauge_update_labels_and_ticks(gauge);

	// Debug: Log Y-axis label creation
	if (gauge->show_y_axis && gauge->labels_container) {
		ESP_LOGI(TAG, "Y-axis labels ready: container=%p, max=%p, center=%p, min=%p",
			gauge->labels_container, gauge->max_label, gauge->center_label, gauge->min_label);
	} else {
		ESP_LOGW(TAG, "Y-axis labels NOT created: show_y_axis=%d, labels_container=%p",
			gauge->show_y_axis, gauge->labels_container);
	}

}