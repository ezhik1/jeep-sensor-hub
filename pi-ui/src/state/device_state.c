#include "device_state.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

static const char *TAG = "device_state";

// Global state instance
device_state_t g_device_state = {0};

// Helper function to get module screen view state
static module_screen_view_state_t* get_module_screen_view_state(const char *module_name)
{
	if (!module_name) {
		printf("[E] device_state: Module name is NULL\n\n\n");
		return NULL;
	}

	if (strcmp(module_name, "power-monitor") == 0) {
		return &g_device_state.power_monitor.screen_view_state;
	} else if (strcmp(module_name, "other") == 0) {
		return &g_device_state.other_modules.screen_view_state;
	} else {
		printf("[W] device_state: Unknown module: %s, using other_modules\n", module_name);
		return &g_device_state.other_modules.screen_view_state;
	}
}

// Default state values
static const device_state_t default_state = {
	.system_initialized = false,
	.last_save_timestamp = 0,
	.screen_navigation = {
		.current_screen = SCREEN_HOME,
		.requested_screen = SCREEN_HOME,
		.current_module = {0},
		.requested_module = {0},
		.screen_transition_pending = false,
		.current_view_index = 0,
		.available_views_count = 0,
		.view_is_visible = false,
		.requested_view_index = 0,
		.view_transition_pending = false,
		.view_cycling_in_progress = false,
	},
	.power_monitor = {
		.screen_view_state = {
			.current_screen = SCREEN_HOME,
			.last_screen = SCREEN_HOME,
			.current_view_index = 0,
			.available_views_count = 0,
			.view_is_visible = false,
			.requested_view_index = 0,
			.view_transition_pending = false,
			.view_cycling_in_progress = false,
		},
		.current_view_type = POWER_MONITOR_VIEW_BAR_GRAPH, // Default to bar graph view
		.is_initialized = false,
		// Alert thresholds (when to flash red)
		.starter_alert_low_voltage_v = 11,
		.starter_alert_high_voltage_v = 15,
		.house_alert_low_voltage_v = 11,
		.house_alert_high_voltage_v = 15,
		.solar_alert_low_voltage_v = 10,
		.solar_alert_high_voltage_v = 20,
		// Bar graph display ranges (what range to show on gauge)
		.starter_min_voltage_tenths = 101, // 10.1V (14.1V - 4V)
		.starter_max_voltage_tenths = 181, // 18.1V (14.1V + 4V)
		.starter_baseline_voltage_tenths = 141, // 14.1V
		.house_min_voltage_tenths = 92, // 9.2V (13.2V - 4V)
		.house_max_voltage_tenths = 172, // 17.2V (13.2V + 4V)
		.house_baseline_voltage_tenths = 132, // 13.2V
		.solar_min_voltage_tenths = 100, // 10.0V
		.solar_max_voltage_tenths = 200 // 20.0V
	},
	.other_modules = {
		.screen_view_state = {
			.current_screen = SCREEN_HOME,
			.last_screen = SCREEN_HOME,
			.current_view_index = 0,
			.available_views_count = 0,
			.view_is_visible = false,
			.requested_view_index = 0,
			.view_transition_pending = false,
			.view_cycling_in_progress = false,
		},
		.is_initialized = false,
	},
	.brightness_level = 80, // Default brightness
	.auto_save_enabled = true,
	.auto_save_interval_ms = 30000 // 30 seconds
};

