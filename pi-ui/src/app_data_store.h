#ifndef APP_DATA_STORE_H
#define APP_DATA_STORE_H

#include <stdint.h>
#include <stdbool.h>

// Include actual module data types (not forward declarations)
#include "displayModules/power-monitor/power-monitor.h"

// Persistent gauge history data (survives screen changes)
// Each gauge has exactly as many history points as bars that fit on canvas
// Largest gauge: 233px / (2+3)px = 46 bars, round up for safety
#define MAX_GAUGE_HISTORY 50
typedef struct {
	float values[MAX_GAUGE_HISTORY];
	int count;  // Current number of values in buffer (grows to max_count then stays)
	int max_count;  // Maximum bars for this specific gauge (calculated once)
	int head;  // Ring buffer head pointer (newest data)
	uint32_t last_update_ms;
	bool has_real_data;  // True if we have actual sensor data (not just initial fill)
} persistent_gauge_history_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Central app data store - all dynamic module data lives here
 *
 * Modules subscribe to this data and update their UI based on it.
 * Data is updated centrally in main.c per frame.
 */
typedef struct {
	// Power monitor module data
	power_monitor_data_t* power_monitor;

	// Persistent gauge histories (survive screen changes)
	persistent_gauge_history_t power_monitor_gauge_histories[POWER_MONITOR_GAUGE_COUNT];

	// Add other module data here as you implement them:
	// cooling_management_data_t* cooling_management;
	// environmental_data_t* environmental;
	// tpms_data_t* tpms;
	// etc.
} app_data_store_t;

/**
 * @brief Initialize the app data store
 */
void app_data_store_init(void);

/**
 * @brief Update all data in the store (called per frame in main.c)
 */
void app_data_store_update(void);

/**
 * @brief Cleanup the app data store
 */
void app_data_store_cleanup(void);

/**
 * @brief Get the global data store instance
 */
app_data_store_t* app_data_store_get(void);

#ifdef __cplusplus
}
#endif

#endif // APP_DATA_STORE_H

