#include "battery_alerts_config.h"
#include "../../../state/device_state.h"
#include "../../shared/gauges/bar_graph_gauge/bar_graph_gauge.h"
#include "../../../screens/detail_screen/detail_screen.h"
#include "../power-monitor.h"
#include <string.h>

// Voltage and Current gauge configurations (6 unique sensor inputs)
const alerts_modal_gauge_config_t voltage_gauge_configs[6] = {
	// STARTER Battery
	{
		.name = "STARTER (V)",
		.unit = "V",
		.raw_min_value = 0.0f,    // RAW_MIN: absolute minimum voltage
		.raw_max_value = 20.0f,   // RAW_MAX: absolute maximum voltage
		.fields = {
			[FIELD_ALERT_LOW] = {
				.name = "LOW",
				.min_value = 0.0f,    // Clamped to RAW_MIN
				.max_value = 20.0f,   // Clamped to RAW_MAX
				.default_value = 11.5f,  // Reasonable low alert for 12V battery
				.is_baseline = false
			},
			[FIELD_ALERT_HIGH] = {
				.name = "HIGH",
				.min_value = 0.0f,    // Clamped to RAW_MIN
				.max_value = 20.0f,   // Clamped to RAW_MAX
				.default_value = 14.8f,  // Reasonable high alert for 12V battery
				.is_baseline = false
			},
			[FIELD_GAUGE_LOW] = {
				.name = "LOW",
				.min_value = 0.0f,    // Clamped to RAW_MIN
				.max_value = 20.0f,   // Clamped to RAW_MAX
				.default_value = 11.0f,  // Reasonable gauge low for 12V battery
				.is_baseline = false
			},
			[FIELD_GAUGE_BASELINE] = {
				.name = "BASE",
				.min_value = 0.0f,    // Clamped to RAW_MIN
				.max_value = 20.0f,   // Clamped to RAW_MAX
				.default_value = 12.6f,  // Reasonable baseline for 12V battery
				.is_baseline = true
			},
			[FIELD_GAUGE_HIGH] = {
				.name = "HIGH",
				.min_value = 0.0f,    // Clamped to RAW_MIN
				.max_value = 20.0f,   // Clamped to RAW_MAX
				.default_value = 14.4f,  // Reasonable gauge high for 12V battery
				.is_baseline = false
			}
		},
		.has_baseline = true
	},
	// STARTER Current
	{
		.name = "STARTER (A)",
		.unit = "A",
		.raw_min_value = -50.0f,   // RAW_MIN: negative current (discharging)
		.raw_max_value = 50.0f,    // RAW_MAX: positive current (charging)
		.fields = {
			[FIELD_ALERT_LOW] = {
				.name = "LOW",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = -30.0f, // Reasonable low alert for discharging current
				.is_baseline = false
			},
			[FIELD_ALERT_HIGH] = {
				.name = "HIGH",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = 30.0f,  // Reasonable high alert for charging current
				.is_baseline = false
			},
			[FIELD_GAUGE_LOW] = {
				.name = "LOW",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = -40.0f, // Reasonable gauge low for discharging
				.is_baseline = false
			},
			[FIELD_GAUGE_BASELINE] = {
				.name = "BASE",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = 0.0f,   // Current baseline is 0A (no load)
				.is_baseline = true
			},
			[FIELD_GAUGE_HIGH] = {
				.name = "HIGH",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = 40.0f,  // Reasonable gauge high for charging
				.is_baseline = false
			}
		},
		.has_baseline = true
	},
	// HOUSE Battery
	{
		.name = "HOUSE (V)",
		.unit = "V",
		.raw_min_value = 0.0f,    // RAW_MIN: absolute minimum voltage
		.raw_max_value = 20.0f,   // RAW_MAX: absolute maximum voltage
		.fields = {
			[FIELD_ALERT_LOW] = {
				.name = "LOW",
				.min_value = 0.0f,    // Clamped to RAW_MIN
				.max_value = 20.0f,   // Clamped to RAW_MAX
				.default_value = 11.5f,  // Reasonable low alert for 12V battery
				.is_baseline = false
			},
			[FIELD_ALERT_HIGH] = {
				.name = "HIGH",
				.min_value = 0.0f,    // Clamped to RAW_MIN
				.max_value = 20.0f,   // Clamped to RAW_MAX
				.default_value = 14.8f,  // Reasonable high alert for 12V battery
				.is_baseline = false
			},
			[FIELD_GAUGE_LOW] = {
				.name = "LOW",
				.min_value = 0.0f,    // Clamped to RAW_MIN
				.max_value = 20.0f,   // Clamped to RAW_MAX
				.default_value = 11.0f,  // Reasonable gauge low for 12V battery
				.is_baseline = false
			},
			[FIELD_GAUGE_BASELINE] = {
				.name = "BASE",
				.min_value = 0.0f,    // Clamped to RAW_MIN
				.max_value = 20.0f,   // Clamped to RAW_MAX
				.default_value = 12.6f,  // Reasonable baseline for 12V battery
				.is_baseline = true
			},
			[FIELD_GAUGE_HIGH] = {
				.name = "HIGH",
				.min_value = 0.0f,    // Clamped to RAW_MIN
				.max_value = 20.0f,   // Clamped to RAW_MAX
				.default_value = 14.4f,  // Reasonable gauge high for 12V battery
				.is_baseline = false
			}
		},
		.has_baseline = true
	},
	// HOUSE Current
	{
		.name = "HOUSE (A)",
		.unit = "A",
		.raw_min_value = -50.0f,   // RAW_MIN: negative current (discharging)
		.raw_max_value = 50.0f,    // RAW_MAX: positive current (charging)
		.fields = {
			[FIELD_ALERT_LOW] = {
				.name = "LOW",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = -30.0f, // Reasonable low alert for discharging current
				.is_baseline = false
			},
			[FIELD_ALERT_HIGH] = {
				.name = "HIGH",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = 30.0f,  // Reasonable high alert for charging current
				.is_baseline = false
			},
			[FIELD_GAUGE_LOW] = {
				.name = "LOW",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = -40.0f, // Reasonable gauge low for discharging
				.is_baseline = false
			},
			[FIELD_GAUGE_BASELINE] = {
				.name = "BASE",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = 0.0f,   // Current baseline is 0A (no load)
				.is_baseline = true
			},
			[FIELD_GAUGE_HIGH] = {
				.name = "HIGH",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = 40.0f,  // Reasonable gauge high for charging
				.is_baseline = false
			}
		},
		.has_baseline = true
	},
	// SOLAR Voltage
	{
		.name = "SOLAR (V)",
		.unit = "V",
		.raw_min_value = 0.0f,     // RAW_MIN: no negative solar voltage
		.raw_max_value = 30.0f,    // RAW_MAX: reasonable max solar voltage
		.fields = {
			[FIELD_ALERT_LOW] = {
				.name = "LOW",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 30.0f,    // Clamped to RAW_MAX
				.default_value = 5.0f,    // Reasonable low alert for solar voltage
				.is_baseline = false
			},
			[FIELD_ALERT_HIGH] = {
				.name = "HIGH",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 30.0f,    // Clamped to RAW_MAX
				.default_value = 25.0f,   // Reasonable high alert for solar voltage
				.is_baseline = false
			},
			[FIELD_GAUGE_LOW] = {
				.name = "LOW",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 30.0f,    // Clamped to RAW_MAX
				.default_value = 0.0f,   // Solar voltage baseline is 0V
				.is_baseline = true
			},
			[FIELD_GAUGE_BASELINE] = {
				.name = "BASE",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 30.0f,    // Clamped to RAW_MAX
				.default_value = 0.0f,   // Solar voltage baseline is 0V
				.is_baseline = true
			},
			[FIELD_GAUGE_HIGH] = {
				.name = "HIGH",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 30.0f,    // Clamped to RAW_MAX
				.default_value = 22.0f,  // Reasonable gauge high for solar
				.is_baseline = false
			}
		},
		.has_baseline = true
	},
	// SOLAR Current
	{
		.name = "SOLAR (A)",
		.unit = "A",
		.raw_min_value = 0.0f,     // RAW_MIN: no negative solar current
		.raw_max_value = 20.0f,    // RAW_MAX: reasonable max solar current
		.fields = {
			[FIELD_ALERT_LOW] = {
				.name = "LOW",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 20.0f,    // Clamped to RAW_MAX
				.default_value = 0.1f,   // Reasonable low alert for solar current
				.is_baseline = false
			},
			[FIELD_ALERT_HIGH] = {
				.name = "HIGH",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 20.0f,    // Clamped to RAW_MAX
				.default_value = 15.0f,  // Reasonable high alert for solar current
				.is_baseline = false
			},
			[FIELD_GAUGE_LOW] = {
				.name = "LOW",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 20.0f,    // Clamped to RAW_MAX
				.default_value = 0.0f,   // Solar current baseline is 0A
				.is_baseline = true
			},
			[FIELD_GAUGE_BASELINE] = {
				.name = "BASE",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 20.0f,    // Clamped to RAW_MAX
				.default_value = 0.0f,   // Solar current baseline is 0A
				.is_baseline = true
			},
			[FIELD_GAUGE_HIGH] = {
				.name = "HIGH",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 20.0f,    // Clamped to RAW_MAX
				.default_value = 18.0f,  // Reasonable gauge high for solar
				.is_baseline = false
			}
		},
		.has_baseline = true
	}
};

