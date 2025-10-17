#ifndef BAR_GRAPH_GAUGE_CANVAS_H
#define BAR_GRAPH_GAUGE_CANVAS_H

#include <lvgl.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	BAR_GRAPH_MODE_POSITIVE_ONLY, // clamp negatives to 0
	BAR_GRAPH_MODE_BIPOLAR        // draw around baseline
} bar_graph_mode_t;

typedef struct {
	// Mode and range
	bar_graph_mode_t mode;
	float baseline_value;
	float init_min_value;
	float init_max_value;

	// LVGL objects
	lv_obj_t *container;
	lv_obj_t *content_container;
	lv_obj_t *labels_container;
	lv_obj_t *canvas_container;
	lv_obj_t *title_label;
	lv_obj_t *max_label;
	lv_obj_t *center_label;
	lv_obj_t *min_label;
	lv_obj_t *max_range_rect;
	lv_obj_t *center_range_rect;
	lv_obj_t *min_range_rect;
	// Indicator line objects (separate from canvas)
	lv_obj_t *indicator_container;
	lv_obj_t *indicator_vertical_line;
	lv_obj_t *indicator_top_line;
	lv_obj_t *indicator_middle_line;
	lv_obj_t *indicator_bottom_line;
	lv_obj_t *canvas;
	lv_color_t *canvas_buffer;

	// Position and size
	int x;
	int y;
	int width;
	int height;

	// Bar config
	int bar_width;
	int bar_gap;

	// Update control
	uint32_t update_interval_ms;
	uint32_t last_data_time;

	// Timeline control
	uint32_t timeline_duration_ms;  // How long it takes for data to move across the gauge

	// Data
	int head;
	int max_data_points; // Maximum number of data points to store
	float *data_points;
	int actual_bars_to_draw; // Number of bars that actually fit in the canvas
	bool initialized;
	float min_value;
	float max_value;
	const char *title;
	const char *unit;
	const char *y_axis_unit;
	uint32_t color;
	bool show_title;
	bool show_y_axis;
	bool show_border; // Flag to control whether gauge has a border
	bool data_added; // Flag to track if data was actually added (for conditional canvas updates)
	bool range_values_changed; // Flag to track when min/max/baseline values change
	uint32_t canvas_padding;
	// Cached performance values
	lv_color_t bar_color;
	int cached_draw_width;
	int cached_draw_height;
	// Cached range for performance (constant for non-auto-scaling)
	float cached_range;
	uint32_t last_invalidate_time; // Last time widget was invalidated (for rate limiting)

	// Data averaging during interval periods
	double accumulated_value; // Sum of values during current interval (double for precision)
	uint32_t sample_count;    // Number of samples during current interval (unsigned for safety)

} bar_graph_gauge_t;

// Core functions
void bar_graph_gauge_init(
	bar_graph_gauge_t *gauge, lv_obj_t *parent,
	int x, int y, int width, int height,
	int bar_width, int bar_gap);

void bar_graph_gauge_add_data_point(bar_graph_gauge_t *gauge, float value);
// Background feed: update data buffer without triggering canvas draw
void bar_graph_gauge_push_data(bar_graph_gauge_t *gauge, float value);
// Draw canvas from current data buffer (no shifting), right-aligned
void bar_graph_gauge_update_canvas(bar_graph_gauge_t *gauge);

void bar_graph_gauge_cleanup(bar_graph_gauge_t *gauge);
void bar_graph_gauge_set_update_interval(bar_graph_gauge_t *gauge, uint32_t interval_ms);
void bar_graph_gauge_set_timeline_duration(bar_graph_gauge_t *gauge, uint32_t duration_ms);
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
	bool show_border);
void bar_graph_gauge_update_labels_and_ticks(bar_graph_gauge_t *gauge);

#ifdef __cplusplus
}
#endif

#endif
