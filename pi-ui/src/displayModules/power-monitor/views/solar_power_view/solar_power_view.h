#ifndef SOLAR_POWER_VIEW_H
#define SOLAR_POWER_VIEW_H

#include <lvgl.h>
#include "../../power-monitor.h"
#include "../../../shared/views/single_value_bar_graph_view/single_value_bar_graph_view.h"

// External variable for gauge access from power-monitor.c
extern single_value_bar_graph_view_state_t* single_view_solar_power;

// Function prototypes
void power_monitor_solar_power_view_render(lv_obj_t *container);
void power_monitor_solar_power_view_update_data(void);
void power_monitor_reset_solar_power_static_gauge(void);

#endif // SOLAR_POWER_VIEW_H
