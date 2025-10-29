#include <stdio.h>
#include <time.h>
#include "mock_data.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "../../displayModules/power-monitor/power-monitor.h"
#include "../../state/device_state.h"

static const char *TAG = "mock_data";

// Global mock data instance
static mock_data_t g_mock_data = {0};

// Configuration
static uint32_t g_update_interval_ms = MOCK_UPDATE_INTERVAL_MS;
static uint32_t g_sweep_duration_ms = MOCK_SWEEP_DURATION_MS;

// Random seed for consistent but varied data
static uint32_t g_random_seed = 0;

// Forward declarations for private update functions
static void update_power_monitor_mock_data(void);
static void update_temp_humidity_mock_data(void);
static void update_inclinometer_mock_data(void);
static void update_gps_mock_data(void);
static void update_coolant_temp_mock_data(void);
static void update_voltage_monitor_mock_data(void);
static void update_tpms_mock_data(void);
static void update_compressor_controller_mock_data(void);

void mock_data_init(void)
{
		printf("[I] mock_data: Initializing mock data component\n");

	// Initialize random seed
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	g_random_seed = (uint32_t)(ts.tv_sec * 1000000 + ts.tv_nsec / 1000);
	srand(g_random_seed);

	// Initialize all data structures
	memset(&g_mock_data, 0, sizeof(mock_data_t));

	// Set initial values
	g_mock_data.power_monitor.current_amps = 0.0f;
	g_mock_data.power_monitor.starter_battery_voltage = 12.6f;
	g_mock_data.power_monitor.starter_battery_current = 0.0f;
	g_mock_data.power_monitor.house_battery_voltage = 12.8f;
	g_mock_data.power_monitor.house_battery_current = 0.0f;
	g_mock_data.power_monitor.solar_input_voltage = 0.0f;
	g_mock_data.power_monitor.solar_input_current = 0.0f;
	g_mock_data.power_monitor.ignition_on = false;
	g_mock_data.power_monitor.starter_battery_connected = true;
	g_mock_data.power_monitor.house_battery_connected = true;
	g_mock_data.power_monitor.solar_input_connected = false;

	// Initialize error states
	g_mock_data.power_monitor.starter_voltage_error = false;
	g_mock_data.power_monitor.starter_current_error = false;
	g_mock_data.power_monitor.house_voltage_error = false;
	g_mock_data.power_monitor.house_current_error = false;
	g_mock_data.power_monitor.solar_voltage_error = false;
	g_mock_data.power_monitor.solar_current_error = false;
	g_mock_data.power_monitor.error_start_time = 0;
	g_mock_data.power_monitor.error_duration_ms = 0;

	g_mock_data.temp_humidity.temperature_celsius = 25.0f;
	g_mock_data.temp_humidity.humidity_percent = 50.0f;
	g_mock_data.temp_humidity.pressure_hpa = 1013.25f;
	g_mock_data.temp_humidity.is_connected = true;

	g_mock_data.inclinometer.pitch_degrees = 0.0f;
	g_mock_data.inclinometer.roll_degrees = 0.0f;
	g_mock_data.inclinometer.yaw_degrees = 0.0f;
	g_mock_data.inclinometer.acceleration_x = 0.0f;
	g_mock_data.inclinometer.acceleration_y = 0.0f;
	g_mock_data.inclinometer.acceleration_z = 1.0f; // 1g downward
	g_mock_data.inclinometer.is_calibrated = true;

	g_mock_data.gps.latitude = 40.7128; // New York coordinates
	g_mock_data.gps.longitude = -74.0060;
	g_mock_data.gps.altitude_meters = 10.0f;
	g_mock_data.gps.speed_kph = 0.0f;
	g_mock_data.gps.heading_degrees = 0.0f;
	g_mock_data.gps.satellites_visible = 8;
	g_mock_data.gps.has_fix = true;
	g_mock_data.gps.last_fix_time = time(NULL);

	g_mock_data.coolant_temp.engine_coolant_temp = 90.0f;
	g_mock_data.coolant_temp.transmission_temp = 80.0f;
	g_mock_data.coolant_temp.oil_temp = 100.0f;
	g_mock_data.coolant_temp.ambient_temp = 25.0f;
	g_mock_data.coolant_temp.engine_running = false;
	g_mock_data.coolant_temp.transmission_active = false;

	g_mock_data.voltage_monitor.main_battery_voltage = 12.6f;
	g_mock_data.voltage_monitor.alternator_voltage = 0.0f;
	g_mock_data.voltage_monitor.accessory_voltage = 12.4f;
	g_mock_data.voltage_monitor.charging_current = 0.0f;
	g_mock_data.voltage_monitor.alternator_active = false;
	g_mock_data.voltage_monitor.battery_charging = false;

	g_mock_data.tpms.front_left_pressure = 32.0f;
	g_mock_data.tpms.front_right_pressure = 32.0f;
	g_mock_data.tpms.rear_left_pressure = 30.0f;
	g_mock_data.tpms.rear_right_pressure = 30.0f;
	g_mock_data.tpms.front_left_temp = 25.0f;
	g_mock_data.tpms.front_right_temp = 25.0f;
	g_mock_data.tpms.rear_left_temp = 25.0f;
	g_mock_data.tpms.rear_right_temp = 25.0f;
	g_mock_data.tpms.front_left_connected = true;
	g_mock_data.tpms.front_right_connected = true;
	g_mock_data.tpms.rear_left_connected = true;
	g_mock_data.tpms.rear_right_connected = true;

	g_mock_data.compressor_controller.tank_pressure_psi = 0.0f;
	g_mock_data.compressor_controller.output_pressure_psi = 0.0f;
	g_mock_data.compressor_controller.motor_current_amps = 0.0f;
	g_mock_data.compressor_controller.motor_voltage = 12.6f;
	g_mock_data.compressor_controller.motor_temp = 25.0f;
	g_mock_data.compressor_controller.compressor_running = false;
	g_mock_data.compressor_controller.tank_full = false;
	g_mock_data.compressor_controller.safety_valve_open = false;
	g_mock_data.compressor_controller.runtime_hours = 0;

	g_mock_data.last_update_time = 0;
	g_mock_data.sweep_cycle_count = 0;
	g_mock_data.mock_data_enabled = true;

		printf("[I] mock_data: Mock data component initialized successfully\n");
}

