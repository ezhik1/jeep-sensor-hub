#include "timeline_modal_config.h"
#include "../../state/device_state.h"
#include <stdio.h>

// Timeline option configurations
const timeline_option_config_t power_monitor_timeline_options[TIMELINE_COUNT] = {
	[TIMELINE_30_SECONDS] = {
		.label = "30",
		.duration_seconds = 30,
		.is_selected = false
	},
	[TIMELINE_1_MINUTE] = {
		.label = "1",
		.duration_seconds = 60,
		.is_selected = false
	},
	[TIMELINE_30_MINUTES] = {
		.label = "30",
		.duration_seconds = 1800,
		.is_selected = false
	},
	[TIMELINE_1_HOUR] = {
		.label = "1",
		.duration_seconds = 3600,
		.is_selected = false
	},
	[TIMELINE_3_HOURS] = {
		.label = "3",
		.duration_seconds = 10800,
		.is_selected = false
	}
};

// Power monitor gauge configurations for timeline (6 electrical gauges)
const timeline_gauge_config_t power_monitor_timeline_gauges[6] = {
	// Gauge 1 - STARTER Battery Voltage
	{
		.name = "STARTER (V)",
		.unit = "V",
		.is_enabled = true
	},
	// Gauge 2 - STARTER Battery Current
	{
		.name = "STARTER (A)",
		.unit = "A",
		.is_enabled = true
	},
	// Gauge 3 - HOUSE Battery Voltage
	{
		.name = "HOUSE (V)",
		.unit = "V",
		.is_enabled = true
	},
	// Gauge 4 - HOUSE Battery Current
	{
		.name = "HOUSE (A)",
		.unit = "A",
		.is_enabled = true
	},
	// Gauge 5 - SOLAR Input Voltage
	{
		.name = "SOLAR (V)",
		.unit = "V",
		.is_enabled = true
	},
	// Gauge 6 - SOLAR Input Current
	{
		.name = "SOLAR (A)",
		.unit = "A",
		.is_enabled = true
	}
};

// Power monitor timeline modal configuration
const timeline_modal_config_t power_monitor_timeline_modal_config = {
	.gauge_count = 6,
	.gauges = (timeline_gauge_config_t*)power_monitor_timeline_gauges,
	.options = (timeline_option_config_t*)power_monitor_timeline_options,
	.modal_title = "Power Monitor Timeline",
	.on_timeline_changed = power_monitor_timeline_changed_callback
};

// Power monitor timeline changed callback
void power_monitor_timeline_changed_callback(int gauge_index, int duration_seconds, bool is_current_view)
{
	printf("[I] power_monitor: Timeline changed for gauge %d to %d seconds (%s view)\n",
		   gauge_index, duration_seconds, is_current_view ? "current" : "detail");

	// Map gauge index to data type
	power_monitor_data_type_t gauge_type;
	switch (gauge_index) {
		case 0: gauge_type = POWER_MONITOR_DATA_STARTER_VOLTAGE; break;
		case 1: gauge_type = POWER_MONITOR_DATA_STARTER_CURRENT; break;
		case 2: gauge_type = POWER_MONITOR_DATA_HOUSE_VOLTAGE; break;
		case 3: gauge_type = POWER_MONITOR_DATA_HOUSE_CURRENT; break;
		case 4: gauge_type = POWER_MONITOR_DATA_SOLAR_VOLTAGE; break;
		case 5: gauge_type = POWER_MONITOR_DATA_SOLAR_CURRENT; break;
		default:
			printf("[E] power_monitor: Invalid gauge index %d\n", gauge_index);
			return;
	}

	// Save to the appropriate view based on is_current_view parameter
	// Helper function to convert gauge type to string name
	const char* gauge_type_to_string(power_monitor_data_type_t gauge_type) {
		switch (gauge_type) {
			case POWER_MONITOR_DATA_STARTER_VOLTAGE: return "starter_voltage";
			case POWER_MONITOR_DATA_STARTER_CURRENT: return "starter_current";
			case POWER_MONITOR_DATA_HOUSE_VOLTAGE: return "house_voltage";
			case POWER_MONITOR_DATA_HOUSE_CURRENT: return "house_current";
			case POWER_MONITOR_DATA_SOLAR_VOLTAGE: return "solar_voltage";
			case POWER_MONITOR_DATA_SOLAR_CURRENT: return "solar_current";
			default: return "unknown";
		}
	}

	if (is_current_view) {
		char timeline_path[256];
		snprintf(timeline_path, sizeof(timeline_path), "power_monitor.gauge_timeline_settings.%s.current_view", gauge_type_to_string(gauge_type));
		device_state_set_int(timeline_path, duration_seconds);
		// Update all gauge instances that use this data type for current view
		extern void power_monitor_update_data_type_timeline_duration(power_monitor_data_type_t data_type, const char* view_type);
		power_monitor_update_data_type_timeline_duration(gauge_type, "current_view");
	} else {
		char timeline_path[256];
		snprintf(timeline_path, sizeof(timeline_path), "power_monitor.gauge_timeline_settings.%s.detail_view", gauge_type_to_string(gauge_type));
		device_state_set_int(timeline_path, duration_seconds);
		// Update all gauge instances that use this data type for detail view
		extern void power_monitor_update_data_type_timeline_duration(power_monitor_data_type_t data_type, const char* view_type);
		power_monitor_update_data_type_timeline_duration(gauge_type, "detail_view");
	}
}
