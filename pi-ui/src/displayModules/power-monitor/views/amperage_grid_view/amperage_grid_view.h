#ifndef POWER_MONITOR_AMPERAGE_GRID_VIEW_H
#define POWER_MONITOR_AMPERAGE_GRID_VIEW_H

#include <lvgl.h>
#include "../../power-monitor.h"
#include "../../../shared/gauges/bar_graph_gauge/bar_graph_gauge.h"

// Function declarations for amperage grid view
void power_monitor_amperage_grid_view_render(lv_obj_t *container);
void power_monitor_amperage_grid_view_update_data(void);
void power_monitor_amperage_grid_view_apply_alert_flashing(const power_monitor_data_t* data, int starter_lo, int starter_hi, int house_lo, int house_hi, int solar_lo, int solar_hi, bool blink_on);
void power_monitor_amperage_grid_view_update_configuration(void);
void power_monitor_reset_amperage_static_gauges(void);

// Current view gauge variables (extern for access from power-monitor.c)
extern bar_graph_gauge_t s_starter_current_gauge;
extern bar_graph_gauge_t s_house_current_gauge;
extern bar_graph_gauge_t s_solar_current_gauge;

#endif // POWER_MONITOR_AMPERAGE_GRID_VIEW_H