void mock_data_update(void)
{
	if (!g_mock_data.mock_data_enabled) {
		return;
	}

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint32_t current_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000; // Convert to milliseconds

	// Update every interval
	if (current_time - g_mock_data.last_update_time < g_update_interval_ms) {
		return;
	}

	g_mock_data.last_update_time = current_time;
	g_mock_data.sweep_cycle_count++;

	// Log every 10 updates to avoid spam but show activity
	static int update_log_counter = 0;
	if (++update_log_counter % 10 == 0) {
		// Mock data update logging removed for cleaner output
	}

	// Update Power Monitor Data
	update_power_monitor_mock_data();

	// Update Temperature & Humidity Data
	update_temp_humidity_mock_data();

	// Update Inclinometer Data
	update_inclinometer_mock_data();

	// Update GPS Data
	update_gps_mock_data();

	// Update Coolant Temperature Data
	update_coolant_temp_mock_data();

	// Update Voltage Monitor Data
	update_voltage_monitor_mock_data();

	// Update TPMS Data
	update_tpms_mock_data();

	// Update Compressor Controller Data
	update_compressor_controller_mock_data();
}

void mock_data_enable(bool enable)
{
	g_mock_data.mock_data_enabled = enable;
		printf("[I] mock_data: Mock data %s\n", enable ? "enabled" : "disabled");
}

void mock_data_set_update_interval(uint32_t interval_ms)
{
	g_update_interval_ms = interval_ms;
		printf("[I] mock_data: Mock data update interval set to %lu ms\n", interval_ms);
}