// Async save debounce timer and impl
static pthread_t s_state_save_timer = 0;
static volatile bool s_state_save_pending = false;
static pthread_t s_state_save_task = 0;
static void device_state_save_impl(void)
{
	printf("[I] device_state: Saving device state (impl)\n");

	// Update timestamp
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	g_device_state.last_save_timestamp = ts.tv_sec * 1000 + ts.tv_nsec / 1000000; // Convert to milliseconds

	FILE* f = fopen("/tmp/jeep_sensor_hub_state", "w");
	if (!f) {
		printf("[E] device_state: Failed to open state file for writing\n\n");
		return;
	}

	// Persist power monitor alert thresholds and view
	fprintf(f, "pm_st_alert_lo=%d\n", g_device_state.power_monitor.starter_alert_low_voltage_v);
	fprintf(f, "pm_st_alert_hi=%d\n", g_device_state.power_monitor.starter_alert_high_voltage_v);
	fprintf(f, "pm_ho_alert_lo=%d\n", g_device_state.power_monitor.house_alert_low_voltage_v);
	fprintf(f, "pm_ho_alert_hi=%d\n", g_device_state.power_monitor.house_alert_high_voltage_v);
	fprintf(f, "pm_so_alert_lo=%d\n", g_device_state.power_monitor.solar_alert_low_voltage_v);
	fprintf(f, "pm_so_alert_hi=%d\n", g_device_state.power_monitor.solar_alert_high_voltage_v);
	fprintf(f, "pm_view=%d\n", (int)g_device_state.power_monitor.current_view_type);

	// Persist bar graph gauge min/max/baseline values
	fprintf(f, "pm_st_min=%d\n", g_device_state.power_monitor.starter_min_voltage_tenths);
	fprintf(f, "pm_st_max=%d\n", g_device_state.power_monitor.starter_max_voltage_tenths);
	fprintf(f, "pm_st_base=%d\n", g_device_state.power_monitor.starter_baseline_voltage_tenths);
	fprintf(f, "pm_ho_min=%d\n", g_device_state.power_monitor.house_min_voltage_tenths);
	fprintf(f, "pm_ho_max=%d\n", g_device_state.power_monitor.house_max_voltage_tenths);
	fprintf(f, "pm_ho_base=%d\n", g_device_state.power_monitor.house_baseline_voltage_tenths);
	fprintf(f, "pm_so_min=%d\n", g_device_state.power_monitor.solar_min_voltage_tenths);
	fprintf(f, "pm_so_max=%d\n", g_device_state.power_monitor.solar_max_voltage_tenths);

	// Persist brightness and autosave settings
	fprintf(f, "bright=%d\n", g_device_state.brightness_level);
	fprintf(f, "as_en=%d\n", g_device_state.auto_save_enabled ? 1 : 0);
	fprintf(f, "as_int=%d\n", g_device_state.auto_save_interval_ms);

	fclose(f);

	printf("[I] device_state: State saved (timestamp: %lu)\n", g_device_state.last_save_timestamp);
}

static void* device_state_save_timer_cb(void *arg)
{
	(void)arg;
	// For simplicity, just set a flag that the save task can check
	// In a real implementation, you might use condition variables or semaphores
	return NULL;
}

static void* state_save_task(void *arg)
{
	(void)arg;
	for (;;) {
		// Wait with timeout so we never block indefinitely (resilience)
		sleep(5);
		if (s_state_save_pending) {
			device_state_save_impl();
			s_state_save_pending = false;
		}
	}
	return NULL;
}

void device_state_init(void)
{
	printf("[I] device_state: Initializing device state\n");

	// Initialize NVS for persistence
	// Initialize file-based storage
	// No initialization needed for file-based storage

	// Create debounce timer for async saves (200ms) - using simple timer
	s_state_save_timer = 0; // Not needed for file-based storage

	// Create dedicated save task to perform file writes off the main thread
	pthread_create(&s_state_save_task, NULL, state_save_task, NULL);

	// Load state from persistent storage
	device_state_load();

	// Mark as initialized
	g_device_state.system_initialized = true;

	printf("[I] device_state: Device state initialized - current screen: %d, requested screen: %d\n",
		g_device_state.screen_navigation.current_screen,
		g_device_state.screen_navigation.requested_screen);
}

void device_state_save(void)
{
	// Schedule async save (debounced)
	s_state_save_pending = true;
	if (s_state_save_timer) {
		// Timer not needed for file-based storage
	}
}

void device_state_load(void)
{
	printf("[I] device_state: Loading device state\n\n");

	// Start with defaults
	memcpy(&g_device_state, &default_state, sizeof(device_state_t));

	FILE* f = fopen("/tmp/jeep_sensor_hub_state", "r");
	if (!f) {
		printf("[W] device_state: State file not found, using defaults\n\n");
		return;
	}

	// Read key-value pairs from file
	char line[256];
	while (fgets(line, sizeof(line), f)) {
		char key[64];
		int value;
		if (sscanf(line, "%63[^=]=%d", key, &value) == 2) {
			if (strcmp(key, "pm_st_alert_lo") == 0) g_device_state.power_monitor.starter_alert_low_voltage_v = value;
			else if (strcmp(key, "pm_st_alert_hi") == 0) g_device_state.power_monitor.starter_alert_high_voltage_v = value;
			else if (strcmp(key, "pm_ho_alert_lo") == 0) g_device_state.power_monitor.house_alert_low_voltage_v = value;
			else if (strcmp(key, "pm_ho_alert_hi") == 0) g_device_state.power_monitor.house_alert_high_voltage_v = value;
			else if (strcmp(key, "pm_so_alert_lo") == 0) g_device_state.power_monitor.solar_alert_low_voltage_v = value;
			else if (strcmp(key, "pm_so_alert_hi") == 0) g_device_state.power_monitor.solar_alert_high_voltage_v = value;
			else if (strcmp(key, "pm_view") == 0) g_device_state.power_monitor.current_view_type = (power_monitor_view_type_t)value;
			else if (strcmp(key, "pm_st_min") == 0) g_device_state.power_monitor.starter_min_voltage_tenths = value;
			else if (strcmp(key, "pm_st_max") == 0) g_device_state.power_monitor.starter_max_voltage_tenths = value;
			else if (strcmp(key, "pm_st_base") == 0) g_device_state.power_monitor.starter_baseline_voltage_tenths = value;
			else if (strcmp(key, "pm_ho_min") == 0) g_device_state.power_monitor.house_min_voltage_tenths = value;
			else if (strcmp(key, "pm_ho_max") == 0) g_device_state.power_monitor.house_max_voltage_tenths = value;
			else if (strcmp(key, "pm_ho_base") == 0) g_device_state.power_monitor.house_baseline_voltage_tenths = value;
			else if (strcmp(key, "pm_so_min") == 0) g_device_state.power_monitor.solar_min_voltage_tenths = value;
			else if (strcmp(key, "pm_so_max") == 0) g_device_state.power_monitor.solar_max_voltage_tenths = value;
			else if (strcmp(key, "bright") == 0) g_device_state.brightness_level = value;
			else if (strcmp(key, "as_en") == 0) g_device_state.auto_save_enabled = (value != 0);
			else if (strcmp(key, "as_int") == 0) g_device_state.auto_save_interval_ms = value;
		}
	}

	fclose(f);

	printf("[I] device_state: State loaded from file (defaults when missing)\n\n");
}

