#ifndef BAR_GRAPH_GAUGE_CANVAS_H
#define BAR_GRAPH_GAUGE_CANVAS_H

#include <lvgl.h>
#include <stdint.h>
#include <stdbool.h>

// Use void* to avoid circular dependency

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
	lv_obj_t *parent;
	lv_obj_t *container;
	lv_obj_t *content_container;
	lv_obj_t *labels_container;
	lv_obj_t *canvas_container;
	lv_obj_t *title_label;
	lv_obj_t *max_label;
	lv_obj_t *center_label;
	lv_obj_t *min_label;
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
	uint32_t last_data_time;

	// Timeline control
	uint32_t timeline_duration_ms;  // How long it takes for data to move across the gauge

	// Data - reference to external history (not owned by gauge)
	int history_type;  // power_monitor_gauge_type_t or -1 if not using persistent history
	int last_rendered_head;  // Last head position from ring buffer that we rendered
	uint32_t last_render_time_ms;  // Last time we rendered (for throttling)
	uint32_t last_update_ms;  // Last time we updated (for cutover logic)
	bool just_seeded;  // Flag: we just seeded, catchup on first add_data_point
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

	// Smooth scrolling state
	int scroll_offset_px;       // 0..(bar_width + bar_gap - 1)
	float prev_value;           // last committed value
	float next_value;           // upcoming value for current bar
	uint32_t last_tick_ms;      // last smooth tick timestamp
	float pixels_per_second;    // derived from timeline duration
	float pixel_accumulator;    // subpixel accumulator for smooth advance
	lv_timer_t *smooth_timer;   // 60Hz timer driving 1px shifts
	// Discrete animation per data point
	uint32_t animation_duration_ms; // how long one shift (bar_spacing) should take
	uint32_t animation_cutover_ms; // if data interval <= this, skip smooth and jump
	uint32_t anim_start_ms;
	uint32_t anim_end_ms;
	int anim_pixels_moved;      // number of pixels shifted in current animation
	bool animating;
	float anim_px_accum;        // fractional pixel accumulator for discrete animation
	float anim_progress;        // 0..1 progress across current animation window
	// Per-bar cached draw value to keep uniform height across bar width
	bool bar_draw_value_valid;
	float bar_draw_value;
	// Immediate jump state for cutover
	bool cutover_jump_active;

	// Pending new sample buffering (to apply after shift completes)
	bool has_pending_sample;
	float pending_value;

} bar_graph_gauge_t;

// Core functions
void bar_graph_gauge_init(
	bar_graph_gauge_t *gauge, lv_obj_t *parent,
	int x, int y, int width, int height,
	int bar_width, int bar_gap);

// Set which persistent history this gauge should render from
void bar_graph_gauge_set_history_type(bar_graph_gauge_t *gauge, int history_type);

void bar_graph_gauge_add_data_point(bar_graph_gauge_t *gauge, void* gauge_data_history);

void bar_graph_gauge_draw_all_data(bar_graph_gauge_t *gauge, void* gauge_data_history);
void bar_graph_gauge_draw_all_data_snapshot(bar_graph_gauge_t *gauge, int snapshot_head, void* gauge_data_history_pointer);

void bar_graph_gauge_cleanup(bar_graph_gauge_t *gauge);
void bar_graph_gauge_set_timeline_duration(bar_graph_gauge_t *gauge, uint32_t duration_ms);
// Set per-sample animation duration (ms) for smooth shift; 0 = immediate shift
void bar_graph_gauge_set_animation_duration(bar_graph_gauge_t *gauge, uint32_t duration_ms);
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
void bar_graph_gauge_update_y_axis_labels(bar_graph_gauge_t *gauge);

// Force complete current animation (useful for interrupting smooth animations)
void bar_graph_gauge_force_complete_animation(bar_graph_gauge_t *gauge);

#ifdef __cplusplus
}
#endif

#endif
