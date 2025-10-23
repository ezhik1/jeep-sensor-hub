#ifndef MOCK_DATA_H
#define MOCK_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "../config.h"

#ifdef __cplusplus
extern "C" {
#endif

// Mock data configuration
#define MOCK_UPDATE_INTERVAL_MS 1000  // Update every 1000ms for maximum stability
#define MOCK_SWEEP_DURATION_MS 5000   // Complete sweep every 5 seconds

// Power Monitor Mock Data
typedef struct {
	float current_amps;           // -50A to +50A
	float starter_battery_voltage; // 10V to 16V
	float starter_battery_current; // -50A to +50A
	float house_battery_voltage;   // 10V to 16V
	float house_battery_current;   // -50A to +50A
	float solar_input_voltage;     // 0V to 24V
	float solar_input_current;     // 0A to 10A (always positive)
	bool ignition_on;
	bool starter_battery_connected;
	bool house_battery_connected;
	bool solar_input_connected;

	// Sensor error states
	bool starter_voltage_error;
	bool starter_current_error;
	bool house_voltage_error;
	bool house_current_error;
	bool solar_voltage_error;
	bool solar_current_error;
	uint32_t error_start_time;    // When current error started
	uint32_t error_duration_ms;   // How long error should last
} mock_power_monitor_data_t;

// Temperature & Humidity Mock Data
typedef struct {
	float temperature_celsius;     // -40°C to +85°C
	float humidity_percent;        // 0% to 100%
	float pressure_hpa;            // 800hPa to 1200hPa
	bool is_connected;
} mock_temp_humidity_data_t;

// Inclinometer Mock Data
typedef struct {
	float pitch_degrees;           // -90° to +90°
	float roll_degrees;            // -180° to +180°
	float yaw_degrees;             // 0° to 360°
	float acceleration_x;           // -2g to +2g
	float acceleration_y;           // -2g to +2g
	float acceleration_z;           // -2g to +2g
	bool is_calibrated;
} mock_inclinometer_data_t;

// GPS Mock Data
typedef struct {
	double latitude;               // -90.0 to +90.0
	double longitude;              // -180.0 to +180.0
	float altitude_meters;         // -1000m to +10000m
	float speed_kph;               // 0 to 200 km/h
	float heading_degrees;         // 0° to 360°
	uint8_t satellites_visible;    // 0 to 32
	bool has_fix;
	time_t last_fix_time;
} mock_gps_data_t;

// Coolant Temperature Mock Data
typedef struct {
	float engine_coolant_temp;     // 60°C to 120°C
	float transmission_temp;        // 50°C to 150°C
	float oil_temp;                // 80°C to 150°C
	float ambient_temp;            // -40°C to +50°C
	bool engine_running;
	bool transmission_active;
} mock_coolant_temp_data_t;

// Voltage Monitor Mock Data
typedef struct {
	float main_battery_voltage;    // 10V to 16V
	float alternator_voltage;      // 12V to 16V
	float accessory_voltage;       // 10V to 16V
	float charging_current;        // -50A to +50A
	bool alternator_active;
	bool battery_charging;
} mock_voltage_monitor_data_t;

// TPMS Mock Data
typedef struct {
	float front_left_pressure;     // 20PSI to 50PSI
	float front_right_pressure;    // 20PSI to 50PSI
	float rear_left_pressure;      // 20PSI to 50PSI
	float rear_right_pressure;     // 20PSI to 50PSI
	float front_left_temp;         // -40°C to +85°C
	float front_right_temp;        // -40°C to +85°C
	float rear_left_temp;          // -40°C to +85°C
	float rear_right_temp;         // -40°C to +85°C
	bool front_left_connected;
	bool front_right_connected;
	bool rear_left_connected;
	bool rear_right_connected;
} mock_tpms_data_t;

// Compressor Controller Mock Data
typedef struct {
	float tank_pressure_psi;       // 0PSI to 150PSI
	float output_pressure_psi;     // 0PSI to 150PSI
	float motor_current_amps;      // 0A to 30A
	float motor_voltage;           // 10V to 16V
	float motor_temp;              // 20°C to 100°C
	bool compressor_running;
	bool tank_full;
	bool safety_valve_open;
	uint32_t runtime_hours;
} mock_compressor_controller_data_t;

// Master Mock Data Structure
typedef struct {
	mock_power_monitor_data_t power_monitor;
	mock_temp_humidity_data_t temp_humidity;
	mock_inclinometer_data_t inclinometer;
	mock_gps_data_t gps;
	mock_coolant_temp_data_t coolant_temp;
	mock_voltage_monitor_data_t voltage_monitor;
	mock_tpms_data_t tpms;
	mock_compressor_controller_data_t compressor_controller;

	// System state
	uint32_t last_update_time;
	uint32_t sweep_cycle_count;
	bool mock_data_enabled;
} mock_data_t;

// Function declarations
void mock_data_init(void);
void mock_data_update(void);
void mock_data_enable(bool enable);
void mock_data_set_update_interval(uint32_t interval_ms);

// Data getter functions
mock_power_monitor_data_t* mock_data_get_power_monitor(void);
mock_temp_humidity_data_t* mock_data_get_temp_humidity(void);
mock_inclinometer_data_t* mock_data_get_inclinometer(void);
mock_gps_data_t* mock_data_get_gps(void);
mock_coolant_temp_data_t* mock_data_get_coolant_temp(void);
mock_voltage_monitor_data_t* mock_data_get_voltage_monitor(void);
mock_tpms_data_t* mock_data_get_tpms(void);
mock_compressor_controller_data_t* mock_data_get_compressor_controller(void);

// State writing functions
void mock_data_write_to_state_objects(void);

// Utility functions
float mock_data_random_float(float min, float max);
float mock_data_sweep_float(float min, float max, uint32_t cycle_count, uint32_t sweep_duration_ms);
bool mock_data_random_bool(float true_probability);
uint32_t mock_data_random_uint32(uint32_t min, uint32_t max);

#ifdef __cplusplus
}
#endif

#endif // MOCK_DATA_H