void device_state_reset_to_defaults(void)
{
	printf("[I] device_state: Resetting device state to defaults\n");

	memcpy(&g_device_state, &default_state, sizeof(device_state_t));

	// Schedule persistence soon
	device_state_mark_dirty();
}

void power_monitor_state_set_current_view(power_monitor_view_type_t view)
{
	if (view >= POWER_MONITOR_VIEW_CURRENT && view < POWER_MONITOR_VIEW_COUNT) {
		g_device_state.power_monitor.current_view_type = view;
		g_device_state.power_monitor.is_initialized = true;

		printf("[I] device_state: Power monitor current view set to: %d\n", view);

		// Mark state as dirty for auto-save
		device_state_mark_dirty();
	}
}

power_monitor_view_type_t power_monitor_state_get_current_view(void)
{
	return g_device_state.power_monitor.current_view_type;
}

// Alert threshold getters/setters
int device_state_get_starter_alert_low_voltage_v(void) { return g_device_state.power_monitor.starter_alert_low_voltage_v; }
int device_state_get_starter_alert_high_voltage_v(void) { return g_device_state.power_monitor.starter_alert_high_voltage_v; }
int device_state_get_house_alert_low_voltage_v(void) { return g_device_state.power_monitor.house_alert_low_voltage_v; }
int device_state_get_house_alert_high_voltage_v(void) { return g_device_state.power_monitor.house_alert_high_voltage_v; }
int device_state_get_solar_alert_low_voltage_v(void) { return g_device_state.power_monitor.solar_alert_low_voltage_v; }
int device_state_get_solar_alert_high_voltage_v(void) { return g_device_state.power_monitor.solar_alert_high_voltage_v; }

void device_state_set_starter_alert_low_voltage_v(int volts) {
	g_device_state.power_monitor.starter_alert_low_voltage_v = volts;
	device_state_mark_dirty();
}
void device_state_set_starter_alert_high_voltage_v(int volts) {
	g_device_state.power_monitor.starter_alert_high_voltage_v = volts;
	device_state_mark_dirty();
}
void device_state_set_house_alert_low_voltage_v(int volts) {
	g_device_state.power_monitor.house_alert_low_voltage_v = volts;
	device_state_mark_dirty();
}
void device_state_set_house_alert_high_voltage_v(int volts) {
	g_device_state.power_monitor.house_alert_high_voltage_v = volts;
	device_state_mark_dirty();
}
void device_state_set_solar_alert_low_voltage_v(int volts) {
	g_device_state.power_monitor.solar_alert_low_voltage_v = volts;
	device_state_mark_dirty();
}
void device_state_set_solar_alert_high_voltage_v(int volts) {
	g_device_state.power_monitor.solar_alert_high_voltage_v = volts;
	device_state_mark_dirty();
}

// Bar graph gauge min/max/baseline setters (volts for convenience)
void device_state_set_starter_min_voltage_v(float volts) {
	device_state_set_starter_min_voltage_tenths((int)(volts * 10.0f));
}
void device_state_set_starter_max_voltage_v(float volts) {
	device_state_set_starter_max_voltage_tenths((int)(volts * 10.0f));
}
void device_state_set_starter_baseline_voltage_v(float volts) {
	device_state_set_starter_baseline_voltage_tenths((int)(volts * 10.0f));
}
void device_state_set_house_min_voltage_v(float volts) {
	device_state_set_house_min_voltage_tenths((int)(volts * 10.0f));
}
void device_state_set_house_max_voltage_v(float volts) {
	device_state_set_house_max_voltage_tenths((int)(volts * 10.0f));
}
void device_state_set_house_baseline_voltage_v(float volts) {
	device_state_set_house_baseline_voltage_tenths((int)(volts * 10.0f));
}
void device_state_set_solar_min_voltage_v(float volts) {
	device_state_set_solar_min_voltage_tenths((int)(volts * 10.0f));
}
void device_state_set_solar_max_voltage_v(float volts) {
	device_state_set_solar_max_voltage_tenths((int)(volts * 10.0f));
}

