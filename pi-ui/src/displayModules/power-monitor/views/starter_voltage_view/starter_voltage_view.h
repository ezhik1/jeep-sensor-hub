#ifndef STARTER_VOLTAGE_VIEW_H
#define STARTER_VOLTAGE_VIEW_H

#include <lvgl.h>
#include "../../power-monitor.h"
#include "../../../shared/views/single_value_bar_graph_view/single_value_bar_graph_view.h"

// External variable for gauge access from power-monitor.c
extern single_value_bar_graph_view_state_t* single_view_starter_voltage;

// Function prototypes
void power_monitor_starter_voltage_view_render(lv_obj_t *container);
void power_monitor_starter_voltage_view_update_data(void);
void power_monitor_starter_voltage_view_apply_alert_flashing(const power_monitor_data_t* data, int starter_lo, int starter_hi, int house_lo, int house_hi, int solar_lo, int solar_hi, bool blink_on);
void power_monitor_starter_voltage_view_update_configuration(void);
void power_monitor_reset_starter_voltage_static_gauge(void);

#endif // STARTER_VOLTAGE_VIEW_H