// Data getter functions
mock_power_monitor_data_t* mock_data_get_power_monitor(void)
{
	return &g_mock_data.power_monitor;
}

mock_temp_humidity_data_t* mock_data_get_temp_humidity(void)
{
	return &g_mock_data.temp_humidity;
}

mock_inclinometer_data_t* mock_data_get_inclinometer(void)
{
	return &g_mock_data.inclinometer;
}

mock_gps_data_t* mock_data_get_gps(void)
{
	return &g_mock_data.gps;
}

mock_coolant_temp_data_t* mock_data_get_coolant_temp(void)
{
	return &g_mock_data.coolant_temp;
}

mock_voltage_monitor_data_t* mock_data_get_voltage_monitor(void)
{
	return &g_mock_data.voltage_monitor;
}

mock_tpms_data_t* mock_data_get_tpms(void)
{
	return &g_mock_data.tpms;
}

mock_compressor_controller_data_t* mock_data_get_compressor_controller(void)
{
	return &g_mock_data.compressor_controller;
}

// Utility functions
float mock_data_random_float(float min, float max)
{
	float random_value = (float)rand() / (float)RAND_MAX;
	return min + random_value * (max - min);
}

float mock_data_sweep_float(float min, float max, uint32_t cycle_count, uint32_t sweep_duration_ms)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint32_t current_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	float sweep_progress = (float)(current_time % sweep_duration_ms) / (float)sweep_duration_ms;

	// Create a smooth sine wave sweep
	float sine_value = sinf(sweep_progress * 2.0f * 3.14159265359f);
	return min + (max - min) * (0.5f + 0.5f * sine_value);
}

bool mock_data_random_bool(float true_probability)
{
	return mock_data_random_float(0.0f, 1.0f) < true_probability;
}

uint32_t mock_data_random_uint32(uint32_t min, uint32_t max)
{
	return min + (rand() % (max - min + 1));
}

