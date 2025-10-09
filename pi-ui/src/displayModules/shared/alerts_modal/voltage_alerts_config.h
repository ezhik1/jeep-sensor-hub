#ifndef VOLTAGE_ALERTS_CONFIG_H
#define VOLTAGE_ALERTS_CONFIG_H

#include "alerts_modal.h"
#include "../../../state/device_state.h"
#include "../gauges/bar_graph_gauge.h"
#include "../../../screens/detail_screen/detail_screen.h"

#ifdef __cplusplus
extern "C" {
#endif

// Voltage-specific gauge configuration
extern const alerts_modal_gauge_config_t voltage_gauge_configs[3];

// Voltage-specific modal configuration
extern const alerts_modal_config_t voltage_alerts_config;

// Voltage-specific callback functions
float voltage_get_value_callback(int gauge_index, int field_type);
void voltage_set_value_callback(int gauge_index, int field_type, float value);
void voltage_refresh_callback(void);

#ifdef __cplusplus
}
#endif

#endif // VOLTAGE_ALERTS_CONFIG_H
