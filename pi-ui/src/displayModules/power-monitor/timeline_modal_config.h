#ifndef POWER_MONITOR_TIMELINE_MODAL_CONFIG_H
#define POWER_MONITOR_TIMELINE_MODAL_CONFIG_H

#include "../shared/timeline_modal/timeline_modal.h"

#ifdef __cplusplus
extern "C" {
#endif

// Power monitor timeline option configurations
extern const timeline_option_config_t power_monitor_timeline_options[TIMELINE_COUNT];

// Power monitor gauge configurations for timeline
extern const timeline_gauge_config_t power_monitor_timeline_gauges[6];

// Power monitor timeline modal configuration
extern const timeline_modal_config_t power_monitor_timeline_modal_config;

// Power monitor timeline changed callback
void power_monitor_timeline_changed_callback(int gauge_index, int duration_seconds, bool is_current_view);

#ifdef __cplusplus
}
#endif

#endif // POWER_MONITOR_TIMELINE_MODAL_CONFIG_H
