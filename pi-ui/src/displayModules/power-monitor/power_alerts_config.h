#ifndef POWER_ALERTS_CONFIG_H
#define POWER_ALERTS_CONFIG_H

#include "../shared/modals/alerts_modal/alerts_modal.h"
#include "../../state/device_state.h"
#include "../shared/gauges/bar_graph_gauge/bar_graph_gauge.h"
#include "../../screens/detail_screen/detail_screen.h"

#ifdef __cplusplus
extern "C" {
#endif

// Voltage and Current gauge configuration
extern const alerts_modal_gauge_config_t voltage_gauge_configs[6];

// Voltage-specific modal configuration
extern const alerts_modal_config_t power_alerts_config;

// Voltage-specific callback functions
float power_monitor_get_state_values(int gauge_index, int field_type);
void power_monitor_set_state_values(int gauge_index, int field_type, float value);
void power_monitor_refresh_all_data_callback(void);

#ifdef __cplusplus
}
#endif

#endif // POWER_ALERTS_CONFIG_H