// Bar graph gauge min/max/baseline getters/setters (tenths of volts)
int device_state_get_starter_min_voltage_tenths(void) { return g_device_state.power_monitor.starter_min_voltage_tenths; }
int device_state_get_starter_max_voltage_tenths(void) { return g_device_state.power_monitor.starter_max_voltage_tenths; }
int device_state_get_starter_baseline_voltage_tenths(void) { return g_device_state.power_monitor.starter_baseline_voltage_tenths; }
int device_state_get_house_min_voltage_tenths(void) { return g_device_state.power_monitor.house_min_voltage_tenths; }
int device_state_get_house_max_voltage_tenths(void) { return g_device_state.power_monitor.house_max_voltage_tenths; }
int device_state_get_house_baseline_voltage_tenths(void) { return g_device_state.power_monitor.house_baseline_voltage_tenths; }
int device_state_get_solar_min_voltage_tenths(void) { return g_device_state.power_monitor.solar_min_voltage_tenths; }
int device_state_get_solar_max_voltage_tenths(void) { return g_device_state.power_monitor.solar_max_voltage_tenths; }

void device_state_set_starter_min_voltage_tenths(int tenths) {
	g_device_state.power_monitor.starter_min_voltage_tenths = tenths;

	// Validate and adjust baseline if it's now outside the new min range
	int baseline = g_device_state.power_monitor.starter_baseline_voltage_tenths;
	int max_val = g_device_state.power_monitor.starter_max_voltage_tenths;
	if (baseline < tenths || baseline > max_val) {
		// Set baseline to middle of new min/max range
		int new_baseline = (tenths + max_val) / 2;
		printf("[W] device_state: Starter baseline %.1fV outside new range, set to middle: %.1fV\n", baseline/10.0f, new_baseline/10.0f);
		g_device_state.power_monitor.starter_baseline_voltage_tenths = new_baseline;
	}

	device_state_mark_dirty();
}
void device_state_set_starter_max_voltage_tenths(int tenths) {
	g_device_state.power_monitor.starter_max_voltage_tenths = tenths;

	// Validate and adjust baseline if it's now outside the new max range
	int baseline = g_device_state.power_monitor.starter_baseline_voltage_tenths;
	int min_val = g_device_state.power_monitor.starter_min_voltage_tenths;
	if (baseline < min_val || baseline > tenths) {
		// Set baseline to middle of new min/max range
		int new_baseline = (min_val + tenths) / 2;
		printf("[W] device_state: Starter baseline %.1fV outside new range, set to middle: %.1fV\n", baseline/10.0f, new_baseline/10.0f);
		g_device_state.power_monitor.starter_baseline_voltage_tenths = new_baseline;
	}

	device_state_mark_dirty();
}
void device_state_set_starter_baseline_voltage_tenths(int tenths) {
	// Validate baseline is within min/max range
	int min_val = g_device_state.power_monitor.starter_min_voltage_tenths;
	int max_val = g_device_state.power_monitor.starter_max_voltage_tenths;

	if (tenths < min_val || tenths > max_val) {
		// Set baseline to middle of min/max range if outside bounds
		tenths = (min_val + max_val) / 2;
		printf("[W] device_state: Starter baseline outside range, set to middle: %.1fV\n", tenths/10.0f);
	}

	g_device_state.power_monitor.starter_baseline_voltage_tenths = tenths;
	device_state_mark_dirty();
}
void device_state_set_house_min_voltage_tenths(int tenths) {
	g_device_state.power_monitor.house_min_voltage_tenths = tenths;

	// Validate and adjust baseline if it's now outside the new min range
	int baseline = g_device_state.power_monitor.house_baseline_voltage_tenths;
	int max_val = g_device_state.power_monitor.house_max_voltage_tenths;
	if (baseline < tenths || baseline > max_val) {
		// Set baseline to middle of new min/max range
		int new_baseline = (tenths + max_val) / 2;
		printf("[W] device_state: House baseline %.1fV outside new range, set to middle: %.1fV\n", baseline/10.0f, new_baseline/10.0f);
		g_device_state.power_monitor.house_baseline_voltage_tenths = new_baseline;
	}

	device_state_mark_dirty();
}
void device_state_set_house_max_voltage_tenths(int tenths) {
	g_device_state.power_monitor.house_max_voltage_tenths = tenths;

	// Validate and adjust baseline if it's now outside the new max range
	int baseline = g_device_state.power_monitor.house_baseline_voltage_tenths;
	int min_val = g_device_state.power_monitor.house_min_voltage_tenths;
	if (baseline < min_val || baseline > tenths) {
		// Set baseline to middle of new min/max range
		int new_baseline = (min_val + tenths) / 2;
		printf("[W] device_state: House baseline %.1fV outside new range, set to middle: %.1fV\n", baseline/10.0f, new_baseline/10.0f);
		g_device_state.power_monitor.house_baseline_voltage_tenths = new_baseline;
	}

	device_state_mark_dirty();
}
void device_state_set_house_baseline_voltage_tenths(int tenths) {
	// Validate baseline is within min/max range
	int min_val = g_device_state.power_monitor.house_min_voltage_tenths;
	int max_val = g_device_state.power_monitor.house_max_voltage_tenths;

	if (tenths < min_val || tenths > max_val) {
		// Set baseline to middle of min/max range if outside bounds
		tenths = (min_val + max_val) / 2;
		printf("[W] device_state: House baseline outside range, set to middle: %.1fV\n", tenths/10.0f);
	}

	g_device_state.power_monitor.house_baseline_voltage_tenths = tenths;
	device_state_mark_dirty();
}
void device_state_set_solar_min_voltage_tenths(int tenths) {
	g_device_state.power_monitor.solar_min_voltage_tenths = tenths;
	device_state_mark_dirty();
}
void device_state_set_solar_max_voltage_tenths(int tenths) {
	g_device_state.power_monitor.solar_max_voltage_tenths = tenths;
	device_state_mark_dirty();
}

