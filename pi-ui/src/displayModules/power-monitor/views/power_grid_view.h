#ifndef POWER_MONITOR_POWER_GRID_VIEW_H
#define POWER_MONITOR_POWER_GRID_VIEW_H

#include <lvgl.h>
#include "../power-monitor.h"

// Function declarations for power grid view (renamed from bar_graph_view)
void power_monitor_power_grid_view_render(lv_obj_t *container);
void power_monitor_power_grid_view_update_data(void);
const char* power_monitor_power_grid_view_get_title(void);
void power_monitor_power_grid_view_apply_alert_flashing(const power_monitor_data_t* data, int starter_lo, int starter_hi, int house_lo, int house_hi, int solar_lo, int solar_hi, bool blink_on);
void power_monitor_power_grid_view_update_configuration(void);
void power_monitor_reset_static_gauges(void);

#endif // POWER_MONITOR_POWER_GRID_VIEW_H