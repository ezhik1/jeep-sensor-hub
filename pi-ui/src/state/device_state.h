#ifndef DEVICE_STATE_H
#define DEVICE_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include "../displayModules/power-monitor/power-monitor.h"

#ifdef __cplusplus
extern "C" {
#endif

// Screen navigation state
typedef enum {
	SCREEN_NONE = -1,
	SCREEN_HOME = 0,
	SCREEN_DETAIL_VIEW,
	SCREEN_COUNT
} screen_type_t;

typedef struct {
	screen_type_t current_screen;
	screen_type_t requested_screen;
	char current_module[32];
	char requested_module[32];
	bool screen_transition_pending;

	// View lifecycle state (separated from state management)
	int current_view_index;        // Index into available views array
	int available_views_count;     // Total number of available views
	bool view_is_visible;          // Whether view is currently visible

	// View state transition management (centralized)
	int requested_view_index;      // Target view index for transitions
	bool view_transition_pending;  // Whether a view transition is pending
	bool view_cycling_in_progress; // Prevents recursive view cycling
	uint32_t cycling_start_time;   // Timestamp when cycling started (for timeout)
} screen_navigation_state_t;

// Module-specific screen and view tracking
typedef struct {
	screen_type_t current_screen;      // Current screen for this module
	screen_type_t last_screen;         // Last screen for this module
	int current_view_index;            // Current view index for current screen
	int available_views_count;         // Total views available for current screen
	bool view_is_visible;              // Whether view is currently visible

	// View transition state
	int requested_view_index;          // Target view index for transitions
	bool view_transition_pending;      // Whether a view transition is pending
	bool view_cycling_in_progress;     // Prevents recursive view cycling
	uint32_t cycling_start_time;       // Timestamp when cycling started (for timeout)
} module_screen_view_state_t;

// Module-specific state structures
typedef struct {
	// Module-specific screen and view management
	module_screen_view_state_t screen_view_state;

	// Module-specific view type (for power monitor)
	power_monitor_view_type_t current_view_type; // Current view type selection
	bool is_initialized;

	// Alert thresholds (whole volts) - when to flash red
	int starter_alert_low_voltage_v;
	int starter_alert_high_voltage_v;
	int house_alert_low_voltage_v;
	int house_alert_high_voltage_v;
	int solar_alert_low_voltage_v;
	int solar_alert_high_voltage_v;

	// Bar graph gauge display ranges (tenths of volts for precision) - what range to show on gauge
	int starter_min_voltage_tenths;
	int starter_max_voltage_tenths;
	int starter_baseline_voltage_tenths;
	int house_min_voltage_tenths;
	int house_max_voltage_tenths;
	int house_baseline_voltage_tenths;
	int solar_min_voltage_tenths;
	int solar_max_voltage_tenths;
} power_monitor_state_t;

typedef struct {
	// Module-specific screen and view management
	module_screen_view_state_t screen_view_state;

	// Module-specific data
	bool is_initialized;
	// Add other module-specific data here as needed
} other_modules_state_t;

// Global device state structure
typedef struct {
	// System state
	bool system_initialized;
	uint32_t last_save_timestamp;

	// Screen navigation state
	screen_navigation_state_t screen_navigation;

	// Module states
	power_monitor_state_t power_monitor;
	other_modules_state_t other_modules;

	// Global settings
	uint8_t brightness_level;
	bool auto_save_enabled;
	uint32_t auto_save_interval_ms;
} device_state_t;

// Global state instance
extern device_state_t g_device_state;

// State management functions
void device_state_init(void);
void device_state_save(void);
void device_state_load(void);
void device_state_reset_to_defaults(void);

// Module-specific state functions
void power_monitor_state_set_current_view(power_monitor_view_type_t view);
power_monitor_view_type_t power_monitor_state_get_current_view(void);

// Power monitor alert thresholds getters/setters (whole volts)
int device_state_get_starter_alert_low_voltage_v(void);
int device_state_get_starter_alert_high_voltage_v(void);
int device_state_get_house_alert_low_voltage_v(void);
int device_state_get_house_alert_high_voltage_v(void);
int device_state_get_solar_alert_low_voltage_v(void);
int device_state_get_solar_alert_high_voltage_v(void);

void device_state_set_starter_alert_low_voltage_v(int volts);
void device_state_set_starter_alert_high_voltage_v(int volts);
void device_state_set_house_alert_low_voltage_v(int volts);
void device_state_set_house_alert_high_voltage_v(int volts);
void device_state_set_solar_alert_low_voltage_v(int volts);
void device_state_set_solar_alert_high_voltage_v(int volts);

// Bar graph gauge min/max/baseline setters (volts for convenience)
void device_state_set_starter_min_voltage_v(float volts);
void device_state_set_starter_max_voltage_v(float volts);
void device_state_set_starter_baseline_voltage_v(float volts);
void device_state_set_house_min_voltage_v(float volts);
void device_state_set_house_max_voltage_v(float volts);
void device_state_set_house_baseline_voltage_v(float volts);
void device_state_set_solar_min_voltage_v(float volts);
void device_state_set_solar_max_voltage_v(float volts);

// Bar graph gauge min/max/baseline getters/setters (tenths of volts for precision)
int device_state_get_starter_min_voltage_tenths(void);
int device_state_get_starter_max_voltage_tenths(void);
int device_state_get_starter_baseline_voltage_tenths(void);
int device_state_get_house_min_voltage_tenths(void);
int device_state_get_house_max_voltage_tenths(void);
int device_state_get_house_baseline_voltage_tenths(void);
int device_state_get_solar_min_voltage_tenths(void);
int device_state_get_solar_max_voltage_tenths(void);

void device_state_set_starter_min_voltage_tenths(int tenths);
void device_state_set_starter_max_voltage_tenths(int tenths);
void device_state_set_starter_baseline_voltage_tenths(int tenths);
void device_state_set_house_min_voltage_tenths(int tenths);
void device_state_set_house_max_voltage_tenths(int tenths);
void device_state_set_house_baseline_voltage_tenths(int tenths);
void device_state_set_solar_min_voltage_tenths(int tenths);
void device_state_set_solar_max_voltage_tenths(int tenths);

// Bar graph gauge min/max/baseline getters (actual float values)
float device_state_get_starter_min_voltage_v(void);
float device_state_get_starter_max_voltage_v(void);
float device_state_get_starter_baseline_voltage_v(void);
float device_state_get_house_min_voltage_v(void);
float device_state_get_house_max_voltage_v(void);
float device_state_get_house_baseline_voltage_v(void);
float device_state_get_solar_min_voltage_v(void);
float device_state_get_solar_max_voltage_v(void);

// Screen navigation state functions
void screen_navigation_request_detail_view(const char *module_name);
void screen_navigation_request_home_screen(void);
void screen_navigation_process_transitions(void);
void screen_navigation_set_current_screen(screen_type_t screen_type, const char *module_name);
screen_type_t screen_navigation_get_current_screen(void);
screen_type_t screen_navigation_get_requested_screen(void);
const char* screen_navigation_get_current_module(void);
const char* screen_navigation_get_requested_module(void);
bool screen_navigation_is_transition_pending(void);

// Module-specific screen and view management
void module_screen_view_initialize(const char *module_name, screen_type_t initial_screen, int available_views_count);
void module_screen_view_cleanup(const char *module_name);
void module_screen_view_set_current_screen(const char *module_name, screen_type_t screen);
screen_type_t module_screen_view_get_current_screen(const char *module_name);
screen_type_t module_screen_view_get_last_screen(const char *module_name);
void module_screen_view_set_view_index(const char *module_name, int view_index);
int module_screen_view_get_view_index(const char *module_name);
int module_screen_view_get_views_count(const char *module_name);
bool module_screen_view_is_visible(const char *module_name);
void module_screen_view_set_visible(const char *module_name, bool visible);

// Module-specific view transitions
void module_screen_view_request_transition(const char *module_name, int target_view_index);
void module_screen_view_process_transitions(const char *module_name);
bool module_screen_view_is_transition_pending(const char *module_name);
void module_screen_view_cycle_to_next(const char *module_name);
void module_screen_view_set_cycling_in_progress(const char *module_name, bool in_progress);
bool module_screen_view_is_cycling_in_progress(const char *module_name);
void module_screen_view_check_timeout(const char *module_name);

// Legacy global view functions (deprecated - use module-specific functions)
void current_view_initialize(int available_views_count);
void current_view_cleanup(void);
int current_view_get_index(void);
int current_view_get_count(void);
bool current_view_is_visible(void);
void current_view_set_visible(bool visible);

// Legacy global state transition functions (deprecated - use module-specific functions)
void view_state_request_transition(int target_view_index);
void view_state_process_transitions(void);
bool view_state_is_transition_pending(void);
void view_state_cycle_to_next(void);
void view_state_set_cycling_in_progress(bool in_progress);
bool view_state_is_cycling_in_progress(void);
void view_state_check_timeout(void);

// Utility functions
bool device_state_is_initialized(void);
void device_state_mark_dirty(void);

#ifdef __cplusplus
}
#endif

#endif // DEVICE_STATE_H