// Voltage and Current modal configuration
const alerts_modal_config_t battery_alerts_config = {
	.gauge_count = 6,
	.gauges = (alerts_modal_gauge_config_t*)voltage_gauge_configs,
	.get_value_cb = power_monitor_get_state_values,
	.set_value_cb = power_monitor_set_state_values,
	.refresh_cb = power_monitor_refresh_all_data_callback,
	.modal_title = "Power Monitor Alerts & Gauges"
};

// Helper function to determine if a gauge has a baseline based on its name
static bool gauge_has_baseline(const char* gauge_name)
{
	// Solar voltage and solar power don't have baselines
	return (strstr(gauge_name, "solar") == NULL || strstr(gauge_name, "current") != NULL);
}

// Helper function to extract base name from gauge name (e.g., "starter" from "starter_voltage")
static void extract_base_name(const char* gauge_name, char* base_name, size_t base_name_size)
{
	const char* underscore = strstr(gauge_name, "_");
	if (underscore) {
		strncpy(base_name, gauge_name, underscore - gauge_name);
		base_name[underscore - gauge_name] = '\0';
	} else {
		strncpy(base_name, gauge_name, base_name_size - 1);
		base_name[base_name_size - 1] = '\0';
	}
}

// Programmatic state value getter using gauge_map
float power_monitor_get_state_values(int gauge_index, int field_type)
{
	// Bounds checking
	if (gauge_index < 0 || gauge_index >= 6) return 0.0f;

	// Get gauge info from the gauge_map
	extern gauge_map_entry_t gauge_map[POWER_MONITOR_GAUGE_COUNT];
	const gauge_map_entry_t* gauge = &gauge_map[gauge_index];

	char path[128];

	// Build the device state path based on field type and gauge name
	switch (field_type) {
		case FIELD_ALERT_LOW: {
			char base_name[32];
			extract_base_name(gauge->gauge_name, base_name, sizeof(base_name));
			snprintf(path, sizeof(path), "power_monitor.%s_alert_low_%s_%c",
				base_name,
				strstr(gauge->gauge_name, "voltage") ? "voltage" :
				strstr(gauge->gauge_name, "current") ? "current" : "power",
				strstr(gauge->gauge_name, "voltage") ? 'v' :
				strstr(gauge->gauge_name, "current") ? 'a' : 'w');
			return (float)device_state_get_int(path);
		}

		case FIELD_ALERT_HIGH: {
			char base_name[32];
			extract_base_name(gauge->gauge_name, base_name, sizeof(base_name));
			snprintf(path, sizeof(path), "power_monitor.%s_alert_high_%s_%c",
				base_name,
				strstr(gauge->gauge_name, "voltage") ? "voltage" :
				strstr(gauge->gauge_name, "current") ? "current" : "power",
				strstr(gauge->gauge_name, "voltage") ? 'v' :
				strstr(gauge->gauge_name, "current") ? 'a' : 'w');
			return (float)device_state_get_int(path);
		}

		case FIELD_GAUGE_LOW: {
			char base_name[32];
			extract_base_name(gauge->gauge_name, base_name, sizeof(base_name));
			snprintf(path, sizeof(path), "power_monitor.%s_min_%s_%c",
				base_name,
				strstr(gauge->gauge_name, "voltage") ? "voltage" :
				strstr(gauge->gauge_name, "current") ? "current" : "power",
				strstr(gauge->gauge_name, "voltage") ? 'v' :
				strstr(gauge->gauge_name, "current") ? 'a' : 'w');
			return device_state_get_float(path);
		}

		case FIELD_GAUGE_BASELINE: {
			if (!gauge_has_baseline(gauge->gauge_name)) return 0.0f; // No baseline for this gauge type
			char base_name[32];
			extract_base_name(gauge->gauge_name, base_name, sizeof(base_name));
			snprintf(path, sizeof(path), "power_monitor.%s_baseline_%s_%c",
				base_name,
				strstr(gauge->gauge_name, "voltage") ? "voltage" :
				strstr(gauge->gauge_name, "current") ? "current" : "power",
				strstr(gauge->gauge_name, "voltage") ? 'v' :
				strstr(gauge->gauge_name, "current") ? 'a' : 'w');
			return device_state_get_float(path);
		}

		case FIELD_GAUGE_HIGH: {
			char base_name[32];
			extract_base_name(gauge->gauge_name, base_name, sizeof(base_name));
			snprintf(path, sizeof(path), "power_monitor.%s_max_%s_%c",
				base_name,
				strstr(gauge->gauge_name, "voltage") ? "voltage" :
				strstr(gauge->gauge_name, "current") ? "current" : "power",
				strstr(gauge->gauge_name, "voltage") ? 'v' :
				strstr(gauge->gauge_name, "current") ? 'a' : 'w');
			return device_state_get_float(path);
		}

		default:
			return 0.0f;
	}
}

