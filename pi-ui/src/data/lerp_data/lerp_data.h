#ifndef LERP_DATA_H
#define LERP_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "../config.h"

#ifdef __cplusplus
extern "C" {
#endif

// LERP configuration
#define LERP_SPEED 0.2f  // Fast interpolation for high performance
#define LERP_THRESHOLD 0.001f  // Stop interpolating when difference is below this (faster convergence)

// LERP data structure for smooth interpolation
typedef struct {
	float raw_value;        // Raw sensor value (always accessible)
	float display_value;    // Current interpolated display value
	float target_value;     // Target value to interpolate to
	bool is_interpolating;  // Whether we're currently interpolating
	uint32_t last_update_ms; // Last update timestamp
} lerp_value_t;

// LERP data container for power monitor
typedef struct {
	lerp_value_t starter_voltage;
	lerp_value_t starter_current;
	lerp_value_t house_voltage;
	lerp_value_t house_current;
	lerp_value_t solar_voltage;
	lerp_value_t solar_current;
} lerp_power_monitor_data_t;

// LERP system functions
void lerp_data_init(void);
void lerp_data_update(void);
void lerp_data_set_targets(const void* raw_data);
void lerp_data_get_current(lerp_power_monitor_data_t* output);
void lerp_data_cleanup(void);

// Individual value LERP functions
void lerp_value_init(lerp_value_t* lerp_val, float initial_value);
void lerp_value_set_target(lerp_value_t* lerp_val, float target_value);
void lerp_value_update(lerp_value_t* lerp_val);
float lerp_value_get_raw(const lerp_value_t* lerp_val);
float lerp_value_get_display(const lerp_value_t* lerp_val);
bool lerp_value_is_interpolating(const lerp_value_t* lerp_val);

#ifdef __cplusplus
}
#endif

#endif // LERP_DATA_H
