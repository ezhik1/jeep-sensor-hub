#ifndef SINGLE_VALUE_BAR_GRAPH_VIEW_H
#define SINGLE_VALUE_BAR_GRAPH_VIEW_H

#include "lvgl.h"
#include "../../utils/number_formatting/number_formatting.h"
#include "../../gauges/bar_graph_gauge/bar_graph_gauge.h"

// Configuration for single value bar graph view
typedef struct {
	const char* title;                    // Title text (e.g., "Starter\nVoltage")
	const char* unit;                     // Unit text (e.g., "(V)")
	lv_color_t bar_graph_color;          // Color for the bar graph
	bar_graph_mode_t bar_mode;           // Bar graph mode (POSITIVE_ONLY or BIPOLAR)
	float baseline_value;                // Baseline value for the gauge
	float min_value;                     // Minimum value for the gauge
	float max_value;                     // Maximum value for the gauge
	number_formatting_config_t number_config; // Number formatting configuration
} single_value_bar_graph_view_config_t;

// Single value bar graph view state
typedef struct {
	lv_obj_t* container;
	lv_obj_t* title_container;
	lv_obj_t* title_label;
	lv_obj_t* unit_label;
	lv_obj_t* value_container;  // Container for value label
	lv_obj_t* value_label;
	lv_obj_t* gauge_container;
	bar_graph_gauge_t gauge;  // Static gauge, not pointer
	number_formatting_config_t number_config;
	bool initialized;
} single_value_bar_graph_view_state_t;

// Function prototypes
single_value_bar_graph_view_state_t* single_value_bar_graph_view_create(lv_obj_t* parent, const single_value_bar_graph_view_config_t* config);
void single_value_bar_graph_view_destroy(single_value_bar_graph_view_state_t* base_view);
void single_value_bar_graph_view_update_data(single_value_bar_graph_view_state_t* base_view, float value, bool has_error);
void single_value_bar_graph_view_render(single_value_bar_graph_view_state_t* base_view);
void single_value_bar_graph_view_apply_alert_flashing(single_value_bar_graph_view_state_t* base_view, float value, float low_threshold, float high_threshold, bool blink_on);
void single_value_bar_graph_view_update_configuration(single_value_bar_graph_view_state_t* base_view, float baseline, float min_val, float max_val);

#endif // SINGLE_VALUE_BAR_GRAPH_VIEW_H