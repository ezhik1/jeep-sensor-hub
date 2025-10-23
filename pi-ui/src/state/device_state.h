#ifndef DEVICE_STATE_H
#define DEVICE_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <cJSON.h>
#include "../displayModules/power-monitor/power-monitor.h"

#ifdef __cplusplus
extern "C" {
#endif

// Simple, scalable JSON device state API
// No more ridiculously long function names!

// Initialize and cleanup
void device_state_init(void);
void device_state_cleanup(void);

// Generic state machine API - handles any namespace structure
int device_state_get_int(const char* path);
float device_state_get_float(const char* path);
bool device_state_get_bool(const char* path);

void device_state_set_int(const char* path, int value);
void device_state_set_float(const char* path, float value);
void device_state_set_bool(const char* path, bool value);

// Check if a path exists without triggering saves
bool device_state_path_exists(const char* path);

// Generic setter that handles both int and float automatically
void device_state_set_value(const char* path, double value);

// Arrays (batch set/get). When save_now is true, persists immediately
void device_state_set_float_array(const char* path, const float* values, int count, bool save_now);
// Returns number of elements copied into out_values (<= max_count)
int device_state_get_float_array(const char* path, float* out_values, int max_count);

// Save/load
void device_state_save(void);
void device_state_load(void);

// All device state access should use the generic functions with namespace strings:
// Examples:
//   device_state_get_int("power_monitor.gauge_timeline_settings[0].current_view")
//   device_state_set_float("power_monitor.starter_baseline_voltage_v", 12.6f)
//   device_state_get_int("power_monitor.starter_alert_low_voltage_v")
//   device_state_set_bool("global.auto_save_enabled", true)

#ifdef __cplusplus
}
#endif

#endif // DEVICE_STATE_JSON_H