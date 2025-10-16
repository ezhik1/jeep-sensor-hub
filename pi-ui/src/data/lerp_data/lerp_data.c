#include <stdio.h>
#include <time.h>
#include "lerp_data.h"

#include <string.h>
#include <math.h>
#include "../../displayModules/power-monitor/power-monitor.h"

static const char *TAG = "lerp_data";

// Global LERP data instance
static lerp_power_monitor_data_t g_lerp_data = {0};
static bool g_lerp_initialized = false;

// Initialize LERP data system
void lerp_data_init(void)
{
	if (g_lerp_initialized) {

		return;
	}

	// Initialize all LERP values to 0
	lerp_value_init(&g_lerp_data.starter_voltage, 0.0f);
	lerp_value_init(&g_lerp_data.starter_current, 0.0f);
	lerp_value_init(&g_lerp_data.house_voltage, 0.0f);
	lerp_value_init(&g_lerp_data.house_current, 0.0f);
	lerp_value_init(&g_lerp_data.solar_voltage, 0.0f);
	lerp_value_init(&g_lerp_data.solar_current, 0.0f);

	g_lerp_initialized = true;
}

// Update all LERP values (call this every frame)
void lerp_data_update(void)
{
	if (!g_lerp_initialized) {

		printf("[W] lerp_data: LERP data not initialized\n");
		return;
	}

	// Update all LERP values
	lerp_value_update(&g_lerp_data.starter_voltage);
	lerp_value_update(&g_lerp_data.starter_current);
	lerp_value_update(&g_lerp_data.house_voltage);
	lerp_value_update(&g_lerp_data.house_current);
	lerp_value_update(&g_lerp_data.solar_voltage);
	lerp_value_update(&g_lerp_data.solar_current);
}

// Set target values from raw sensor data
void lerp_data_set_targets(const void* raw_data)
{
	if (!g_lerp_initialized) {

		printf("[W] lerp_data: LERP data not initialized\n");
		return;
	}

	// Cast to power monitor data structure
	const power_monitor_data_t* data = (const power_monitor_data_t*)raw_data;

	// Set target values
	lerp_value_set_target(&g_lerp_data.starter_voltage, data->starter_battery.voltage);
	lerp_value_set_target(&g_lerp_data.starter_current, data->starter_battery.current);
	lerp_value_set_target(&g_lerp_data.house_voltage, data->house_battery.voltage);
	lerp_value_set_target(&g_lerp_data.house_current, data->house_battery.current);
	lerp_value_set_target(&g_lerp_data.solar_voltage, data->solar_input.voltage);
	lerp_value_set_target(&g_lerp_data.solar_current, data->solar_input.current);
}

// Get current interpolated values
void lerp_data_get_current(lerp_power_monitor_data_t* output)
{
	if (!g_lerp_initialized || !output) {

		printf("[W] lerp_data: LERP data not initialized or output is NULL\n");
		return;
	}

	// Copy current interpolated values
	output->starter_voltage = g_lerp_data.starter_voltage;
	output->starter_current = g_lerp_data.starter_current;
	output->house_voltage = g_lerp_data.house_voltage;
	output->house_current = g_lerp_data.house_current;
	output->solar_voltage = g_lerp_data.solar_voltage;
	output->solar_current = g_lerp_data.solar_current;
}

// Cleanup LERP data system
void lerp_data_cleanup(void)
{
	if (!g_lerp_initialized) {
		return;
	}

	memset(&g_lerp_data, 0, sizeof(g_lerp_data));
	g_lerp_initialized = false;
}

// Initialize a single LERP value
void lerp_value_init(lerp_value_t* lerp_val, float initial_value)
{
	if (!lerp_val) {

		printf("[E] lerp_data: LERP value pointer is NULL\n");
		return;
	}

	lerp_val->raw_value = initial_value;
	lerp_val->display_value = initial_value;
	lerp_val->target_value = initial_value;
	lerp_val->is_interpolating = false;
	lerp_val->last_update_ms = 0;
}

// Set target value for interpolation
void lerp_value_set_target(lerp_value_t* lerp_val, float target_value)
{
	if (!lerp_val) {

		printf("[E] lerp_data: LERP value pointer is NULL\n");
		return;
	}

	// Update raw value immediately
	lerp_val->raw_value = target_value;
	lerp_val->target_value = target_value;

	// Check if we need to start interpolating
	float diff = fabsf(lerp_val->display_value - lerp_val->target_value);
	if (diff > LERP_THRESHOLD) {
		lerp_val->is_interpolating = true;
	}
}

// Update a single LERP value
void lerp_value_update(lerp_value_t* lerp_val)
{
	if (!lerp_val || !lerp_val->is_interpolating) {
		return;
	}

	// Calculate interpolation
	float diff = lerp_val->target_value - lerp_val->display_value;
	lerp_val->display_value += diff * LERP_SPEED;

	// Check if we're close enough to stop interpolating
	if (fabsf(diff) < LERP_THRESHOLD) {
		lerp_val->display_value = lerp_val->target_value;
		lerp_val->is_interpolating = false;
	}

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	lerp_val->last_update_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// Get raw value (always current sensor reading)
float lerp_value_get_raw(const lerp_value_t* lerp_val)
{
	if (!lerp_val) {

		printf("[E] lerp_data: LERP value pointer is NULL\n");
		return 0.0f;
	}

	return lerp_val->raw_value;
}

// Get display value (smoothly interpolated)
float lerp_value_get_display(const lerp_value_t* lerp_val)
{
	if (!lerp_val) {

		printf("[E] lerp_data: LERP value pointer is NULL\n");
		return 0.0f;
	}

	return lerp_val->display_value;
}

// Check if value is currently interpolating
bool lerp_value_is_interpolating(const lerp_value_t* lerp_val)
{
	if (!lerp_val) {

		return false;
	}

	return lerp_val->is_interpolating;
}
