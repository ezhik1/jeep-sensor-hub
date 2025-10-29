#ifndef GAUGE_TYPES_H
#define GAUGE_TYPES_H

#include "../shared/gauges/bar_graph_gauge/bar_graph_gauge.h"
#include "../../data/lerp_data/lerp_data.h"
#include "power-monitor.h"

#ifdef __cplusplus
extern "C" {
#endif

// Data source gauge types (for persistent history)
typedef enum {
	POWER_MONITOR_DATA_STARTER_VOLTAGE = 0,
	POWER_MONITOR_DATA_STARTER_CURRENT,
	POWER_MONITOR_DATA_HOUSE_VOLTAGE,
	POWER_MONITOR_DATA_HOUSE_CURRENT,
	POWER_MONITOR_DATA_SOLAR_VOLTAGE,
	POWER_MONITOR_DATA_SOLAR_CURRENT,
	POWER_MONITOR_DATA_STARTER_POWER,
	POWER_MONITOR_DATA_HOUSE_POWER,
	POWER_MONITOR_DATA_SOLAR_POWER,
	POWER_MONITOR_DATA_COUNT
} power_monitor_data_type_t;

// Power monitor gauge instance types - each gauge instance has a unique ID
typedef enum {
	// Detail view gauges
	POWER_MONITOR_GAUGE_DETAIL_STARTER_VOLTAGE = 0,
	POWER_MONITOR_GAUGE_DETAIL_STARTER_CURRENT,
	POWER_MONITOR_GAUGE_DETAIL_HOUSE_VOLTAGE,
	POWER_MONITOR_GAUGE_DETAIL_HOUSE_CURRENT,
	POWER_MONITOR_GAUGE_DETAIL_SOLAR_VOLTAGE,
	POWER_MONITOR_GAUGE_DETAIL_SOLAR_CURRENT,

	// Power grid view gauges (current_view)
	POWER_MONITOR_GAUGE_GRID_STARTER_VOLTAGE,
	POWER_MONITOR_GAUGE_GRID_HOUSE_VOLTAGE,
	POWER_MONITOR_GAUGE_GRID_SOLAR_VOLTAGE,
	POWER_MONITOR_GAUGE_GRID_STARTER_CURRENT,
	POWER_MONITOR_GAUGE_GRID_HOUSE_CURRENT,
	POWER_MONITOR_GAUGE_GRID_SOLAR_CURRENT,

	// Power grid view gauges (current_view) - wattage
	POWER_MONITOR_GAUGE_GRID_STARTER_POWER,
	POWER_MONITOR_GAUGE_GRID_HOUSE_POWER,
	POWER_MONITOR_GAUGE_GRID_SOLAR_POWER,

	// Single view gauges (current_view)
	POWER_MONITOR_GAUGE_SINGLE_STARTER_VOLTAGE,
	POWER_MONITOR_GAUGE_SINGLE_HOUSE_VOLTAGE,
	POWER_MONITOR_GAUGE_SINGLE_SOLAR_VOLTAGE,
	POWER_MONITOR_GAUGE_SINGLE_STARTER_CURRENT,
	POWER_MONITOR_GAUGE_SINGLE_HOUSE_CURRENT,
	POWER_MONITOR_GAUGE_SINGLE_SOLAR_CURRENT,
	POWER_MONITOR_GAUGE_SINGLE_STARTER_POWER,
	POWER_MONITOR_GAUGE_SINGLE_HOUSE_POWER,
	POWER_MONITOR_GAUGE_SINGLE_SOLAR_POWER,

	POWER_MONITOR_GAUGE_COUNT
} power_monitor_gauge_type_t;

// Function pointer type for getting data values from lerp data
typedef float (*lerp_data_getter_t)(const lerp_power_monitor_data_t* data);

// Gauge map entry structure - maps gauge instances to their types and view contexts
typedef struct {
	power_monitor_gauge_type_t gauge_type;
	bar_graph_gauge_t* gauge; // The actual gauge instance
	const char* gauge_name;	// gauge name
	const char* view_type; // "current_view" or "detail_view" - determines timeline settings
	lerp_data_getter_t data_getter; // Function to get the data value
	const char* error_path; // Path to error field, e.g. "house_battery.voltage.error"
} gauge_map_entry_t;

// External declaration of the gauge map array
extern gauge_map_entry_t gauge_map[POWER_MONITOR_GAUGE_COUNT];

#ifdef __cplusplus
}
#endif

#endif // GAUGE_TYPES_H
