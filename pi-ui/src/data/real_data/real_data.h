#ifndef REAL_DATA_H
#define REAL_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "../config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Real data configuration
#define REAL_UPDATE_INTERVAL_MS 1000  // Update every 1000ms

// Real data initialization and update functions
void real_data_init(void);
void real_data_update(void);
void real_data_write_to_state_objects(void);

// Real data getter functions (placeholder for now)
// These will be implemented when real sensors are connected
void* real_data_get_power_monitor(void);
void* real_data_get_temp_humidity(void);
void* real_data_get_inclinometer(void);
void* real_data_get_gps(void);
void* real_data_get_coolant_temp(void);
void* real_data_get_voltage_monitor(void);
void* real_data_get_tpms(void);
void* real_data_get_compressor_controller(void);

#ifdef __cplusplus
}
#endif

#endif // REAL_DATA_H
