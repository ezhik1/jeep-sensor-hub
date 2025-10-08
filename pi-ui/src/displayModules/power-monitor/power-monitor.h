#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include <lvgl.h>
#include "displayModules/shared/gauges/bar_graph_gauge.h"

// View type enumeration
typedef enum {
	POWER_MONITOR_VIEW_CURRENT = 0,
	POWER_MONITOR_VIEW_VOLTAGE,
	POWER_MONITOR_VIEW_POWER,
	POWER_MONITOR_VIEW_BAR_GRAPH,
	POWER_MONITOR_VIEW_NUMERICAL,
	POWER_MONITOR_VIEW_COUNT
} power_monitor_view_type_t;

#ifdef __cplusplus
extern "C" {
#endif

// Battery data structure
typedef struct {
	float voltage;
	float current;
	bool is_connected;
	bool is_charging;
	uint32_t last_update;
} battery_data_t;

typedef struct {
	float current_amps;
	bool is_connected;
	bool is_active;
	uint32_t last_update_ms;

	// Battery monitoring data
	battery_data_t starter_battery;
	battery_data_t house_battery;
	battery_data_t solar_input;
	bool ignition_on;
} power_monitor_data_t;

// Module interface functions
void power_monitor_init(void);
void power_monitor_init_with_default_view(power_monitor_view_type_t default_view);
power_monitor_data_t* power_monitor_get_data(void);
void power_monitor_cleanup(void);
void power_monitor_show_detail_screen(void);
void power_monitor_destroy_detail_screen(void);

// Starter voltage view functions
void power_monitor_starter_voltage_view_render(lv_obj_t *container);
void power_monitor_starter_voltage_view_update_data(void);
const char* power_monitor_starter_voltage_view_get_title(void);
void power_monitor_starter_voltage_view_apply_alert_flashing(const power_monitor_data_t* data, int starter_lo, int starter_hi, int house_lo, int house_hi, int solar_lo, int solar_hi, bool blink_on);

// Data access functions
float power_monitor_get_current(void);
bool power_monitor_is_connected(void);
bool power_monitor_is_active(void);

// View management functions
void power_monitor_render_simple_view(lv_obj_t *container);
// Data-only update function (no UI structure changes)
void power_monitor_update_data_only(void);

// Detail screen gauge range update function
void power_monitor_update_detail_gauge_ranges(void);


// Overlay control for shared current_view
void power_monitor_show_in_container(lv_obj_t *container);
void power_monitor_show_in_container_home(lv_obj_t *container);
// void power_monitor_show_in_container_detail(lv_obj_t *container); - REMOVED, using shared template
void power_monitor_create_current_view_in_container(lv_obj_t* container);
void power_monitor_create_current_view_content(lv_obj_t* container);
void power_monitor_destroy(void);

// Module state management functions (controlled by this module)
void power_monitor_init_state(void);
power_monitor_view_type_t power_monitor_get_current_view_type(void);
void power_monitor_set_current_view_type(power_monitor_view_type_t view_type);
void power_monitor_cycle_current_view(void);

// Missing function declarations for current_view.c and detail.c compatibility
void power_monitor_bar_graph_view_render(lv_obj_t *parent);
void power_monitor_bar_graph_view_update_data(void);
void power_monitor_open_alerts_modal(void);
void power_monitor_open_timeline_modal(void);
void power_monitor_handle_back_button(void);
void power_monitor_handle_alerts_button(void);
void power_monitor_handle_timeline_button(void);

// Additional compatibility functions for screen_manager and home_screen
void power_monitor_destroy(void);
void power_monitor_show_in_container_home(lv_obj_t* container);
void power_monitor_show_in_container_detail(lv_obj_t* container);
void power_monitor_cycle_current_view(void);
void power_monitor_render_current_view(lv_obj_t* container);

// Original power-monitor UI functions (matching original signatures)
lv_obj_t* power_monitor_current_view_init(lv_obj_t* parent);
void power_monitor_current_view_update(lv_obj_t* container);

// Interface function to get current values from detail screen gauges (for power grid view)
bool power_monitor_get_detail_gauge_values(float* starter_voltage, float* house_voltage, float* solar_voltage);

// Stub implementations for view_manager compatibility
void *power_monitor_detail_get_instance(void);
void power_monitor_detail_render_bar_graphs(void *view);

// Comprehensive settlement function
void power_monitor_ensure_complete_settlement(void);

// View state management
void power_monitor_power_grid_view_reset_state(void);

#ifdef __cplusplus
}
#endif

#endif // POWER_MONITOR_H
