#include "voltage_alerts_config.h"
#include "../../state/device_state.h"
#include "../shared/gauges/bar_graph_gauge.h"
#include "../../screens/detail_screen/detail_screen.h"
#include "power-monitor.h"

// Voltage and Current gauge configurations (6 total gauges)
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
	// STARTER Battery Current
	{
		.name = "STARTER (A)",
		.unit = "A",
		.raw_min_value = -50.0f,   // RAW_MIN: negative current (charging)
		.raw_max_value = 50.0f,    // RAW_MAX: positive current (discharging)
		.fields = {
			[FIELD_ALERT_LOW] = {
				.name = "LOW",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = -30.0f, // Reasonable low alert for charging current
				.is_baseline = false
			},
			[FIELD_ALERT_HIGH] = {
				.name = "HIGH",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = 30.0f,  // Reasonable high alert for discharging current
				.is_baseline = false
			},
			[FIELD_GAUGE_LOW] = {
				.name = "LOW",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = -40.0f, // Reasonable gauge low for charging
				.is_baseline = false
			},
			[FIELD_GAUGE_BASELINE] = {
				.name = "BASE",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = 0.0f,   // Baseline is 0A (no current)
				.is_baseline = true
			},
			[FIELD_GAUGE_HIGH] = {
				.name = "HIGH",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = 40.0f,  // Reasonable gauge high for discharging
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
	// HOUSE Battery Current
	{
		.name = "HOUSE (A)",
		.unit = "A",
		.raw_min_value = -50.0f,   // RAW_MIN: negative current (charging)
		.raw_max_value = 50.0f,    // RAW_MAX: positive current (discharging)
		.fields = {
			[FIELD_ALERT_LOW] = {
				.name = "LOW",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = -30.0f, // Reasonable low alert for charging current
				.is_baseline = false
			},
			[FIELD_ALERT_HIGH] = {
				.name = "HIGH",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = 30.0f,  // Reasonable high alert for discharging current
				.is_baseline = false
			},
			[FIELD_GAUGE_LOW] = {
				.name = "LOW",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = -40.0f, // Reasonable gauge low for charging
				.is_baseline = false
			},
			[FIELD_GAUGE_BASELINE] = {
				.name = "BASE",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = 0.0f,   // Baseline is 0A (no current)
				.is_baseline = true
			},
			[FIELD_GAUGE_HIGH] = {
				.name = "HIGH",
				.min_value = -50.0f,   // Clamped to RAW_MIN
				.max_value = 50.0f,    // Clamped to RAW_MAX
				.default_value = 40.0f,  // Reasonable gauge high for discharging
				.is_baseline = false
			}
		},
		.has_baseline = true
	},
	// SOLAR Input
	{
		.name = "SOLAR (V)",
		.unit = "V",
		.raw_min_value = 0.0f,    // RAW_MIN: absolute minimum voltage
		.raw_max_value = 25.0f,   // RAW_MAX: absolute maximum voltage
		.fields = {
			[FIELD_ALERT_LOW] = {
				.name = "LOW",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 25.0f,    // Clamped to RAW_MAX
				.default_value = 12.0f,  // Reasonable low alert for solar
				.is_baseline = false
			},
			[FIELD_ALERT_HIGH] = {
				.name = "HIGH",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 25.0f,    // Clamped to RAW_MAX
				.default_value = 22.0f,  // Reasonable high alert for solar
				.is_baseline = false
			},
			[FIELD_GAUGE_LOW] = {
				.name = "LOW",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 25.0f,    // Clamped to RAW_MAX
				.default_value = 0.0f,   // Solar can be 0V
				.is_baseline = false
			},
			[FIELD_GAUGE_BASELINE] = {
				.name = "BASE",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 25.0f,    // Clamped to RAW_MAX
				.default_value = 0.0f,   // Solar baseline is 0V
				.is_baseline = true
			},
			[FIELD_GAUGE_HIGH] = {
				.name = "HIGH",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 25.0f,    // Clamped to RAW_MAX
				.default_value = 20.0f,  // Reasonable gauge high for solar
				.is_baseline = false
			}
		},
		.has_baseline = false
	},
	// SOLAR Input Current
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
				.default_value = 0.0f,   // Solar can be 0A
				.is_baseline = false
			},
			[FIELD_GAUGE_BASELINE] = {
				.name = "BASE",
				.min_value = 0.0f,     // Clamped to RAW_MIN
				.max_value = 20.0f,    // Clamped to RAW_MAX
				.default_value = 0.0f,   // Solar baseline is 0A
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
const alerts_modal_config_t voltage_alerts_config = {
	.gauge_count = 6,
	.gauges = (alerts_modal_gauge_config_t*)voltage_gauge_configs,
	.get_value_cb = voltage_get_value_callback,
	.set_value_cb = voltage_set_value_callback,
	.refresh_cb = voltage_refresh_callback,
	.modal_title = "Power Monitor Alerts & Gauges"
};

// Voltage and Current callback functions
float voltage_get_value_callback(int gauge_index, int field_type)
{
	// Gauges: 0=STARTER V, 1=STARTER A, 2=HOUSE V, 3=HOUSE A, 4=SOLAR V, 5=SOLAR A
	if (field_type == FIELD_ALERT_LOW) {
		if (gauge_index == 0) return (float)device_state_get_int("power_monitor.starter_alert_low_voltage_v");
		else if (gauge_index == 1) return (float)device_state_get_int("power_monitor.starter_alert_low_current_a");
		else if (gauge_index == 2) return (float)device_state_get_int("power_monitor.house_alert_low_voltage_v");
		else if (gauge_index == 3) return (float)device_state_get_int("power_monitor.house_alert_low_current_a");
		else if (gauge_index == 4) return (float)device_state_get_int("power_monitor.solar_alert_low_voltage_v");
		else if (gauge_index == 5) return (float)device_state_get_int("power_monitor.solar_alert_low_current_a");
	} else if (field_type == FIELD_ALERT_HIGH) {
		if (gauge_index == 0) return (float)device_state_get_int("power_monitor.starter_alert_high_voltage_v");
		else if (gauge_index == 1) return (float)device_state_get_int("power_monitor.starter_alert_high_current_a");
		else if (gauge_index == 2) return (float)device_state_get_int("power_monitor.house_alert_high_voltage_v");
		else if (gauge_index == 3) return (float)device_state_get_int("power_monitor.house_alert_high_current_a");
		else if (gauge_index == 4) return (float)device_state_get_int("power_monitor.solar_alert_high_voltage_v");
		else if (gauge_index == 5) return (float)device_state_get_int("power_monitor.solar_alert_high_current_a");
	} else if (field_type == FIELD_GAUGE_LOW) {
		if (gauge_index == 0) return device_state_get_float("power_monitor.starter_min_voltage_v");
		else if (gauge_index == 1) return device_state_get_float("power_monitor.starter_min_current_a");
		else if (gauge_index == 2) return device_state_get_float("power_monitor.house_min_voltage_v");
		else if (gauge_index == 3) return device_state_get_float("power_monitor.house_min_current_a");
		else if (gauge_index == 4) return device_state_get_float("power_monitor.solar_min_voltage_v");
		else if (gauge_index == 5) return device_state_get_float("power_monitor.solar_min_current_a");
	} else if (field_type == FIELD_GAUGE_BASELINE) {
		if (gauge_index == 0) return device_state_get_float("power_monitor.starter_baseline_voltage_v");
		else if (gauge_index == 1) return device_state_get_float("power_monitor.starter_baseline_current_a");
		else if (gauge_index == 2) return device_state_get_float("power_monitor.house_baseline_voltage_v");
		else if (gauge_index == 3) return device_state_get_float("power_monitor.house_baseline_current_a");
		else if (gauge_index == 4) return 0.0f; // Solar voltage has no baseline
		else if (gauge_index == 5) return device_state_get_float("power_monitor.solar_baseline_current_a");
	} else if (field_type == FIELD_GAUGE_HIGH) {
		if (gauge_index == 0) return device_state_get_float("power_monitor.starter_max_voltage_v");
		else if (gauge_index == 1) return device_state_get_float("power_monitor.starter_max_current_a");
		else if (gauge_index == 2) return device_state_get_float("power_monitor.house_max_voltage_v");
		else if (gauge_index == 3) return device_state_get_float("power_monitor.house_max_current_a");
		else if (gauge_index == 4) return device_state_get_float("power_monitor.solar_max_voltage_v");
		else if (gauge_index == 5) return device_state_get_float("power_monitor.solar_max_current_a");
	}
	return 0.0f;
}

void voltage_set_value_callback(int gauge_index, int field_type, float value)
{
	// Gauges: 0=STARTER V, 1=STARTER A, 2=HOUSE V, 3=HOUSE A, 4=SOLAR V, 5=SOLAR A
	if (field_type == FIELD_ALERT_LOW) {
		if (gauge_index == 0) device_state_set_int("power_monitor.starter_alert_low_voltage_v", (int)value);
		else if (gauge_index == 1) device_state_set_int("power_monitor.starter_alert_low_current_a", (int)value);
		else if (gauge_index == 2) device_state_set_int("power_monitor.house_alert_low_voltage_v", (int)value);
		else if (gauge_index == 3) device_state_set_int("power_monitor.house_alert_low_current_a", (int)value);
		else if (gauge_index == 4) device_state_set_int("power_monitor.solar_alert_low_voltage_v", (int)value);
		else if (gauge_index == 5) device_state_set_int("power_monitor.solar_alert_low_current_a", (int)value);
	} else if (field_type == FIELD_ALERT_HIGH) {
		if (gauge_index == 0) device_state_set_int("power_monitor.starter_alert_high_voltage_v", (int)value);
		else if (gauge_index == 1) device_state_set_int("power_monitor.starter_alert_high_current_a", (int)value);
		else if (gauge_index == 2) device_state_set_int("power_monitor.house_alert_high_voltage_v", (int)value);
		else if (gauge_index == 3) device_state_set_int("power_monitor.house_alert_high_current_a", (int)value);
		else if (gauge_index == 4) device_state_set_int("power_monitor.solar_alert_high_voltage_v", (int)value);
		else if (gauge_index == 5) device_state_set_int("power_monitor.solar_alert_high_current_a", (int)value);
	} else if (field_type == FIELD_GAUGE_LOW) {
		if (gauge_index == 0) device_state_set_float("power_monitor.starter_min_voltage_v", value);
		else if (gauge_index == 1) device_state_set_float("power_monitor.starter_min_current_a", value);
		else if (gauge_index == 2) device_state_set_float("power_monitor.house_min_voltage_v", value);
		else if (gauge_index == 3) device_state_set_float("power_monitor.house_min_current_a", value);
		else if (gauge_index == 4) device_state_set_float("power_monitor.solar_min_voltage_v", value);
		else if (gauge_index == 5) device_state_set_float("power_monitor.solar_min_current_a", value);
	} else if (field_type == FIELD_GAUGE_BASELINE) {
		if (gauge_index == 0) device_state_set_float("power_monitor.starter_baseline_voltage_v", value);
		else if (gauge_index == 1) device_state_set_float("power_monitor.starter_baseline_current_a", value);
		else if (gauge_index == 2) device_state_set_float("power_monitor.house_baseline_voltage_v", value);
		else if (gauge_index == 3) device_state_set_float("power_monitor.house_baseline_current_a", value);
		// Solar voltage baseline is not saved (always 0.0)
		else if (gauge_index == 5) device_state_set_float("power_monitor.solar_baseline_current_a", value);
	} else if (field_type == FIELD_GAUGE_HIGH) {
		if (gauge_index == 0) device_state_set_float("power_monitor.starter_max_voltage_v", value);
		else if (gauge_index == 1) device_state_set_float("power_monitor.starter_max_current_a", value);
		else if (gauge_index == 2) device_state_set_float("power_monitor.house_max_voltage_v", value);
		else if (gauge_index == 3) device_state_set_float("power_monitor.house_max_current_a", value);
		else if (gauge_index == 4) device_state_set_float("power_monitor.solar_max_voltage_v", value);
		else if (gauge_index == 5) device_state_set_float("power_monitor.solar_max_current_a", value);
	}
}

void voltage_refresh_callback(void)
{
	// Update power grid view gauge configuration
	extern void power_monitor_power_grid_view_update_configuration(void);
	power_monitor_power_grid_view_update_configuration();

	// Update starter voltage view gauge configuration
	extern void power_monitor_starter_voltage_view_update_configuration(void);
	power_monitor_starter_voltage_view_update_configuration();

	// Update detail screen gauge ranges
	extern void power_monitor_update_detail_gauge_ranges(void);
	power_monitor_update_detail_gauge_ranges();

	// Update all power monitor data and gauges (this includes detail screen gauges)
	extern void power_monitor_update_data_only(void);
	power_monitor_update_data_only();

	// Update alert flashing
	extern void power_monitor_power_grid_view_apply_alert_flashing(const power_monitor_data_t* data, int starter_lo, int starter_hi, int house_lo, int house_hi, int solar_lo, int solar_hi, bool blink_on);
	extern power_monitor_data_t* power_monitor_get_data(void);

	power_monitor_data_t* data = power_monitor_get_data();
	if (data) {
		// Get current alert thresholds from device state
		int starter_lo = device_state_get_int("power_monitor.starter_alert_low_voltage_v");
		int starter_hi = device_state_get_int("power_monitor.starter_alert_high_voltage_v");
		int house_lo = device_state_get_int("power_monitor.house_alert_low_voltage_v");
		int house_hi = device_state_get_int("power_monitor.house_alert_high_voltage_v");
		int solar_lo = device_state_get_int("power_monitor.solar_alert_low_voltage_v");
		int solar_hi = device_state_get_int("power_monitor.solar_alert_high_voltage_v");

		// Apply alert flashing with current thresholds
		power_monitor_power_grid_view_apply_alert_flashing(data, starter_lo, starter_hi, house_lo, house_hi, solar_lo, solar_hi, false);
	}
}