// Programmatic state value setter using gauge_map
void power_monitor_set_state_values(int gauge_index, int field_type, float value)
{
	// Bounds checking
	if (gauge_index < 0 || gauge_index >= 6) return;

	// Get gauge info from the gauge_map
	extern gauge_map_entry_t gauge_map[POWER_MONITOR_GAUGE_COUNT];
	const gauge_map_entry_t* gauge = &gauge_map[gauge_index];

	char path[128];

	// Build the device state path based on field type and gauge name
	switch (field_type) {
		case FIELD_ALERT_LOW: {
			char base_name[32];
			extract_base_name(gauge->gauge_name, base_name, sizeof(base_name));
			snprintf(path, sizeof(path), "power_monitor.%s_alert_low_%s_%c",
				base_name,
				strstr(gauge->gauge_name, "voltage") ? "voltage" :
				strstr(gauge->gauge_name, "current") ? "current" : "power",
				strstr(gauge->gauge_name, "voltage") ? 'v' :
				strstr(gauge->gauge_name, "current") ? 'a' : 'w');
			device_state_set_int(path, (int)value);
			break;
		}

		case FIELD_ALERT_HIGH: {
			char base_name[32];
			extract_base_name(gauge->gauge_name, base_name, sizeof(base_name));
			snprintf(path, sizeof(path), "power_monitor.%s_alert_high_%s_%c",
				base_name,
				strstr(gauge->gauge_name, "voltage") ? "voltage" :
				strstr(gauge->gauge_name, "current") ? "current" : "power",
				strstr(gauge->gauge_name, "voltage") ? 'v' :
				strstr(gauge->gauge_name, "current") ? 'a' : 'w');
			device_state_set_int(path, (int)value);
			break;
		}

		case FIELD_GAUGE_LOW: {
			char base_name[32];
			extract_base_name(gauge->gauge_name, base_name, sizeof(base_name));
			snprintf(path, sizeof(path), "power_monitor.%s_min_%s_%c",
				base_name,
				strstr(gauge->gauge_name, "voltage") ? "voltage" :
				strstr(gauge->gauge_name, "current") ? "current" : "power",
				strstr(gauge->gauge_name, "voltage") ? 'v' :
				strstr(gauge->gauge_name, "current") ? 'a' : 'w');
			device_state_set_float(path, value);
			break;
		}

		case FIELD_GAUGE_BASELINE: {
			if (!gauge_has_baseline(gauge->gauge_name)) return; // No baseline for this gauge type
			char base_name[32];
			extract_base_name(gauge->gauge_name, base_name, sizeof(base_name));
			snprintf(path, sizeof(path), "power_monitor.%s_baseline_%s_%c",
				base_name,
				strstr(gauge->gauge_name, "voltage") ? "voltage" :
				strstr(gauge->gauge_name, "current") ? "current" : "power",
				strstr(gauge->gauge_name, "voltage") ? 'v' :
				strstr(gauge->gauge_name, "current") ? 'a' : 'w');
			device_state_set_float(path, value);
			break;
		}

		case FIELD_GAUGE_HIGH: {
			char base_name[32];
			extract_base_name(gauge->gauge_name, base_name, sizeof(base_name));
			snprintf(path, sizeof(path), "power_monitor.%s_max_%s_%c",
				base_name,
				strstr(gauge->gauge_name, "voltage") ? "voltage" :
				strstr(gauge->gauge_name, "current") ? "current" : "power",
				strstr(gauge->gauge_name, "voltage") ? 'v' :
				strstr(gauge->gauge_name, "current") ? 'a' : 'w');
			device_state_set_float(path, value);
			break;
		}
	}
}

void power_monitor_refresh_all_data_callback(void)
{
	// This function can be used to refresh all data if needed
	// Currently not implemented as data is refreshed automatically
}