// Bar graph gauge min/max/baseline getters (actual float values)
float device_state_get_starter_min_voltage_v(void) {
	return g_device_state.power_monitor.starter_min_voltage_tenths / 10.0f;
}
float device_state_get_starter_max_voltage_v(void) {
	return g_device_state.power_monitor.starter_max_voltage_tenths / 10.0f;
}
float device_state_get_starter_baseline_voltage_v(void) {
	return g_device_state.power_monitor.starter_baseline_voltage_tenths / 10.0f;
}
float device_state_get_house_min_voltage_v(void) {
	return g_device_state.power_monitor.house_min_voltage_tenths / 10.0f;
}
float device_state_get_house_max_voltage_v(void) {
	return g_device_state.power_monitor.house_max_voltage_tenths / 10.0f;
}
float device_state_get_house_baseline_voltage_v(void) {
	return g_device_state.power_monitor.house_baseline_voltage_tenths / 10.0f;
}
float device_state_get_solar_min_voltage_v(void) {
	return g_device_state.power_monitor.solar_min_voltage_tenths / 10.0f;
}
float device_state_get_solar_max_voltage_v(void) {
	return g_device_state.power_monitor.solar_max_voltage_tenths / 10.0f;
}

bool device_state_is_initialized(void)
{
	return g_device_state.system_initialized;
}

void device_state_mark_dirty(void)
{
	if (g_device_state.auto_save_enabled) {
		// Debounce async save to avoid blocking UI
		s_state_save_pending = true;
		if (s_state_save_timer) {
			// Timer not needed for file-based storage
		}
	}
}

// Screen navigation state functions
void screen_navigation_request_detail_view(const char *module_name)
{
	if (!module_name) return;

	printf("[I] device_state: Requesting detail view for module: %s\n", module_name);
	printf("[I] device_state: Current state: screen=%d, module=%s\n",
		g_device_state.screen_navigation.current_screen,
		g_device_state.screen_navigation.current_module);

	g_device_state.screen_navigation.requested_screen = SCREEN_DETAIL_VIEW;
	strncpy(g_device_state.screen_navigation.requested_module, module_name,
		sizeof(g_device_state.screen_navigation.requested_module) - 1);
	g_device_state.screen_navigation.requested_module[sizeof(g_device_state.screen_navigation.requested_module) - 1] = '\0';
	g_device_state.screen_navigation.screen_transition_pending = true;

	printf("[I] device_state: Requested state: screen=%d, module=%s, pending=%d\n",
		g_device_state.screen_navigation.requested_screen,
		g_device_state.screen_navigation.requested_module,
		g_device_state.screen_navigation.screen_transition_pending);

	printf("[I] device_state: screen_navigation_request_detail_view completed successfully\n");

	device_state_mark_dirty();
}

