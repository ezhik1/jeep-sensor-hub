#ifndef POWER_MONITOR_H
#define POWER_MONITOR_H

#include <stdint.h>
#include <stdbool.h>
#include <lvgl.h>
#include "displayModules/shared/gauges/bar_graph_gauge/bar_graph_gauge.h"
#include "gauge_types.h"
#include "../../data/lerp_data/lerp_data.h"

// Forward declarations
struct detail_screen_t;
struct display_module_base_s;

// Structure to store sensor value label references
typedef struct {
	lv_obj_t* starter_voltage;
	lv_obj_t* starter_current;
	lv_obj_t* house_voltage;
	lv_obj_t* house_current;
	lv_obj_t* solar_voltage;
	lv_obj_t* solar_current;
} power_monitor_sensor_labels_t;

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
	bool voltage_error;
	bool current_error;
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

	// Sensor label references for detail screen
	power_monitor_sensor_labels_t sensor_labels;
} power_monitor_data_t;

// Module interface functions
void power_monitor_init(void);
void power_monitor_init_with_default_view(power_monitor_view_type_t default_view);
power_monitor_data_t* power_monitor_get_data(void);
void power_monitor_cleanup(void);
void power_monitor_show_detail_screen(void);
void power_monitor_destroy_detail_screen(void);

// New standardized lifecycle
void power_monitor_create(void);   // once: initialize UI elements and data
void power_monitor_destroy(void);  // gracefully destroy all UI elements and state
void power_monitor_render(void);   // per-frame UI updates only (no data writes)

// Module base accessor (for home screen to wire lifecycle)
struct display_module_base_s* power_monitor_get_module_base(void);

// Starter voltage view functions
void power_monitor_starter_voltage_view_render(lv_obj_t *container);
void power_monitor_starter_voltage_view_update_data(void);
void power_monitor_starter_voltage_view_apply_alert_flashing(
	const power_monitor_data_t* data,
	int starter_lo, int starter_hi,
	int house_lo, int house_hi,
	int solar_lo, int solar_hi,
	bool blink_on
);

// Data access functions
float power_monitor_get_current(void);
bool power_monitor_is_connected(void);
bool power_monitor_is_active(void);

// Centralized gauge history API (data-only, UI reads from here)
void power_monitor_push_gauge_sample(power_monitor_data_type_t gauge_type, float value);
void power_monitor_update_all_gauge_histories(void); // Called every frame
void power_monitor_seed_gauge_from_history(power_monitor_gauge_type_t gauge_type, bar_graph_gauge_t* gauge);

// View management functions
void power_monitor_render_simple_view(lv_obj_t *container);
// Data-only update function (no UI structure changes)
void power_monitor_update_data_only(void);

// Force all gauges to redraw from persistent history (useful after modal changes)
void power_monitor_force_gauge_redraw_from_history(void);

// Detail screen gauge range update function
void power_monitor_update_detail_gauge_ranges(void);


// Overlay control for shared current_view
void power_monitor_show_in_container(lv_obj_t *container);
void power_monitor_show_in_container_home(lv_obj_t *container);
void power_monitor_create_current_view_content(lv_obj_t* container);

// Module state management functions (controlled by this module)
void power_monitor_init_state(void);
power_monitor_view_type_t power_monitor_get_current_view_type(void);
void power_monitor_set_current_view_type(power_monitor_view_type_t view_type);
void power_monitor_cycle_current_view(void);

void power_monitor_handle_back_button(void);
void power_monitor_handle_alerts_button(void);
void power_monitor_handle_timeline_button(void);

void power_monitor_render_current_view(lv_obj_t* container);

// View state management
void power_monitor_power_grid_view_reset_state(void);

// Timeline modal functions
void power_monitor_timeline_changed_callback(int gauge_index, int duration_seconds, bool is_current_view);
void power_monitor_update_gauge_timeline_duration(power_monitor_gauge_type_t gauge_type);
void power_monitor_update_data_type_timeline_duration(power_monitor_data_type_t data_type, const char* view_type);
void power_monitor_update_single_view_gauge_pointer(void);

// Detail screen sensor label functions
void power_monitor_create_sensor_labels_in_detail_screen(lv_obj_t* container);
void power_monitor_update_sensor_labels_in_detail_screen(lv_obj_t* sensor_section, const lerp_power_monitor_data_t* lerp_data);

// Modal functions are internal to power-monitor module

#ifdef __cplusplus
}
#endif

#endif // POWER_MONITOR_H