// Private update functions
static void update_power_monitor_mock_data(void)
{
	// Get current time for error simulation
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint32_t current_time_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

	// Simulate realistic power monitoring scenarios
	g_mock_data.power_monitor.ignition_on = mock_data_random_bool(0.3f); // 30% chance ignition is on

	if (g_mock_data.power_monitor.ignition_on) {
		// Engine running - simulate charging scenario
		g_mock_data.power_monitor.current_amps = mock_data_sweep_float(-5.0f, 15.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
		// Different sweep duration for starter voltage - 6 second cycle
		g_mock_data.power_monitor.starter_battery_voltage = mock_data_sweep_float(10.0f, 18.0f, g_mock_data.sweep_cycle_count, 6000);
		g_mock_data.power_monitor.starter_battery_connected = true;
		// Different sweep duration for starter current - 7 second cycle
		g_mock_data.power_monitor.starter_battery_current = mock_data_sweep_float(-150.0f, 150.0f, g_mock_data.sweep_cycle_count, 7000);
	} else {
		// Engine off - simulate discharge scenario
		g_mock_data.power_monitor.current_amps = mock_data_sweep_float(-2.0f, 0.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
		// Different sweep duration for starter voltage - 6 second cycle
		g_mock_data.power_monitor.starter_battery_voltage = mock_data_sweep_float(10.0f, 18.0f, g_mock_data.sweep_cycle_count, 6000);
		g_mock_data.power_monitor.starter_battery_connected = mock_data_random_bool(0.95f);
		// Different sweep duration for starter current - 7 second cycle
		g_mock_data.power_monitor.starter_battery_current = mock_data_sweep_float(-150.0f, 150.0f, g_mock_data.sweep_cycle_count, 7000);
	}

	// House battery always connected and powered
	// Different sweep duration for house voltage - 8 second cycle
	g_mock_data.power_monitor.house_battery_voltage = mock_data_sweep_float(9.0f, 17.0f, g_mock_data.sweep_cycle_count, 8000);
	g_mock_data.power_monitor.house_battery_connected = true;
	// Different sweep duration for house current - 9 second cycle
	g_mock_data.power_monitor.house_battery_current = mock_data_sweep_float(-10.0f, 20.0f, g_mock_data.sweep_cycle_count, 9000);

	// Solar input - always generate some voltage for testing (simulate daytime)
	// Different sweep duration for solar voltage - 10 second cycle
	g_mock_data.power_monitor.solar_input_voltage = mock_data_sweep_float(18.0f, 22.0f, g_mock_data.sweep_cycle_count, 10000);
	g_mock_data.power_monitor.solar_input_connected = true;

	// Different sweep duration for solar current - 11 second cycle
	g_mock_data.power_monitor.solar_input_current = mock_data_sweep_float(2.0f, 8.0f, g_mock_data.sweep_cycle_count, 11000);

	// Power monitor mock data logging removed for cleaner output

	// Add a very small oscillating component to make changes more visible (minimal intensity)
	static uint32_t test_counter = 0;
	test_counter++;

	// Add a very small oscillating component to make changes more visible
	float oscillation = sinf((float)test_counter * 0.02f) * 0.1f; // Very reduced frequency and amplitude
	g_mock_data.power_monitor.current_amps += oscillation;
	g_mock_data.power_monitor.starter_battery_voltage += oscillation * 0.02f; // Very reduced impact
	g_mock_data.power_monitor.house_battery_voltage += oscillation * 0.02f; // Very reduced impact

	// Simulate sensor read errors (1-3 second duration)
	// Check if current error has expired
	if (g_mock_data.power_monitor.error_start_time > 0 &&
		(current_time_ms - g_mock_data.power_monitor.error_start_time) >= g_mock_data.power_monitor.error_duration_ms) {
		// Error expired, clear all errors
		g_mock_data.power_monitor.starter_voltage_error = false;
		g_mock_data.power_monitor.starter_current_error = false;
		g_mock_data.power_monitor.house_voltage_error = false;
		g_mock_data.power_monitor.house_current_error = false;
		g_mock_data.power_monitor.solar_voltage_error = false;
		g_mock_data.power_monitor.solar_current_error = false;
		g_mock_data.power_monitor.error_start_time = 0;
		g_mock_data.power_monitor.error_duration_ms = 0;
	}

	// Start new error if none is active (20% chance per update for testing)
	if (g_mock_data.power_monitor.error_start_time == 0 && mock_data_random_bool(0.20f)) {
		// Randomly select 1-3 sensors to have errors
		int num_errors = mock_data_random_uint32(1, 3);
		g_mock_data.power_monitor.error_start_time = current_time_ms;
		g_mock_data.power_monitor.error_duration_ms = mock_data_random_uint32(1000, 3000); // 1-3 seconds

		// Reset all error flags first
		g_mock_data.power_monitor.starter_voltage_error = false;
		g_mock_data.power_monitor.starter_current_error = false;
		g_mock_data.power_monitor.house_voltage_error = false;
		g_mock_data.power_monitor.house_current_error = false;
		g_mock_data.power_monitor.solar_voltage_error = false;
		g_mock_data.power_monitor.solar_current_error = false;

		// Randomly select which sensors to error
		bool sensors[6] = {false, false, false, false, false, false};
		for (int i = 0; i < num_errors; i++) {
			int sensor_idx;
			do {
				sensor_idx = mock_data_random_uint32(0, 5);
			} while (sensors[sensor_idx]);
			sensors[sensor_idx] = true;
		}

		g_mock_data.power_monitor.starter_voltage_error = sensors[0];
		g_mock_data.power_monitor.starter_current_error = sensors[1];
		g_mock_data.power_monitor.house_voltage_error = sensors[2];
		g_mock_data.power_monitor.house_current_error = sensors[3];
		g_mock_data.power_monitor.solar_voltage_error = sensors[4];
		g_mock_data.power_monitor.solar_current_error = sensors[5];
	}
}

static void update_temp_humidity_mock_data(void)
{
	// Simulate realistic environmental conditions
	g_mock_data.temp_humidity.temperature_celsius = mock_data_sweep_float(15.0f, 35.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);

	// Humidity inversely related to temperature (warmer = drier)
	float humidity_base = 80.0f - (g_mock_data.temp_humidity.temperature_celsius - 15.0f) * 1.5f;
	g_mock_data.temp_humidity.humidity_percent = mock_data_sweep_float(humidity_base - 10.0f, humidity_base + 10.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	g_mock_data.temp_humidity.humidity_percent = fmaxf(0.0f, fminf(100.0f, g_mock_data.temp_humidity.humidity_percent));

	// Pressure varies slightly
	g_mock_data.temp_humidity.pressure_hpa = mock_data_sweep_float(1000.0f, 1030.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);

	g_mock_data.temp_humidity.is_connected = mock_data_random_bool(0.98f);
}

static void update_inclinometer_mock_data(void)
{
	// Simulate vehicle movement and terrain
	g_mock_data.inclinometer.pitch_degrees = mock_data_sweep_float(-15.0f, 15.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	g_mock_data.inclinometer.roll_degrees = mock_data_sweep_float(-10.0f, 10.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	g_mock_data.inclinometer.yaw_degrees = mock_data_sweep_float(0.0f, 360.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);

	// Acceleration based on movement
	float movement_intensity = mock_data_sweep_float(0.0f, 1.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	g_mock_data.inclinometer.acceleration_x = mock_data_random_float(-0.2f, 0.2f) * movement_intensity;
	g_mock_data.inclinometer.acceleration_y = mock_data_random_float(-0.2f, 0.2f) * movement_intensity;
	g_mock_data.inclinometer.acceleration_z = 1.0f + mock_data_random_float(-0.1f, 0.1f) * movement_intensity;

	g_mock_data.inclinometer.is_calibrated = mock_data_random_bool(0.99f);
}

static void update_gps_mock_data(void)
{
	// Simulate GPS movement (slow drift)
	double lat_offset = sinf((float)g_mock_data.sweep_cycle_count * 0.1f) * 0.0001;
	double lon_offset = cosf((float)g_mock_data.sweep_cycle_count * 0.1f) * 0.0001;

	g_mock_data.gps.latitude = 40.7128 + lat_offset;
	g_mock_data.gps.longitude = -74.0060 + lon_offset;

	g_mock_data.gps.altitude_meters = mock_data_sweep_float(5.0f, 15.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	g_mock_data.gps.speed_kph = mock_data_sweep_float(0.0f, 5.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	g_mock_data.gps.heading_degrees = mock_data_sweep_float(0.0f, 360.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);

	g_mock_data.gps.satellites_visible = mock_data_random_uint32(6, 12);
	g_mock_data.gps.has_fix = mock_data_random_bool(0.95f);

	if (g_mock_data.gps.has_fix) {
		g_mock_data.gps.last_fix_time = time(NULL);
	}
}

static void update_coolant_temp_mock_data(void)
{
	// Simulate engine temperature cycles
	if (g_mock_data.coolant_temp.engine_running) {
		g_mock_data.coolant_temp.engine_coolant_temp = mock_data_sweep_float(85.0f, 110.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
		g_mock_data.coolant_temp.transmission_temp = mock_data_sweep_float(70.0f, 120.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
		g_mock_data.coolant_temp.oil_temp = mock_data_sweep_float(90.0f, 130.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	} else {
		// Engine cooling down
		g_mock_data.coolant_temp.engine_coolant_temp = mock_data_sweep_float(25.0f, 40.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
		g_mock_data.coolant_temp.transmission_temp = mock_data_sweep_float(25.0f, 35.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
		g_mock_data.coolant_temp.oil_temp = mock_data_sweep_float(25.0f, 35.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	}

	g_mock_data.coolant_temp.ambient_temp = mock_data_sweep_float(20.0f, 30.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	g_mock_data.coolant_temp.engine_running = mock_data_random_bool(0.4f);
	g_mock_data.coolant_temp.transmission_active = g_mock_data.coolant_temp.engine_running && mock_data_random_bool(0.7f);
}

static void update_voltage_monitor_mock_data(void)
{
	// Simulate electrical system behavior
	if (g_mock_data.coolant_temp.engine_running) {
		// Engine running - alternator active
		g_mock_data.voltage_monitor.alternator_voltage = mock_data_sweep_float(13.8f, 14.4f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
		g_mock_data.voltage_monitor.charging_current = mock_data_sweep_float(2.0f, 8.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
		g_mock_data.voltage_monitor.alternator_active = true;
		g_mock_data.voltage_monitor.battery_charging = true;
	} else {
		// Engine off - no charging
		g_mock_data.voltage_monitor.alternator_voltage = 0.0f;
		g_mock_data.voltage_monitor.charging_current = mock_data_sweep_float(-1.0f, 0.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
		g_mock_data.voltage_monitor.alternator_active = false;
		g_mock_data.voltage_monitor.battery_charging = false;
	}

	g_mock_data.voltage_monitor.main_battery_voltage = mock_data_sweep_float(12.0f, 12.8f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	g_mock_data.voltage_monitor.accessory_voltage = g_mock_data.voltage_monitor.main_battery_voltage - 0.2f;
}

static void update_tpms_mock_data(void)
{
	// Simulate tire pressure and temperature variations
	float base_pressure = 32.0f;
	float base_temp = 25.0f;

	// Pressure varies with temperature and load
	g_mock_data.tpms.front_left_pressure = base_pressure + mock_data_sweep_float(-2.0f, 3.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	g_mock_data.tpms.front_right_pressure = base_pressure + mock_data_sweep_float(-2.0f, 3.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	g_mock_data.tpms.rear_left_pressure = base_pressure + mock_data_sweep_float(-1.0f, 2.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	g_mock_data.tpms.rear_right_pressure = base_pressure + mock_data_sweep_float(-1.0f, 2.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);

	// Temperature varies with ambient and driving
	g_mock_data.tpms.front_left_temp = base_temp + mock_data_sweep_float(-5.0f, 15.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	g_mock_data.tpms.front_right_temp = base_temp + mock_data_sweep_float(-5.0f, 15.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	g_mock_data.tpms.rear_left_temp = base_temp + mock_data_sweep_float(-3.0f, 10.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	g_mock_data.tpms.rear_right_temp = base_temp + mock_data_sweep_float(-3.0f, 10.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);

	// Connection status (rarely disconnected)
	g_mock_data.tpms.front_left_connected = mock_data_random_bool(0.99f);
	g_mock_data.tpms.front_right_connected = mock_data_random_bool(0.99f);
	g_mock_data.tpms.rear_left_connected = mock_data_random_bool(0.99f);
	g_mock_data.tpms.rear_right_connected = mock_data_random_bool(0.99f);
}

static void update_compressor_controller_mock_data(void)
{
	// Simulate compressor operation cycles
	if (g_mock_data.compressor_controller.compressor_running) {
		// Compressor running - building pressure
		g_mock_data.compressor_controller.tank_pressure_psi = mock_data_sweep_float(50.0f, 140.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
		g_mock_data.compressor_controller.output_pressure_psi = g_mock_data.compressor_controller.tank_pressure_psi - 5.0f;
		g_mock_data.compressor_controller.motor_current_amps = mock_data_sweep_float(15.0f, 25.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
		g_mock_data.compressor_controller.motor_temp = mock_data_sweep_float(40.0f, 80.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);

		// Check if tank is full
		if (g_mock_data.compressor_controller.tank_pressure_psi >= 140.0f) {
			g_mock_data.compressor_controller.tank_full = true;
			g_mock_data.compressor_controller.compressor_running = false;
		}
	} else {
		// Compressor stopped - pressure slowly dropping
		g_mock_data.compressor_controller.tank_pressure_psi = mock_data_sweep_float(0.0f, 50.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
		g_mock_data.compressor_controller.output_pressure_psi = 0.0f;
		g_mock_data.compressor_controller.motor_current_amps = 0.0f;
		g_mock_data.compressor_controller.motor_temp = mock_data_sweep_float(25.0f, 35.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);

		// Check if tank needs refilling
		if (g_mock_data.compressor_controller.tank_pressure_psi <= 20.0f) {
			g_mock_data.compressor_controller.tank_full = false;
			g_mock_data.compressor_controller.compressor_running = mock_data_random_bool(0.3f); // 30% chance to start
		}
	}

	g_mock_data.compressor_controller.motor_voltage = mock_data_sweep_float(12.0f, 13.0f, g_mock_data.sweep_cycle_count, g_sweep_duration_ms);
	g_mock_data.compressor_controller.safety_valve_open = g_mock_data.compressor_controller.tank_pressure_psi > 145.0f;

	// Increment runtime when compressor is running
	if (g_mock_data.compressor_controller.compressor_running) {
		g_mock_data.compressor_controller.runtime_hours += 1; // Increment every update cycle
	}
}

// Function to write mock data directly to state objects
void mock_data_write_to_state_objects(void)
{
	// Only write if mock data is the current data source
	if (data_config_get_source() != DATA_SOURCE_MOCK) {
		return;
	}

	// Rate limiting - only update every 100ms for more responsive updates
	static uint32_t last_write_time = 0;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint32_t current_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	if (current_time - last_write_time < 100) {
		return;
	}
	last_write_time = current_time;

	// Write power monitor data to state
	power_monitor_data_t *power_data = power_monitor_get_data();
	if (!power_data) {
		printf("[W] mock_data: power_data is NULL, skipping state write\n");
		return;
	}

	mock_power_monitor_data_t *mock_power = mock_data_get_power_monitor();
	if (!mock_power) {
		printf("[W] mock_data: mock_power is NULL, skipping state write\n");
		return;
	}

	// Safely update power data with bounds checking
	power_data->current_amps = mock_power->current_amps;
	power_data->is_connected = mock_power->starter_battery_connected || mock_power->house_battery_connected;
	power_data->is_active = true;
	power_data->last_update_ms = current_time;

	// Update battery data with bounds checking
	power_data->starter_battery.voltage.value = mock_power->starter_battery_voltage;
	power_data->starter_battery.voltage.error = mock_power->starter_voltage_error;
	power_data->starter_battery.current.value = mock_power->starter_battery_current;
	power_data->starter_battery.current.error = mock_power->starter_current_error;
	power_data->starter_battery.is_connected = mock_power->starter_battery_connected;
	power_data->starter_battery.is_charging = mock_power->ignition_on;

	power_data->house_battery.voltage.value = mock_power->house_battery_voltage;
	power_data->house_battery.voltage.error = mock_power->house_voltage_error;
	power_data->house_battery.current.value = mock_power->house_battery_current;
	power_data->house_battery.current.error = mock_power->house_current_error;
	power_data->house_battery.is_connected = mock_power->house_battery_connected;
	power_data->house_battery.is_charging = mock_power->solar_input_connected;

	power_data->solar_input.voltage.value = mock_power->solar_input_voltage;
	power_data->solar_input.voltage.error = mock_power->solar_voltage_error;
	power_data->solar_input.current.value = mock_power->solar_input_current;
	power_data->solar_input.current.error = mock_power->solar_current_error;
	power_data->solar_input.is_connected = mock_power->solar_input_connected;
	power_data->solar_input.is_charging = mock_power->solar_input_connected && mock_power->solar_input_voltage > 0.0f;

	power_data->ignition_on = mock_power->ignition_on;

	// Debug logging to verify data updates
	// printf("[D] TAG: "Mock data written to power monitor: %.1fA, %.1fV", power_data->current_amps, power_data->starter_battery.voltage\n");
}