void screen_navigation_request_home_screen(void)
{
	printf("[I] device_state: Requesting home screen\n");
	printf("[I] device_state: Current state: screen=%d, module=%s\n",
		g_device_state.screen_navigation.current_screen,
		g_device_state.screen_navigation.current_module);

	g_device_state.screen_navigation.requested_screen = SCREEN_HOME;
	g_device_state.screen_navigation.requested_module[0] = '\0'; // Clear requested module
	g_device_state.screen_navigation.screen_transition_pending = true;

	printf("[I] device_state: Requested state: screen=%d, module=%s, pending=%d\n",
		g_device_state.screen_navigation.requested_screen,
		g_device_state.screen_navigation.requested_module,
		g_device_state.screen_navigation.screen_transition_pending);

	printf("[I] device_state: screen_navigation_request_detail_view completed successfully\n");

	device_state_mark_dirty();
}

void screen_navigation_process_transitions(void)
{
	if (!g_device_state.screen_navigation.screen_transition_pending) {
		return;
	}

	printf("[I] device_state: Processing screen transition: %d -> %d, module: %s\n",
		g_device_state.screen_navigation.current_screen,
		g_device_state.screen_navigation.requested_screen,
		g_device_state.screen_navigation.requested_module);

	// Update current state to match requested state
	g_device_state.screen_navigation.current_screen = g_device_state.screen_navigation.requested_screen;
	strncpy(g_device_state.screen_navigation.current_module,
		g_device_state.screen_navigation.requested_module,
		sizeof(g_device_state.screen_navigation.current_module) - 1);
	g_device_state.screen_navigation.current_module[sizeof(g_device_state.screen_navigation.current_module) - 1] = '\0';

	// Clear requested state
	g_device_state.screen_navigation.requested_module[0] = '\0';
	g_device_state.screen_navigation.screen_transition_pending = false;

	printf("[I] device_state: Transition complete: current screen=%d, module=%s\n",
		g_device_state.screen_navigation.current_screen,
		g_device_state.screen_navigation.current_module);

	device_state_mark_dirty();
}

void screen_navigation_set_current_screen(screen_type_t screen_type, const char *module_name)
{
	printf("[I] device_state: Setting current screen to %d (module: %s)\n", screen_type, module_name ? module_name : "none");

	g_device_state.screen_navigation.current_screen = screen_type;
	if (module_name) {
		strncpy(g_device_state.screen_navigation.current_module, module_name,
			sizeof(g_device_state.screen_navigation.current_module) - 1);
		g_device_state.screen_navigation.current_module[sizeof(g_device_state.screen_navigation.current_module) - 1] = '\0';
	} else {
		memset(g_device_state.screen_navigation.current_module, 0, sizeof(g_device_state.screen_navigation.current_module));
	}

	printf("[I] device_state: Current screen state updated: screen=%d, module=%s\n",
		g_device_state.screen_navigation.current_screen,
		g_device_state.screen_navigation.current_module);

	device_state_mark_dirty();
}

screen_type_t screen_navigation_get_current_screen(void)
{
	return g_device_state.screen_navigation.current_screen;
}

screen_type_t screen_navigation_get_requested_screen(void)
{
	return g_device_state.screen_navigation.requested_screen;
}

const char* screen_navigation_get_current_module(void)
{
	return g_device_state.screen_navigation.current_module;
}

const char* screen_navigation_get_requested_module(void)
{
	return g_device_state.screen_navigation.requested_module;
}

bool screen_navigation_is_transition_pending(void)
{
	return g_device_state.screen_navigation.screen_transition_pending;
}

// View lifecycle management functions (separated from state management)
void current_view_initialize(int available_views_count)
{
	printf("[I] device_state: Initializing view lifecycle with %d available views\n", available_views_count);

	if (available_views_count <= 0) {
		printf("[E] device_state: Invalid available views count: %d\n", available_views_count);
		return;
	}

	g_device_state.screen_navigation.available_views_count = available_views_count;
	g_device_state.screen_navigation.current_view_index = 0;
	g_device_state.screen_navigation.view_is_visible = false;

	printf("[I] device_state: View lifecycle initialized: index=%d, count=%d, visible=%s\n",
		g_device_state.screen_navigation.current_view_index,
		g_device_state.screen_navigation.available_views_count,
		g_device_state.screen_navigation.view_is_visible ? "true" : "false");

	device_state_mark_dirty();
}

void current_view_cleanup(void)
{
	printf("[I] device_state: Cleaning up view lifecycle\n");

	g_device_state.screen_navigation.view_is_visible = false;
	g_device_state.screen_navigation.available_views_count = 0;
	g_device_state.screen_navigation.current_view_index = 0;

	printf("[I] device_state: View lifecycle cleaned up\n");
	device_state_mark_dirty();
}

int current_view_get_index(void)
{
	return g_device_state.screen_navigation.current_view_index;
}

int current_view_get_count(void)
{
	return g_device_state.screen_navigation.available_views_count;
}

bool current_view_is_visible(void)
{
	return g_device_state.screen_navigation.view_is_visible;
}

void current_view_set_visible(bool visible)
{
	printf("[I] device_state: Setting view visibility: %s\n", visible ? "true" : "false");
	g_device_state.screen_navigation.view_is_visible = visible;
	device_state_mark_dirty();
}

// State transition management functions (centralized in global state)
void view_state_request_transition(int target_view_index)
{
	if (target_view_index < 0 || target_view_index >= g_device_state.screen_navigation.available_views_count) {
		printf("[E] device_state: Invalid target view index: %d (available: %d)\n",
			target_view_index, g_device_state.screen_navigation.available_views_count);
		return;
	}

	printf("[I] device_state: Requesting view transition: %d -> %d\n",
		g_device_state.screen_navigation.current_view_index, target_view_index);

	g_device_state.screen_navigation.requested_view_index = target_view_index;
	g_device_state.screen_navigation.view_transition_pending = true;

	device_state_mark_dirty();
}

void view_state_process_transitions(void)
{
	if (!g_device_state.screen_navigation.view_transition_pending) {
		return;
	}

	printf("[I] device_state: Processing view transition: %d -> %d\n",
		g_device_state.screen_navigation.current_view_index,
		g_device_state.screen_navigation.requested_view_index);

	// Update current view index to match requested
	g_device_state.screen_navigation.current_view_index = g_device_state.screen_navigation.requested_view_index;

	// Clear transition state
	g_device_state.screen_navigation.view_transition_pending = false;

	printf("[I] device_state: View transition complete: current index=%d\n",
		g_device_state.screen_navigation.current_view_index);

	device_state_mark_dirty();
}

bool view_state_is_transition_pending(void)
{
	return g_device_state.screen_navigation.view_transition_pending;
}

void view_state_cycle_to_next(void)
{
	if (g_device_state.screen_navigation.view_cycling_in_progress) {
		printf("[W] device_state: View cycling already in progress, ignoring request\n");
		return;
	}

	if (g_device_state.screen_navigation.available_views_count <= 0) {
		printf("[E] device_state: No available views to cycle through\n");
		return;
	}

	int next_view = (g_device_state.screen_navigation.current_view_index + 1) %
		g_device_state.screen_navigation.available_views_count;

	printf("[I] device_state: State requesting view cycle: %d -> %d\n",
		g_device_state.screen_navigation.current_view_index, next_view);

	g_device_state.screen_navigation.view_cycling_in_progress = true;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	g_device_state.screen_navigation.cycling_start_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

	// Request transition to next view
	view_state_request_transition(next_view);

	device_state_mark_dirty();
}

void view_state_set_cycling_in_progress(bool in_progress)
{
	printf("[I] device_state: Setting view cycling in progress: %s\n", in_progress ? "true" : "false");
	g_device_state.screen_navigation.view_cycling_in_progress = in_progress;
	device_state_mark_dirty();
}

bool view_state_is_cycling_in_progress(void)
{
	return g_device_state.screen_navigation.view_cycling_in_progress;
}

void view_state_check_timeout(void)
{
	if (!g_device_state.screen_navigation.view_cycling_in_progress) {
		return; // Not cycling, nothing to check
	}

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint32_t current_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	uint32_t elapsed_time = current_time - g_device_state.screen_navigation.cycling_start_time;

	// Timeout after 5 seconds (5000ms)
	if (elapsed_time > 5000) {
		printf("[W] device_state: View cycling timeout after %lu ms, clearing flag\n", elapsed_time);
		g_device_state.screen_navigation.view_cycling_in_progress = false;
		device_state_mark_dirty();
	}
}

// Module-specific screen and view management functions
void module_screen_view_initialize(const char *module_name, screen_type_t initial_screen, int available_views_count)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return;

	printf("[I] device_state: Initializing module %s screen view state: screen=%d, views=%d\n",
		module_name, initial_screen, available_views_count);

	if (available_views_count <= 0) {
		printf("[E] device_state: Invalid available views count: %d\n", available_views_count);
		return;
	}

	state->current_screen = initial_screen;
	state->last_screen = initial_screen;
	state->current_view_index = 0;
	state->available_views_count = available_views_count;
	state->view_is_visible = false;
	state->requested_view_index = 0;
	state->view_transition_pending = false;
	state->view_cycling_in_progress = false;

	printf("[I] device_state: Module %s screen view state initialized\n", module_name);
	device_state_mark_dirty();
}

void module_screen_view_cleanup(const char *module_name)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return;

	printf("[I] device_state: Cleaning up module %s screen view state\n", module_name);

	state->view_is_visible = false;
	state->available_views_count = 0;
	state->current_view_index = 0;
	state->view_transition_pending = false;
	state->view_cycling_in_progress = false;

	printf("[I] device_state: Module %s screen view state cleaned up\n", module_name);
	device_state_mark_dirty();
}

void module_screen_view_set_current_screen(const char *module_name, screen_type_t screen)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return;

	printf("[I] device_state: Module %s setting current screen: %d -> %d\n",
		module_name, state->current_screen, screen);

	state->last_screen = state->current_screen;
	state->current_screen = screen;

	device_state_mark_dirty();
}

screen_type_t module_screen_view_get_current_screen(const char *module_name)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return SCREEN_HOME;
	return state->current_screen;
}

screen_type_t module_screen_view_get_last_screen(const char *module_name)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return SCREEN_HOME;
	return state->last_screen;
}

void module_screen_view_set_view_index(const char *module_name, int view_index)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return;

	if (view_index < 0 || view_index >= state->available_views_count) {
		printf("[E] device_state: Invalid view index for module %s: %d (available: %d)\n",
			module_name, view_index, state->available_views_count);
		return;
	}

	printf("[I] device_state: Module %s setting view index: %d -> %d\n",
		module_name, state->current_view_index, view_index);

	state->current_view_index = view_index;
	device_state_mark_dirty();
}

int module_screen_view_get_view_index(const char *module_name)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return 0;
	return state->current_view_index;
}

int module_screen_view_get_views_count(const char *module_name)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return 0;
	return state->available_views_count;
}

bool module_screen_view_is_visible(const char *module_name)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return false;
	return state->view_is_visible;
}

void module_screen_view_set_visible(const char *module_name, bool visible)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return;

	printf("[I] device_state: Module %s setting view visibility: %s\n",
		module_name, visible ? "true" : "false");

	state->view_is_visible = visible;
	device_state_mark_dirty();
}

// Module-specific view transitions
void module_screen_view_request_transition(const char *module_name, int target_view_index)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return;

	if (target_view_index < 0 || target_view_index >= state->available_views_count) {
		printf("[E] device_state: Invalid target view index for module %s: %d (available: %d)\n",
			module_name, target_view_index, state->available_views_count);
		return;
	}

	printf("[I] device_state: Module %s requesting view transition: %d -> %d\n",
		module_name, state->current_view_index, target_view_index);

	state->requested_view_index = target_view_index;
	state->view_transition_pending = true;

	device_state_mark_dirty();
}

void module_screen_view_process_transitions(const char *module_name)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return;

	if (!state->view_transition_pending) {
		return;
	}

	printf("[I] device_state: Module %s processing view transition: %d -> %d\n",
		module_name, state->current_view_index, state->requested_view_index);

	// Update current view index to match requested
	state->current_view_index = state->requested_view_index;

	// Clear transition state
	state->view_transition_pending = false;

	printf("[I] device_state: Module %s view transition complete: current index=%d\n",
		module_name, state->current_view_index);

	device_state_mark_dirty();
}

bool module_screen_view_is_transition_pending(const char *module_name)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return false;
	return state->view_transition_pending;
}

void module_screen_view_cycle_to_next(const char *module_name)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return;

	if (state->view_cycling_in_progress) {
		printf("[W] device_state: Module %s view cycling already in progress, ignoring request\n", module_name);
		return;
	}

	if (state->available_views_count <= 0) {
		printf("[E] device_state: Module %s has no available views to cycle through\n", module_name);
		return;
	}

	int next_view = (state->current_view_index + 1) % state->available_views_count;

	printf("[I] device_state: Module %s requesting view cycle: %d -> %d\n",
		module_name, state->current_view_index, next_view);

	state->view_cycling_in_progress = true;
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	state->cycling_start_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

	// Request transition to next view
	module_screen_view_request_transition(module_name, next_view);

	device_state_mark_dirty();
}

void module_screen_view_set_cycling_in_progress(const char *module_name, bool in_progress)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return;

	printf("[I] device_state: Module %s setting view cycling in progress: %s\n",
		module_name, in_progress ? "true" : "false");

	state->view_cycling_in_progress = in_progress;
	device_state_mark_dirty();
}

bool module_screen_view_is_cycling_in_progress(const char *module_name)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return false;
	return state->view_cycling_in_progress;
}

void module_screen_view_check_timeout(const char *module_name)
{
	module_screen_view_state_t *state = get_module_screen_view_state(module_name);
	if (!state) return;

	if (!state->view_cycling_in_progress) {
		return; // Not cycling, nothing to check
	}

	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint32_t current_time = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	uint32_t elapsed_time = current_time - state->cycling_start_time;

	// Timeout after 5 seconds (5000ms)
	if (elapsed_time > 5000) {
		printf("[W] device_state: Module %s view cycling timeout after %lu ms, clearing flag\n",
			module_name, elapsed_time);
		state->view_cycling_in_progress = false;
		device_state_mark_dirty();
	}
}

