#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "power-monitor.h"
#include "gauge_types.h"

// State
#include "../../state/device_state.h"

// Data
#include "../../data/mock_data/mock_data.h"
#include "../../data/lerp_data/lerp_data.h"
#include "../../data/config.h"

// Views
#include "views/voltage_grid_view/voltage_grid_view.h"
#include "views/power_grid_view/power_grid_view.h"
#include "views/amperage_grid_view/amperage_grid_view.h"
#include "views/starter_voltage_view/starter_voltage_view.h"
#include "views/house_voltage_view/house_voltage_view.h"
#include "views/solar_voltage_view/solar_voltage_view.h"
#include "views/starter_current_view/starter_current_view.h"
#include "views/house_current_view/house_current_view.h"
#include "views/solar_current_view/solar_current_view.h"
#include "views/starter_power_view/starter_power_view.h"
#include "views/house_power_view/house_power_view.h"
#include "views/solar_power_view/solar_power_view.h"

// Number of available views - update this when adding/removing views
#define POWER_MONITOR_VIEW_COUNT 12
#include "../shared/views/single_value_bar_graph_view/single_value_bar_graph_view.h"


// Shared Modules
#include "../shared/module_interface.h"
#include "../shared/display_module_base.h"
#include "../shared/current_view/current_view_manager.h"
#include "../shared/modals/alerts_modal/alerts_modal.h"
#include "../shared/modals/timeline_modal/timeline_modal.h"
#include "../shared/gauges/bar_graph_gauge/bar_graph_gauge.h"
#include "../shared/utils/number_formatting/number_formatting.h"
#include "../shared/utils/warning_icon/warning_icon.h"

// App data store
#include "../../app_data_store.h"

// External references to current view gauges (declared in voltage_grid_view.h)
extern bar_graph_gauge_t s_starter_voltage_gauge;
extern bar_graph_gauge_t s_house_voltage_gauge;
extern bar_graph_gauge_t s_solar_voltage_gauge;

// External references to power grid view gauges (declared in power_grid_view.h)
extern bar_graph_gauge_t s_starter_power_gauge;
extern bar_graph_gauge_t s_house_power_gauge;
extern bar_graph_gauge_t s_solar_power_gauge;

// External references to amperage grid view gauges (declared in amperage_grid_view.h)
extern bar_graph_gauge_t s_starter_current_gauge;
extern bar_graph_gauge_t s_house_current_gauge;
extern bar_graph_gauge_t s_solar_current_gauge;

// External reference to single view gauges
extern single_value_bar_graph_view_state_t* single_view_starter_voltage;
extern single_value_bar_graph_view_state_t* single_view_house_voltage;
extern single_value_bar_graph_view_state_t* single_view_solar_voltage;
extern single_value_bar_graph_view_state_t* single_view_starter_current;
extern single_value_bar_graph_view_state_t* single_view_house_current;
extern single_value_bar_graph_view_state_t* single_view_solar_current;
extern single_value_bar_graph_view_state_t* single_view_starter_power;
extern single_value_bar_graph_view_state_t* single_view_house_power;
extern single_value_bar_graph_view_state_t* single_view_solar_power;

#include "../../screens/screen_manager.h"
#include "../../screens/detail_screen/detail_screen.h"
#include "../../screens/home_screen/home_screen.h"

// Module Configurations
#include "config/battery_alerts_config.h"
#include "config/timeline_modal_config.h"

// UI Styling
#include "../../displayModules/shared/palette.h"
#include "../../fonts/lv_font_noplato_24.h"


// Power monitor data defaults
#define POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS 30
#define POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS 30

#define POWER_MONITOR_DEFAULT_STARTER_ALERT_LOW_VOLTAGE_V 11.0f
#define POWER_MONITOR_DEFAULT_STARTER_ALERT_HIGH_VOLTAGE_V 14.0f
#define POWER_MONITOR_DEFAULT_STARTER_BASELINE_VOLTAGE_V 12.6f
#define POWER_MONITOR_DEFAULT_STARTER_MIN_VOLTAGE_V	11.0f
#define POWER_MONITOR_DEFAULT_STARTER_MAX_VOLTAGE_V 14.4f

#define POWER_MONITOR_DEFAULT_HOUSE_ALERT_LOW_VOLTAGE_V	11.0f
#define POWER_MONITOR_DEFAULT_HOUSE_ALERT_HIGH_VOLTAGE_V 14.0f
#define POWER_MONITOR_DEFAULT_HOUSE_BASELINE_VOLTAGE_V	12.6f
#define POWER_MONITOR_DEFAULT_HOUSE_MIN_VOLTAGE_V 11.0f
#define POWER_MONITOR_DEFAULT_HOUSE_MAX_VOLTAGE_V 14.4f

#define POWER_MONITOR_DEFAULT_SOLAR_ALERT_LOW_VOLTAGE_V	12.0f
#define POWER_MONITOR_DEFAULT_SOLAR_ALERT_HIGH_VOLTAGE_V 22.0f
#define POWER_MONITOR_DEFAULT_SOLAR_MIN_VOLTAGE_V 0.0f
#define POWER_MONITOR_DEFAULT_SOLAR_MAX_VOLTAGE_V 20.0f

// Forward declarations
static void power_monitor_cycle_view(void);
// Modal functions moved to detail_screen.c

void power_monitor_create_current_view_content(lv_obj_t* container);
static void power_monitor_home_current_view_touch_cb(lv_event_t * e);
static void power_monitor_detail_current_view_touch_cb(lv_event_t * e);
static void power_monitor_create_detail_gauges(lv_obj_t* container);
static void power_monitor_update_detail_gauges(void);
static power_monitor_data_t* power_monitor_get_data_internal(void);
void power_monitor_render_current_view(lv_obj_t* container);
void power_monitor_cleanup_internal(void);
static void power_monitor_destroy_current_view(int old_view_index);
static int power_monitor_get_view_index(void);
static void power_monitor_set_view_index(int index);

static const char *TAG = "power_monitor";

// UI state flags moved to s_ui_state struct


// Module base instance
static display_module_base_t s_module_base;

// Accessor for module base (for home screen)
display_module_base_t* power_monitor_get_module_base(void)
{
	return &s_module_base;
}

// Module-specific UI state (not data - data lives in app_data_store)
typedef struct {
	// UI-only state
	bool detail_view_needs_refresh;
	bool navigation_teardown_in_progress;
	bool view_destroy_in_progress;
	bool rendering_in_progress;
	bool reset_in_progress;
} power_monitor_ui_state_t;

static power_monitor_ui_state_t s_ui_state = {0};

// Removed unused home_overlay
static detail_screen_t* detail_screen = NULL;


// Global current view containers for this module

// Guard to prevent recursive calls

// Available view types in order (first is default)
static const power_monitor_view_type_t available_views[] = {
	POWER_MONITOR_VIEW_BAR_GRAPH,      // Voltage grid view
	POWER_MONITOR_VIEW_AMPERAGE_GRID,  // Current grid view
	POWER_MONITOR_VIEW_POWER,          // Power grid view
	POWER_MONITOR_VIEW_NUMERICAL,      // Starter voltage single view
	POWER_MONITOR_VIEW_HOUSE_VOLTAGE,  // House voltage single view
	POWER_MONITOR_VIEW_SOLAR_VOLTAGE,  // Solar voltage single view
	POWER_MONITOR_VIEW_STARTER_CURRENT,// Starter current single view
	POWER_MONITOR_VIEW_HOUSE_CURRENT,  // House current single view
	POWER_MONITOR_VIEW_SOLAR_CURRENT,  // Solar current single view
	POWER_MONITOR_VIEW_STARTER_POWER,  // Starter power single view
	POWER_MONITOR_VIEW_HOUSE_POWER,    // House power single view
	POWER_MONITOR_VIEW_SOLAR_POWER     // Solar power single view
};

// Modal state management handled by detail_screen.c

// Helper function to get current view type from global state
static power_monitor_view_type_t get_current_view_type(void)
{
	int view_index = power_monitor_get_view_index();

	if (view_index < 0 || view_index >= POWER_MONITOR_VIEW_COUNT) {
		printf("[E] power_monitor: Invalid view index: %d (total: %d)\n", view_index, POWER_MONITOR_VIEW_COUNT);
		return POWER_MONITOR_VIEW_BAR_GRAPH; // Default fallback
	}

	return available_views[view_index];
}

// Navigation callback implementations
static void power_monitor_navigation_cycle_to_next_view(void);
static void power_monitor_navigation_hide_detail_screen(void);
static void power_monitor_navigation_request_home_screen(void);

static void power_monitor_navigation_cycle_to_next_view(void)
{

	// Get current index from device state
	int current_index = power_monitor_get_view_index();
	printf("[I] power_monitor: Before cycle: index=%d, total_views=%d\n", current_index, POWER_MONITOR_VIEW_COUNT);

	// Simple wrap-around logic
	int next_index = (current_index + 1) % POWER_MONITOR_VIEW_COUNT;

	// Update device state directly
	power_monitor_set_view_index(next_index);
	printf("[I] power_monitor: View cycle complete - updated from index %d to %d\n", current_index, next_index);
}

// Callback to request home screen after LVGL cleanup
static bool s_detail_destroy_pending = false;
static lv_timer_t* s_detail_destroy_timer = NULL;

static void power_monitor_destroy_detail_screen_timer_cb(lv_timer_t* timer)
{
	s_detail_destroy_timer = NULL;

	// Kill static gauges first (ensures timers are stopped before LVGL tree goes away)
	extern void power_monitor_reset_static_gauges(void);
	power_monitor_reset_static_gauges();

	if (detail_screen) {
		detail_screen_destroy(detail_screen);
		detail_screen = NULL;
	}
	s_detail_destroy_pending = false;
	power_monitor_navigation_request_home_screen();
	if (timer) lv_timer_del(timer);

	// Clear navigation teardown flag once we finished
	s_ui_state.navigation_teardown_in_progress = false;
}

static void power_monitor_navigation_hide_detail_screen(void)
{
	if (s_ui_state.navigation_teardown_in_progress) {
		printf("[W] power_monitor: Navigation teardown in progress, ignoring hide request\n");
		return;
	}
	s_ui_state.navigation_teardown_in_progress = true;
	if (s_detail_destroy_pending) {
		printf("[W] power_monitor: Destroy already pending, ignoring duplicate request\n");
		return; // keep teardown flag set until callback clears
	}
	s_detail_destroy_pending = true;
	if (s_detail_destroy_timer) {
		lv_timer_del(s_detail_destroy_timer);
		s_detail_destroy_timer = NULL;
	}
	// Defer destroy slightly to let LVGL finish pending ops
	s_detail_destroy_timer = lv_timer_create(power_monitor_destroy_detail_screen_timer_cb, 50, NULL);
}

static void power_monitor_navigation_request_home_screen(void)
{
	printf("[I] power_monitor: About to request home screen transition\n");
	extern void screen_navigation_request_home_screen(void);
	printf("[I] power_monitor: Calling screen_navigation_request_home_screen\n");
	screen_navigation_request_home_screen();
	printf("[I] power_monitor: Home screen transition requested\n");
	printf("[I] power_monitor: power_monitor_navigation_request_home_screen completed\n");
}


// Simple container management
static lv_obj_t* power_monitor_container = NULL;

// Sensor data labels moved to detail_screen.c - power monitor only provides data

// Initialize power-monitor's own container (simplified)
static void power_monitor_init_widget(void)
{
	if (power_monitor_container) {
		printf("[I] power_monitor: Container already initialized\n");
		return;
	}

	// Create power-monitor's own container on the main screen
	power_monitor_container = lv_obj_create(lv_scr_act());
	lv_obj_set_size(power_monitor_container, 320, 240);
	lv_obj_align(power_monitor_container, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_bg_opa(power_monitor_container, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(power_monitor_container, 0, 0);
	lv_obj_clear_flag(power_monitor_container, LV_OBJ_FLAG_SCROLLABLE);

	printf("[I] power_monitor: Power monitor container created successfully\n");
}


// Create current view using the shared template system
void power_monitor_create_current_view_content(lv_obj_t* container)
{

	if (!container) {

		printf("[E] power_monitor: Container is NULL\n");
		return;
	}

	// Safety check: validate container
	if (!lv_obj_is_valid(container)) {

		printf("[E] power_monitor: Container is not valid\n");
		return;
	}

	// Clear container first
	lv_obj_clean(container);

	// Re-apply container styling after clean (clean removes styling)
	// Use detail screen's reusable function for consistent styling
	extern detail_screen_t* detail_screen;
	if (detail_screen && container == detail_screen->current_view_container) {
		detail_screen_restore_current_view_styling(container);
	}

	// Get current view type and render appropriate view directly in container
	power_monitor_view_type_t current_type = get_current_view_type();

	if (current_type == POWER_MONITOR_VIEW_BAR_GRAPH) {

		power_monitor_voltage_grid_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_AMPERAGE_GRID) {

		power_monitor_amperage_grid_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_POWER) {

		power_monitor_power_grid_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_NUMERICAL) {

		power_monitor_starter_voltage_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_HOUSE_VOLTAGE) {

		power_monitor_house_voltage_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_SOLAR_VOLTAGE) {

		power_monitor_solar_voltage_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_STARTER_CURRENT) {

		power_monitor_starter_current_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_HOUSE_CURRENT) {

		power_monitor_house_current_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_SOLAR_CURRENT) {

		power_monitor_solar_current_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_STARTER_POWER) {

		power_monitor_starter_power_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_HOUSE_POWER) {

		power_monitor_house_power_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_SOLAR_POWER) {

		power_monitor_solar_power_view_render(container);
	}


	// Mark view cycling as complete if this was called during cycling
	if( current_view_manager_is_cycling_in_progress() ){

		current_view_manager_set_cycling_in_progress(false);
	}
}

// Simple view cycling - just calls the main cycling function
static void power_monitor_cycle_view(void)
{
	printf("[I] power_monitor: === CYCLING CURRENT VIEW ===\n");
	power_monitor_cycle_current_view();
}


// Removed unused power_monitor_get_view_info function

// ===========================
// Centralized gauge histories (IN-MEMORY ONLY)
// ===========================
#define POWER_MONITOR_MAX_GAUGE_POINTS 2000
#include <time.h>
#include <stdio.h>
typedef struct {
	float values[POWER_MONITOR_MAX_GAUGE_POINTS];
	int count;
} power_monitor_gauge_history_t;

static power_monitor_gauge_history_t s_histories[POWER_MONITOR_DATA_COUNT];

static inline power_monitor_gauge_history_t* power_monitor_get_history(power_monitor_data_type_t t){
	if((int)t < 0 || t >= POWER_MONITOR_DATA_COUNT) return NULL;
	return &s_histories[t];
}

// LERP data getter functions
static float get_starter_voltage(const lerp_power_monitor_data_t* data) {
	return lerp_value_get_display(&data->starter_voltage);
}

static float get_starter_current(const lerp_power_monitor_data_t* data) {
	return lerp_value_get_display(&data->starter_current);
}

static float get_house_voltage(const lerp_power_monitor_data_t* data) {
	return lerp_value_get_display(&data->house_voltage);
}

static float get_house_current(const lerp_power_monitor_data_t* data) {
	return lerp_value_get_display(&data->house_current);
}

static float get_solar_voltage(const lerp_power_monitor_data_t* data) {
	return lerp_value_get_display(&data->solar_voltage);
}

static float get_solar_current(const lerp_power_monitor_data_t* data) {
	return lerp_value_get_display(&data->solar_current);
}

// Power calculation functions (voltage * current)
// These are non-static so they can be used by power_grid_view.c for numeric labels
float get_starter_power(const lerp_power_monitor_data_t* data) {
	float voltage = lerp_value_get_display(&data->starter_voltage);
	float current = lerp_value_get_display(&data->starter_current);
	return voltage * current;
}

float get_house_power(const lerp_power_monitor_data_t* data) {
	float voltage = lerp_value_get_display(&data->house_voltage);
	float current = lerp_value_get_display(&data->house_current);
	return voltage * current;
}

float get_solar_power(const lerp_power_monitor_data_t* data) {
	float voltage = lerp_value_get_display(&data->solar_voltage);
	float current = lerp_value_get_display(&data->solar_current);
	return voltage * current;
}

// Static gauge instances for detail screen (like historic detail.c)
static bar_graph_gauge_t detail_starter_voltage_gauge = {0};
static bar_graph_gauge_t detail_starter_current_gauge = {0};
static bar_graph_gauge_t detail_house_voltage_gauge = {0};
static bar_graph_gauge_t detail_house_current_gauge = {0};
static bar_graph_gauge_t detail_solar_voltage_gauge = {0};
static bar_graph_gauge_t detail_solar_current_gauge = {0};

// Map all gauge instances to their types and view contexts

gauge_map_entry_t gauge_map[POWER_MONITOR_GAUGE_COUNT] = {

	// Detail view gauges
	[POWER_MONITOR_GAUGE_DETAIL_STARTER_VOLTAGE] = {
		POWER_MONITOR_GAUGE_DETAIL_STARTER_VOLTAGE,
		&detail_starter_voltage_gauge,
		"starter_voltage", "detail_view",
		get_starter_voltage,
		"starter_battery.voltage.error"
	},
	[POWER_MONITOR_GAUGE_DETAIL_STARTER_CURRENT] = {
		POWER_MONITOR_GAUGE_DETAIL_STARTER_CURRENT,
		&detail_starter_current_gauge,
		"starter_current", "detail_view",
		get_starter_current,
		"starter_battery.current.error"
	},
	[POWER_MONITOR_GAUGE_DETAIL_HOUSE_VOLTAGE] = {
		POWER_MONITOR_GAUGE_DETAIL_HOUSE_VOLTAGE,
		&detail_house_voltage_gauge,
		"house_voltage", "detail_view",
		get_house_voltage,
		"house_battery.voltage.error"
	},
	[POWER_MONITOR_GAUGE_DETAIL_HOUSE_CURRENT] = {
		POWER_MONITOR_GAUGE_DETAIL_HOUSE_CURRENT,
		&detail_house_current_gauge,
		"house_current", "detail_view",
		get_house_current,
		"house_battery.current.error"
	},
	[POWER_MONITOR_GAUGE_DETAIL_SOLAR_VOLTAGE] = {
		POWER_MONITOR_GAUGE_DETAIL_SOLAR_VOLTAGE,
		&detail_solar_voltage_gauge,
		"solar_voltage", "detail_view",
		get_solar_voltage,
		"solar_input.voltage.error"
	},
	[POWER_MONITOR_GAUGE_DETAIL_SOLAR_CURRENT] = {
		POWER_MONITOR_GAUGE_DETAIL_SOLAR_CURRENT,
		&detail_solar_current_gauge,
		"solar_current", "detail_view",
		get_solar_current,
		"solar_input.current.error"
	},

	// Power grid view gauges (current_view)
	[POWER_MONITOR_GAUGE_GRID_STARTER_VOLTAGE] = {
		POWER_MONITOR_GAUGE_GRID_STARTER_VOLTAGE,
		&s_starter_voltage_gauge,
		"starter_voltage", "current_view",
		get_starter_voltage,
		"starter_battery.voltage.error"
	},
	[POWER_MONITOR_GAUGE_GRID_HOUSE_VOLTAGE] = {
		POWER_MONITOR_GAUGE_GRID_HOUSE_VOLTAGE,
		&s_house_voltage_gauge,
		"house_voltage", "current_view",
		get_house_voltage,
		"house_battery.voltage.error"
	},
	[POWER_MONITOR_GAUGE_GRID_SOLAR_VOLTAGE] = {
		POWER_MONITOR_GAUGE_GRID_SOLAR_VOLTAGE,
		&s_solar_voltage_gauge,
		"solar_voltage", "current_view",
		get_solar_voltage,
		"solar_input.voltage.error"
	},

	// Amperage grid view gauges (current_view)
	[POWER_MONITOR_GAUGE_GRID_STARTER_CURRENT] = {
		POWER_MONITOR_GAUGE_GRID_STARTER_CURRENT,
		&s_starter_current_gauge,
		"starter_current", "current_view",
		get_starter_current,
		"starter_battery.current.error"
	},
	[POWER_MONITOR_GAUGE_GRID_HOUSE_CURRENT] = {
		POWER_MONITOR_GAUGE_GRID_HOUSE_CURRENT,
		&s_house_current_gauge,
		"house_current", "current_view",
		get_house_current,
		"house_battery.current.error"
	},
	[POWER_MONITOR_GAUGE_GRID_SOLAR_CURRENT] = {
		POWER_MONITOR_GAUGE_GRID_SOLAR_CURRENT,
		&s_solar_current_gauge,
		"solar_current", "current_view",
		get_solar_current,
		"solar_input.current.error"
	},

	// Power grid view gauges (current_view) - wattage
	// These gauges display power (voltage * current), but should use voltage timeline durations
	[POWER_MONITOR_GAUGE_GRID_STARTER_POWER] = {
		POWER_MONITOR_GAUGE_GRID_STARTER_POWER,
		&s_starter_power_gauge,
		"starter_voltage", "current_view",
		get_starter_power,
		"starter_battery.power.error"
	},
	[POWER_MONITOR_GAUGE_GRID_HOUSE_POWER] = {
		POWER_MONITOR_GAUGE_GRID_HOUSE_POWER,
		&s_house_power_gauge,
		"house_voltage", "current_view",
		get_house_power,
		"house_battery.power.error"
	},
	[POWER_MONITOR_GAUGE_GRID_SOLAR_POWER] = {
		POWER_MONITOR_GAUGE_GRID_SOLAR_POWER,
		&s_solar_power_gauge,
		"solar_voltage", "current_view",
		get_solar_power,
		"solar_input.power.error"
	},

	// Single view gauges (current_view) - pointers will be updated at runtime
	[POWER_MONITOR_GAUGE_SINGLE_STARTER_VOLTAGE] = {
		POWER_MONITOR_GAUGE_SINGLE_STARTER_VOLTAGE,
		NULL, // pointer will be updated at runtime
		"starter_voltage",
		"current_view",
		get_starter_voltage,
		"starter_battery.voltage.error"
	},
	[POWER_MONITOR_GAUGE_SINGLE_HOUSE_VOLTAGE] = {
		POWER_MONITOR_GAUGE_SINGLE_HOUSE_VOLTAGE,
		NULL, // pointer will be updated at runtime
		"house_voltage",
		"current_view",
		get_house_voltage,
		"house_battery.voltage.error"
	},
	[POWER_MONITOR_GAUGE_SINGLE_SOLAR_VOLTAGE] = {
		POWER_MONITOR_GAUGE_SINGLE_SOLAR_VOLTAGE,
		NULL, // pointer will be updated at runtime
		"solar_voltage",
		"current_view",
		get_solar_voltage,
		"solar_input.voltage.error"
	},
	[POWER_MONITOR_GAUGE_SINGLE_STARTER_CURRENT] = {
		POWER_MONITOR_GAUGE_SINGLE_STARTER_CURRENT,
		NULL, // pointer will be updated at runtime
		"starter_current",
		"current_view",
		get_starter_current,
		"starter_battery.current.error"
	},
	[POWER_MONITOR_GAUGE_SINGLE_HOUSE_CURRENT] = {
		POWER_MONITOR_GAUGE_SINGLE_HOUSE_CURRENT,
		NULL, // pointer will be updated at runtime
		"house_current",
		"current_view",
		get_house_current,
		"house_battery.current.error"
	},
	[POWER_MONITOR_GAUGE_SINGLE_SOLAR_CURRENT] = {
		POWER_MONITOR_GAUGE_SINGLE_SOLAR_CURRENT,
		NULL, // pointer will be updated at runtime
		"solar_current",
		"current_view",
		get_solar_current,
		"solar_input.current.error"
	},
	[POWER_MONITOR_GAUGE_SINGLE_STARTER_POWER] = {
		POWER_MONITOR_GAUGE_SINGLE_STARTER_POWER,
		NULL, // pointer will be updated at runtime
		"starter_voltage", // Use voltage timeline settings
		"current_view",
		get_starter_power,
		"starter_battery.power.error"
	},
	[POWER_MONITOR_GAUGE_SINGLE_HOUSE_POWER] = {
		POWER_MONITOR_GAUGE_SINGLE_HOUSE_POWER,
		NULL, // pointer will be updated at runtime
		"house_voltage", // Use voltage timeline settings
		"current_view",
		get_house_power,
		"house_battery.power.error"
	},
	[POWER_MONITOR_GAUGE_SINGLE_SOLAR_POWER] = {
		POWER_MONITOR_GAUGE_SINGLE_SOLAR_POWER,
		NULL, // pointer will be updated at runtime
		"solar_voltage", // Use voltage timeline settings
		"current_view",
		get_solar_power,
		"solar_input.power.error"
	}
};


// Update all persistent gauge histories every frame (data-only, no UI)
void power_monitor_update_all_gauge_histories(void)
{
	app_data_store_t* store = app_data_store_get();
	if (!store) return;

	// Get LERP data for smooth display values
	lerp_power_monitor_data_t lerp_data;
	lerp_data_get_current(&lerp_data);

	// Get current timestamp
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	uint32_t current_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

	// Update each gauge instance using the gauge map
	for (int i = 0; i < POWER_MONITOR_GAUGE_COUNT; i++) {
		const gauge_map_entry_t* entry = &gauge_map[i];

		// Get the persistent history for this specific gauge instance (1:1 mapping)
		persistent_gauge_history_t* gauge_history = &store->power_monitor_gauge_histories[i];

		// Calculate max bars for this gauge (only once)
		if (gauge_history->max_count == 0) {
			// Use default values for gauge dimensions
			int bar_width = 2;
			int bar_gap = 3;
			int canvas_width = 200; // Default canvas width
			int bar_spacing = bar_width + bar_gap;
			gauge_history->max_count = canvas_width / bar_spacing;
			if (gauge_history->max_count <= 0) gauge_history->max_count = 1;
			if (gauge_history->max_count > MAX_GAUGE_HISTORY) gauge_history->max_count = MAX_GAUGE_HISTORY;

			// Initialize buffer with NaN to indicate empty/uninitialized
			for( int j = 0; j < gauge_history->max_count; j++ ) {

				gauge_history->values[ j ] = NAN;  // Use NaN to indicate empty
			}

			gauge_history->head = -1;  // Start with invalid head to indicate no data
			gauge_history->has_real_data = false;  // No real data yet
		}

		// Get timeline duration from device state using the correct view type for this gauge instance
		char timeline_path[ 256 ];
		snprintf( timeline_path, sizeof( timeline_path ),
			"power_monitor.gauge_timeline_settings.%s.%s",
			entry->gauge_name, entry->view_type
		);
		int timeline_duration_s = device_state_get_int( timeline_path );
		uint32_t timeline_duration_ms = timeline_duration_s * 1000;

		// For power gauges in current_view, sample using the matching Amperage timeline rate
		// so that V and A are sampled together for P = V*A.
		if (strcmp(entry->view_type, "current_view") == 0 &&
			(entry->data_getter == get_starter_power || entry->data_getter == get_house_power || entry->data_getter == get_solar_power)) {
			const char* current_name = NULL;
			if (entry->data_getter == get_starter_power) current_name = "starter_current";
			else if (entry->data_getter == get_house_power) current_name = "house_current";
			else if (entry->data_getter == get_solar_power) current_name = "solar_current";
			if (current_name) {
				char current_timeline_path[256];
				snprintf(current_timeline_path, sizeof(current_timeline_path),
					"power_monitor.gauge_timeline_settings.%s.%s", current_name, entry->view_type);
				int current_timeline_s = device_state_get_int(current_timeline_path);
				if (current_timeline_s > 0) {
					timeline_duration_ms = (uint32_t)current_timeline_s * 1000;
				}
			}
		}

		// Calculate if we should sample based on timeline duration
		bool should_sample = false;

		if( timeline_duration_ms == 0 ){

			// Realtime - sample every frame
			should_sample = true;
		} else {
			// Timeline-based - calculate interval based on actual gauge buffer size
			// Use the actual number of points in the gauge buffer (hist->max_count)
			uint32_t data_interval_ms = timeline_duration_ms / gauge_history->max_count;

			if(
				gauge_history->last_update_ms == 0 ||
			    ( current_ms - gauge_history->last_update_ms ) >= data_interval_ms
			){

				should_sample = true;
			}
		}


		if( should_sample ){

			// Check if there's a sensor error for this data type
			power_monitor_data_t* power_data = power_monitor_get_data();

			if (entry->error_path && power_data) {
				// error_path string like "house_battery.voltage.error" maps to power_data->house_battery.voltage.error
				const char* path = entry->error_path;
				bool error;

				// Extract the field address and dereference it
				if (strcmp(path, "starter_battery.voltage.error") == 0) error = power_data->starter_battery.voltage.error;
				else if (strcmp(path, "starter_battery.current.error") == 0) error = power_data->starter_battery.current.error;
				else if (strcmp(path, "house_battery.voltage.error") == 0) error = power_data->house_battery.voltage.error;
				else if (strcmp(path, "house_battery.current.error") == 0) error = power_data->house_battery.current.error;
				else if (strcmp(path, "solar_input.voltage.error") == 0) error = power_data->solar_input.voltage.error;
				else if (strcmp(path, "solar_input.current.error") == 0) error = power_data->solar_input.current.error;
				else if (strcmp(path, "starter_battery.power.error") == 0) error = power_data->starter_battery.voltage.error || power_data->starter_battery.current.error;
				else if (strcmp(path, "house_battery.power.error") == 0) error = power_data->house_battery.voltage.error || power_data->house_battery.current.error;
				else if (strcmp(path, "solar_input.power.error") == 0) error = power_data->solar_input.voltage.error || power_data->solar_input.current.error;
				else error = false;

				if (error) continue;
			}

			// Get the current value using function pointer from gauge map
			float current_value = entry->data_getter( &lerp_data );

			// Advance head and write to ring buffer
			if( gauge_history->head == -1 ){

				// First sample
				gauge_history->head = 0;
			} else {

				gauge_history->head = ( gauge_history->head + 1 ) % gauge_history->max_count;
			}

			gauge_history->values[ gauge_history->head ] = current_value;
			gauge_history->last_update_ms = current_ms;
			gauge_history->has_real_data = true;  // Now we have real sensor data

			// Update the gauge canvas with the new data point (only if gauge exists and is initialized)
		if (entry->gauge && entry->gauge->initialized && entry->gauge->canvas && lv_obj_is_valid(entry->gauge->canvas)) {
			bar_graph_gauge_add_data_point( entry->gauge, gauge_history );
		}
		}
	}
}

// Create 6 bar graph gauges in the gauges container (matching original detail.c)
static void power_monitor_create_detail_gauges(lv_obj_t* container)
{
	printf("[D] power_monitor_create_detail_gauges: CALLED with container=%p\n", (void*)container);

	if (!container) {
		printf("[E] power_monitor: Gauges container is NULL\n");
		return;
	}

	// Force layout update to get correct dimensions
	lv_obj_update_layout(container);

	// Get container dimensions after layout update
	lv_coord_t container_width = lv_obj_get_width(container);
	lv_coord_t container_height = lv_obj_get_height(container);

	// Check for valid dimensions
	if (container_width <= 0 || container_height <= 0) {

		printf("[E] power_monitor: Invalid container dimensions: %dx%d\n", container_width, container_height);
		return;
	}

	// Calculate gauge dimensions for manual positioning
	int gauge_padding = 12; // Padding between gauges
	lv_coord_t gauge_width = container_width;  // Full width
	lv_coord_t gauge_height = (container_height - (gauge_padding * 6)) / 6;
	printf("[D] power_monitor: Gauge width: %d, Gauge height: %d\n", gauge_width, gauge_height);

	// Get gauge initial configuration values from device state
	// Voltage gauges
	float starter_baseline = device_state_get_float("power_monitor.starter_baseline_voltage_v");
	float starter_min = device_state_get_float("power_monitor.starter_min_voltage_v");
	float starter_max = device_state_get_float("power_monitor.starter_max_voltage_v");

	float house_baseline = device_state_get_float("power_monitor.house_baseline_voltage_v");
	float house_min = device_state_get_float("power_monitor.house_min_voltage_v");
	float house_max = device_state_get_float("power_monitor.house_max_voltage_v");

	float solar_min = device_state_get_float("power_monitor.solar_min_voltage_v");
	float solar_max = device_state_get_float("power_monitor.solar_max_voltage_v");

	// Current gauges
	float starter_current_baseline = device_state_get_float("power_monitor.starter_baseline_current_a");
	float starter_current_min = device_state_get_float("power_monitor.starter_min_current_a");
	float starter_current_max = device_state_get_float("power_monitor.starter_max_current_a");
	float house_current_baseline = device_state_get_float("power_monitor.house_baseline_current_a");
	float house_current_min = device_state_get_float("power_monitor.house_min_current_a");
	float house_current_max = device_state_get_float("power_monitor.house_max_current_a");
	float solar_current_baseline = device_state_get_float("power_monitor.solar_baseline_current_a");
	float solar_current_min = device_state_get_float("power_monitor.solar_min_current_a");
	float solar_current_max = device_state_get_float("power_monitor.solar_max_current_a");

	// Define gauge configuration array for loop-based creation
	struct {
		bar_graph_gauge_t* gauge;
		const char* title;
		const char* unit;
		const char* y_unit;
		bar_graph_mode_t mode;
		float baseline, min_val, max_val;
		power_monitor_gauge_type_t gauge_type;
	} gauge_configs[] = {
		{&detail_starter_voltage_gauge, "STARTER BATTERY", "V", "V", BAR_GRAPH_MODE_BIPOLAR, starter_baseline, starter_min, starter_max, POWER_MONITOR_GAUGE_DETAIL_STARTER_VOLTAGE},
		{&detail_starter_current_gauge, "STARTER CURRENT", "A", "A", BAR_GRAPH_MODE_BIPOLAR, starter_current_baseline, starter_current_min, starter_current_max, POWER_MONITOR_GAUGE_DETAIL_STARTER_CURRENT},
		{&detail_house_voltage_gauge, "HOUSE BATTERY", "V", "V", BAR_GRAPH_MODE_BIPOLAR, house_baseline, house_min, house_max, POWER_MONITOR_GAUGE_DETAIL_HOUSE_VOLTAGE},
		{&detail_house_current_gauge, "HOUSE CURRENT", "A", "A", BAR_GRAPH_MODE_BIPOLAR, house_current_baseline, house_current_min, house_current_max, POWER_MONITOR_GAUGE_DETAIL_HOUSE_CURRENT},
		{&detail_solar_voltage_gauge, "SOLAR VOLTS", "V", "V", BAR_GRAPH_MODE_POSITIVE_ONLY, 0.0f, 0.0f, 25.0f, POWER_MONITOR_GAUGE_DETAIL_SOLAR_VOLTAGE},
		{&detail_solar_current_gauge, "SOLAR CURRENT", "A", "A", BAR_GRAPH_MODE_BIPOLAR, solar_current_baseline, solar_current_min, solar_current_max, POWER_MONITOR_GAUGE_DETAIL_SOLAR_CURRENT}
	};

	// Create gauges using loop with manual positioning
	for (int i = 0; i < 6; i++) {
		// Calculate Y position for this gauge
		int y_pos = i * (gauge_height + gauge_padding);

		printf("[D] power_monitor: Creating gauge %d at position (%d, %d) size %dx%d\n",
			i, 0, y_pos, gauge_width, gauge_height);

		// Initialize gauge with manual positioning
		bar_graph_gauge_init(gauge_configs[i].gauge, container, 0, y_pos, gauge_width, gauge_height, 2, 3);

		// Manually position the gauge container after initialization
		lv_obj_set_pos(gauge_configs[i].gauge->container, 0, y_pos);

		// Configure gauge
		bar_graph_gauge_configure_advanced(
			gauge_configs[i].gauge,
			gauge_configs[i].mode,
			gauge_configs[i].baseline, gauge_configs[i].min_val, gauge_configs[i].max_val,
			gauge_configs[i].title, gauge_configs[i].unit, gauge_configs[i].y_unit, PALETTE_WARM_WHITE,
			true, true, true // Show title, Show Y-axis, Show Border
		);

		// Apply timeline settings
		power_monitor_update_gauge_timeline_duration(gauge_configs[i].gauge_type);
	}

	// Update labels and ticks for all gauges
	for (int i = 0; i < 6; i++) {

		bar_graph_gauge_update_y_axis_labels(gauge_configs[i].gauge);
	}

}


// Apply alert flashing for current view values
static void power_monitor_apply_current_view_alert_flashing(void)
{
	// Get current data
	power_monitor_data_t *data = power_monitor_get_data();
	if (!data) {
		return;
	}

	// Get alert thresholds
	int starter_lo = device_state_get_int("power_monitor.starter_alert_low_voltage_v");
	int starter_hi = device_state_get_int("power_monitor.starter_alert_high_voltage_v");
	int house_lo = device_state_get_int("power_monitor.house_alert_low_voltage_v");
	int house_hi = device_state_get_int("power_monitor.house_alert_high_voltage_v");
	int solar_lo = device_state_get_int("power_monitor.solar_alert_low_voltage_v");
	int solar_hi = device_state_get_int("power_monitor.solar_alert_high_voltage_v");

	// Blink timing - asymmetric: 1 second on, 0.5 seconds off (1.5 second total cycle)
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	int tick_ms = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
	bool blink_on = (tick_ms % 1500) < 1000;

	// Apply alert flashing to the currently active view (works for both home and detail screen)
	power_monitor_view_type_t current_view = get_current_view_type();
	if (current_view == POWER_MONITOR_VIEW_BAR_GRAPH) {
		power_monitor_voltage_grid_view_apply_alert_flashing(data, starter_lo, starter_hi, house_lo, house_hi, solar_lo, solar_hi, blink_on);
	} else if (current_view == POWER_MONITOR_VIEW_AMPERAGE_GRID) {
		// For amperage grid, use current alerts but we need current thresholds
		int starter_current_lo = device_state_get_int("power_monitor.starter_alert_low_current_a");
		int starter_current_hi = device_state_get_int("power_monitor.starter_alert_high_current_a");
		int house_current_lo = device_state_get_int("power_monitor.house_alert_low_current_a");
		int house_current_hi = device_state_get_int("power_monitor.house_alert_high_current_a");
		int solar_current_lo = device_state_get_int("power_monitor.solar_alert_low_current_a");
		int solar_current_hi = device_state_get_int("power_monitor.solar_alert_high_current_a");
		power_monitor_amperage_grid_view_apply_alert_flashing(data, starter_current_lo, starter_current_hi, house_current_lo, house_current_hi, solar_current_lo, solar_current_hi, blink_on);
	} else if (current_view == POWER_MONITOR_VIEW_POWER) {
		// Get power alert thresholds
		int starter_power_lo = device_state_get_int("power_monitor.starter_alert_low_power_w");
		int starter_power_hi = device_state_get_int("power_monitor.starter_alert_high_power_w");
		int house_power_lo = device_state_get_int("power_monitor.house_alert_low_power_w");
		int house_power_hi = device_state_get_int("power_monitor.house_alert_high_power_w");
		int solar_power_lo = device_state_get_int("power_monitor.solar_alert_low_power_w");
		int solar_power_hi = device_state_get_int("power_monitor.solar_alert_high_power_w");
		power_monitor_power_grid_view_apply_alert_flashing(data, starter_power_lo, starter_power_hi, house_power_lo, house_power_hi, solar_power_lo, solar_power_hi, blink_on);
	} else if (current_view >= POWER_MONITOR_VIEW_NUMERICAL && current_view < POWER_MONITOR_VIEW_COUNT) {
		// Single-value views - alert flashing is handled by the generic single value view component
	} else {
		printf("[W] power_monitor: Unknown view type: %d, skipping alert flashing\n", current_view);
	}
}

// Update detail screen gauges with LERP display data (optimized for performance)
static void power_monitor_update_detail_gauges(void)
{
	// Only update gauges if detail screen exists and gauges are properly initialized
	if (!detail_screen) {

		return; // No detail screen, skip gauge updates
	}

	// Get LERP data for smooth display values
	lerp_power_monitor_data_t lerp_data;
	lerp_data_get_current(&lerp_data);

	if (detail_screen->sensor_data_section) {
		power_monitor_update_sensor_labels_in_detail_screen(detail_screen->sensor_data_section, &lerp_data);
	}
}


// Get internal data (same as public now - data from app store)
static power_monitor_data_t* power_monitor_get_data_internal(void)
{
	app_data_store_t* store = app_data_store_get();
	return store ? store->power_monitor : NULL;
}

// Public function for getting data (subscribed from app data store)
power_monitor_data_t* power_monitor_get_data(void)
{
	app_data_store_t* store = app_data_store_get();
	return store ? store->power_monitor : NULL;
}

// Public function to update detail screen gauge ranges
void power_monitor_update_detail_gauge_ranges(void)
{
	// Only update if detail screen exists
	if (!detail_screen) {
		return;
	}

	// Get current gauge range values from device state
	// Voltage gauges
	float starter_baseline = device_state_get_float("power_monitor.starter_baseline_voltage_v");
	float starter_min = device_state_get_float("power_monitor.starter_min_voltage_v");
	float starter_max = device_state_get_float("power_monitor.starter_max_voltage_v");
	float house_baseline = device_state_get_float("power_monitor.house_baseline_voltage_v");
	float house_min = device_state_get_float("power_monitor.house_min_voltage_v");
	float house_max = device_state_get_float("power_monitor.house_max_voltage_v");
	float solar_min = device_state_get_float("power_monitor.solar_min_voltage_v");
	float solar_max = device_state_get_float("power_monitor.solar_max_voltage_v");

	// Current gauges
	float starter_current_baseline = device_state_get_float("power_monitor.starter_baseline_current_a");
	float starter_current_min = device_state_get_float("power_monitor.starter_min_current_a");
	float starter_current_max = device_state_get_float("power_monitor.starter_max_current_a");
	float house_current_baseline = device_state_get_float("power_monitor.house_baseline_current_a");
	float house_current_min = device_state_get_float("power_monitor.house_min_current_a");
	float house_current_max = device_state_get_float("power_monitor.house_max_current_a");
	float solar_current_baseline = device_state_get_float("power_monitor.solar_baseline_current_a");
	float solar_current_min = device_state_get_float("power_monitor.solar_min_current_a");
	float solar_current_max = device_state_get_float("power_monitor.solar_max_current_a");

	// Update detail screen gauge ranges
	if (detail_starter_voltage_gauge.initialized) {
		bar_graph_gauge_configure_advanced(
			&detail_starter_voltage_gauge,
			BAR_GRAPH_MODE_BIPOLAR,
			starter_baseline,
			starter_min,
			starter_max,
			"STARTER BATTERY", "V", "V", PALETTE_WARM_WHITE,
			true, true, true
		);
	}

	if (detail_house_voltage_gauge.initialized) {
		bar_graph_gauge_configure_advanced(
			&detail_house_voltage_gauge,
			BAR_GRAPH_MODE_BIPOLAR,
			house_baseline,
			house_min,
			house_max,
			"HOUSE BATTERY", "V", "V", PALETTE_WARM_WHITE,
			true, true, true
		);
	}

	if (detail_solar_voltage_gauge.initialized) {
		bar_graph_gauge_configure_advanced(
			&detail_solar_voltage_gauge,
			BAR_GRAPH_MODE_POSITIVE_ONLY,
			0.0f,
			solar_min,
			solar_max,
			"SOLAR INPUT", "V", "V", PALETTE_WARM_WHITE,
			true, true, true
		);
	}

	// Update current gauge ranges
	if (detail_starter_current_gauge.initialized) {
		bar_graph_gauge_configure_advanced(
			&detail_starter_current_gauge,
			BAR_GRAPH_MODE_BIPOLAR,
			starter_current_baseline,
			starter_current_min,
			starter_current_max,
			"STARTER CURRENT", "A", "A", PALETTE_WARM_WHITE,
			true, true, true
		);
	}

	if (detail_house_current_gauge.initialized) {
		bar_graph_gauge_configure_advanced(
			&detail_house_current_gauge,
			BAR_GRAPH_MODE_BIPOLAR,
			house_current_baseline,
			house_current_min,
			house_current_max,
			"HOUSE CURRENT", "A", "A", PALETTE_WARM_WHITE,
			true, true, true
		);
	}

	if (detail_solar_current_gauge.initialized) {
		bar_graph_gauge_configure_advanced(
			&detail_solar_current_gauge,
			BAR_GRAPH_MODE_BIPOLAR,
			solar_current_baseline,
			solar_current_min,
			solar_current_max,
			"SOLAR CURRENT", "A", "A", PALETTE_WARM_WHITE,
			true, true, true
		);
	}
}


// Data-only update function (no UI structure changes)
// NOTE: Data updates now happen in app_data_store_update() in main.c
// This function only does UI updates based on that data
void power_monitor_update_data_only(void)
{


	// Data is updated centrally in main.c via app_data_store_update()
	// Here we only update UI surfaces based on the data

	// Update detail screen gauges data (if detail screen exists)
	power_monitor_update_detail_gauges();

	// Update view data (this updates the data in the views, not the UI structure)
	power_monitor_voltage_grid_view_update_data();
	power_monitor_amperage_grid_view_update_data();
	power_monitor_power_grid_view_update_data();
	power_monitor_starter_voltage_view_update_data();
	power_monitor_house_voltage_view_update_data();
	power_monitor_solar_voltage_view_update_data();
	power_monitor_starter_current_view_update_data();
	power_monitor_house_current_view_update_data();
	power_monitor_solar_current_view_update_data();
	power_monitor_starter_power_view_update_data();
	power_monitor_house_power_view_update_data();
	power_monitor_solar_power_view_update_data();

	// Current view alert flashing (still handled by power monitor)
	power_monitor_apply_current_view_alert_flashing();
}

// Force all gauges to redraw from persistent history (useful after modal changes)
void power_monitor_force_gauge_redraw_from_history(void)
{

	app_data_store_t* store = app_data_store_get();
	if (!store) {

		return;
	}

	// Force all gauges in the gauge map to redraw from their persistent history
	for (int i = 0; i < POWER_MONITOR_GAUGE_COUNT; i++) {
		const gauge_map_entry_t* entry = &gauge_map[i];

		// Skip if gauge is not initialized
		if (!entry->gauge || !entry->gauge->initialized) {
			continue;
		}

		// Get the persistent history for this gauge
		persistent_gauge_history_t* hist = &store->power_monitor_gauge_histories[i];

		// Reset the gauge's last_rendered_head to force a full redraw
		entry->gauge->last_rendered_head = -1;

		// Force the gauge to redraw from persistent history
		bar_graph_gauge_draw_all_data(entry->gauge, hist);
	}
}




// Public function for initializing with default view (required by main.c)
void power_monitor_init_with_default_view(power_monitor_view_type_t default_view)
{
	power_monitor_init();
	power_monitor_set_current_view_type((power_monitor_view_type_t)default_view);
}

// Default setting mapping structure
typedef struct {
	const char* path;
	double value;
} power_monitor_default_t;


// Initialize power monitor defaults in device state only if values don't exist
static void power_monitor_init_defaults(void)
{
	// Map all default settings
	power_monitor_default_t defaults[] = {
		// Gauge timeline settings - named properties for each gauge
		{"power_monitor.gauge_timeline_settings.starter_voltage.current_view", POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS},
		{"power_monitor.gauge_timeline_settings.starter_voltage.detail_view", POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS},

		{"power_monitor.gauge_timeline_settings.starter_current.current_view", POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS},
		{"power_monitor.gauge_timeline_settings.starter_current.detail_view", POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS},

		{"power_monitor.gauge_timeline_settings.house_voltage.current_view", POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS},
		{"power_monitor.gauge_timeline_settings.house_voltage.detail_view", POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS},

		{"power_monitor.gauge_timeline_settings.house_current.current_view", POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS},
		{"power_monitor.gauge_timeline_settings.house_current.detail_view", POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS},

		{"power_monitor.gauge_timeline_settings.solar_voltage.current_view", POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS},
		{"power_monitor.gauge_timeline_settings.solar_voltage.detail_view", POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS},

		{"power_monitor.gauge_timeline_settings.solar_current.current_view", POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS},
		{"power_monitor.gauge_timeline_settings.solar_current.detail_view", POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS},

		// Power gauge timeline settings
		{"power_monitor.gauge_timeline_settings.starter_power.current_view", POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS},
		{"power_monitor.gauge_timeline_settings.starter_power.detail_view", POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS},

		{"power_monitor.gauge_timeline_settings.house_power.current_view", POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS},
		{"power_monitor.gauge_timeline_settings.house_power.detail_view", POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS},

		{"power_monitor.gauge_timeline_settings.solar_power.current_view", POWER_MONITOR_DEFAULT_TIMELINE_CURRENT_VIEW_SECONDS},
		{"power_monitor.gauge_timeline_settings.solar_power.detail_view", POWER_MONITOR_DEFAULT_TIMELINE_DETAIL_VIEW_SECONDS},

		// Starter battery settings
		{"power_monitor.starter_alert_low_voltage_v", (double)POWER_MONITOR_DEFAULT_STARTER_ALERT_LOW_VOLTAGE_V},
		{"power_monitor.starter_alert_high_voltage_v", (double)POWER_MONITOR_DEFAULT_STARTER_ALERT_HIGH_VOLTAGE_V},
		{"power_monitor.starter_baseline_voltage_v", (double)POWER_MONITOR_DEFAULT_STARTER_BASELINE_VOLTAGE_V},
		{"power_monitor.starter_min_voltage_v", (double)POWER_MONITOR_DEFAULT_STARTER_MIN_VOLTAGE_V},
		{"power_monitor.starter_max_voltage_v", (double)POWER_MONITOR_DEFAULT_STARTER_MAX_VOLTAGE_V},

		// Starter current settings
		{"power_monitor.starter_alert_low_current_a", (double)-30.0f},
		{"power_monitor.starter_alert_high_current_a", (double)30.0f},
		{"power_monitor.starter_baseline_current_a", (double)0.0f},
		{"power_monitor.starter_min_current_a", (double)-40.0f},
		{"power_monitor.starter_max_current_a", (double)40.0f},

		// House battery settings
		{"power_monitor.house_alert_low_voltage_v", (double)POWER_MONITOR_DEFAULT_HOUSE_ALERT_LOW_VOLTAGE_V},
		{"power_monitor.house_alert_high_voltage_v", (double)POWER_MONITOR_DEFAULT_HOUSE_ALERT_HIGH_VOLTAGE_V},
		{"power_monitor.house_baseline_voltage_v", (double)POWER_MONITOR_DEFAULT_HOUSE_BASELINE_VOLTAGE_V},
		{"power_monitor.house_min_voltage_v", (double)POWER_MONITOR_DEFAULT_HOUSE_MIN_VOLTAGE_V},
		{"power_monitor.house_max_voltage_v", (double)POWER_MONITOR_DEFAULT_HOUSE_MAX_VOLTAGE_V},

		// House current settings
		{"power_monitor.house_alert_low_current_a", (double)-30.0f},
		{"power_monitor.house_alert_high_current_a", (double)30.0f},
		{"power_monitor.house_baseline_current_a", (double)0.0f},
		{"power_monitor.house_min_current_a", (double)-40.0f},
		{"power_monitor.house_max_current_a", (double)40.0f},

		// Solar settings
		{"power_monitor.solar_alert_low_voltage_v", (double)POWER_MONITOR_DEFAULT_SOLAR_ALERT_LOW_VOLTAGE_V},
		{"power_monitor.solar_alert_high_voltage_v", (double)POWER_MONITOR_DEFAULT_SOLAR_ALERT_HIGH_VOLTAGE_V},
		{"power_monitor.solar_min_voltage_v", (double)POWER_MONITOR_DEFAULT_SOLAR_MIN_VOLTAGE_V},
		{"power_monitor.solar_max_voltage_v", (double)POWER_MONITOR_DEFAULT_SOLAR_MAX_VOLTAGE_V},

		// Solar current settings
		{"power_monitor.solar_alert_low_current_a", (double)-30.0f},
		{"power_monitor.solar_alert_high_current_a", (double)30.0f},
		{"power_monitor.solar_baseline_current_a", (double)0.0f},
		{"power_monitor.solar_min_current_a", (double)-40.0f},
		{"power_monitor.solar_max_current_a", (double)40.0f},

		// Power gauge settings
		{"power_monitor.starter_alert_low_power_w", (double)-2000.0f},
		{"power_monitor.starter_alert_high_power_w", (double)2000.0f},
		{"power_monitor.starter_baseline_power_w", (double)0.0f},
		{"power_monitor.starter_min_power_w", (double)-3000.0f},
		{"power_monitor.starter_max_power_w", (double)3000.0f},

		{"power_monitor.house_alert_low_power_w", (double)-1000.0f},
		{"power_monitor.house_alert_high_power_w", (double)1000.0f},
		{"power_monitor.house_baseline_power_w", (double)0.0f},
		{"power_monitor.house_min_power_w", (double)-1500.0f},
		{"power_monitor.house_max_power_w", (double)1500.0f},

		{"power_monitor.solar_alert_low_power_w", (double)10.0f},
		{"power_monitor.solar_alert_high_power_w", (double)2500.0f},
		{"power_monitor.solar_min_power_w", (double)0.0f},
		{"power_monitor.solar_max_power_w", (double)3000.0f},
	};

	// Process all defaults
	for (size_t i = 0; i < sizeof(defaults) / sizeof(defaults[0]); i++) {

		if (!device_state_path_exists(defaults[i].path)) {

			device_state_set_value(defaults[i].path, defaults[i].value);
		}
	}
}

// Module interface implementation
void power_monitor_init(void)
{

	// Initialize power monitor defaults
	power_monitor_init_defaults();



	// Initialize shared current view manager (for backward compatibility)
	current_view_manager_init(sizeof(available_views) / sizeof(available_views[0]));

	// Initialize default view index only if not already set
	extern int module_screen_view_get_view_index(const char *module_name);
	int current_index = module_screen_view_get_view_index("power-monitor");
	if (current_index < 0 || current_index >= POWER_MONITOR_VIEW_COUNT) {
		printf("[I] power_monitor: Setting initial view index to 0 (voltage grid view)\n");
		power_monitor_set_view_index(0);
	} else {
		printf("[I] power_monitor: Using existing view index %d\n", current_index);
	}

	// Navigation callbacks removed - using direct function calls

	// Initialize data (now in app data store)
	power_monitor_data_t* data = power_monitor_get_data();
	if (data) {
		memset(data, 0, sizeof(power_monitor_data_t));
	}

	// Initialize histories as empty only if not already populated
	static bool histories_initialized = false;
	if (!histories_initialized) {
		memset(s_histories, 0, sizeof(s_histories));
		histories_initialized = true;
	}

	// LERP is initialized in main.c before modules - don't call again

	// Initialize widget
	power_monitor_init_widget();

	// Initialize gauge data source pointers

}

// =============================================================================
// DISPLAY MODULE BASE LIFECYCLE
// =============================================================================

/**
 * @brief Create module UI in a container (called when screen shows module)
 * This is for home screen tiles - detail screen uses its own create/destroy
 */
static void power_monitor_create_in_container(lv_obj_t* container)
{
	printf("[I] power_monitor: Creating module UI in container\n");
	if (!container) return;

	// Render the current view directly into the container
	power_monitor_render_current_view(container);

	// Add touch callback for home screen navigation
	lv_obj_add_event_cb(container, power_monitor_home_current_view_touch_cb, LV_EVENT_CLICKED, NULL);
}

/**
 * @brief Destroy module UI (called when screen hides module)
 */
static void power_monitor_destroy_ui(void)
{
	printf("[I] power_monitor: Destroying module UI\n");

	// Modal handling is now done by individual toggle functions

	// Note: Home screen containers are destroyed by home_screen itself
	// Detail screen has its own destroy via power_monitor_destroy_detail_screen()
}

/**
 * @brief Render module (per-frame UI updates only, no data writes)
 */
static void power_monitor_render_ui(void)
{
	// Data is updated centrally in ui_update_timer_callback via module update
	// Here we only update UI surfaces
	power_monitor_update_detail_gauges();
	power_monitor_voltage_grid_view_update_data();
	power_monitor_power_grid_view_update_data();
	power_monitor_starter_voltage_view_update_data();
	power_monitor_apply_current_view_alert_flashing();
}

// Public lifecycle functions (for compatibility and explicit calls)

/**
 * @brief Lifecycle: create (once) - initialize UI elements and data
 */
void power_monitor_create(void)
{
	// Idempotent: rely on power_monitor_init to set defaults and histories
	power_monitor_init();

	// Initialize the display module base
	display_module_base_init(
		&s_module_base,
		"power-monitor",
		NULL,  // UI state tracked separately in s_ui_state
		power_monitor_create_in_container,
		power_monitor_destroy_ui,
		power_monitor_render_ui
	);
}

/**
 * @brief Lifecycle: destroy - gracefully destroy all UI elements
 */
void power_monitor_destroy(void)
{
	display_module_base_destroy(&s_module_base);
}

/**
 * @brief Lifecycle: render - per-frame UI updates only (no data writes)
 */
void power_monitor_render(void)
{
	display_module_base_render(&s_module_base);
}

void power_monitor_show_in_container(lv_obj_t *container)
{

	if (!container) {
		printf("[E] power_monitor: Container is NULL\n");
		return;
	}

	// Create current view content directly in the container
	power_monitor_create_current_view_content(container);
}

void power_monitor_show_in_container_home(lv_obj_t *container)
{

	// Render the current view directly into the container
	power_monitor_render_current_view(container);

	// Add touch callback for home screen navigation
	lv_obj_add_event_cb(container, power_monitor_home_current_view_touch_cb, LV_EVENT_CLICKED, NULL);
}

void power_monitor_cycle_current_view(void)
{

	// Simple guard against rapid cycling
	if (s_ui_state.detail_view_needs_refresh || s_ui_state.view_destroy_in_progress || s_ui_state.navigation_teardown_in_progress) {

		return;
	}

	// Use shared current view manager instead of local cycling logic
	extern void current_view_manager_cycle_to_next(void);
	current_view_manager_cycle_to_next();

	// Mark that we need to re-render the detail view on next update
	// This avoids immediate re-rendering which can cause crashes
	extern screen_type_t screen_navigation_get_current_screen(void);
	screen_type_t current_screen = screen_navigation_get_current_screen();

	if( current_screen == SCREEN_DETAIL_VIEW ){

		s_ui_state.detail_view_needs_refresh = true;
	}
}

void power_monitor_cleanup_internal(void)
{

	// Clean up containers
	if (power_monitor_container) {
		lv_obj_del(power_monitor_container);
		power_monitor_container = NULL;
	}

	// Clean up detail screen
	if (detail_screen) {
		detail_screen_destroy(detail_screen);
		detail_screen = NULL;
	}

	// Clean up current view manager
	current_view_manager_cleanup();

	// Starter voltage view cleanup is handled internally


	// Reset static gauge variables to prevent memory conflicts
	extern void power_monitor_reset_static_gauges(void);
	extern void power_monitor_reset_starter_voltage_static_gauge(void);
	power_monitor_reset_static_gauges();
	power_monitor_reset_starter_voltage_static_gauge();

	// Reset detail gauge variables to prevent crashes during updates
	memset(&detail_starter_voltage_gauge, 0, sizeof(bar_graph_gauge_t));
	memset(&detail_starter_current_gauge, 0, sizeof(bar_graph_gauge_t));
	memset(&detail_house_voltage_gauge, 0, sizeof(bar_graph_gauge_t));
	memset(&detail_house_current_gauge, 0, sizeof(bar_graph_gauge_t));
	memset(&detail_solar_voltage_gauge, 0, sizeof(bar_graph_gauge_t));
	memset(&detail_solar_current_gauge, 0, sizeof(bar_graph_gauge_t));
	printf("[I] power_monitor: Detail gauge variables reset\n");
}

// Update timeline for a specific gauge instance
void power_monitor_update_gauge_timeline_duration(power_monitor_gauge_type_t gauge_type)
{
	// Validate gauge type
	if (gauge_type >= POWER_MONITOR_GAUGE_COUNT) {
		printf("[E] power_monitor: Invalid gauge type %d\n", gauge_type);
		return;
	}

	// Get the gauge map entry for this specific gauge instance
	const gauge_map_entry_t* entry = &gauge_map[gauge_type];

	// Update the gauge if it exists and is initialized
	if (entry->gauge && entry->gauge->initialized) {
		char timeline_path[256];
		snprintf(timeline_path, sizeof(timeline_path), "power_monitor.gauge_timeline_settings.%s.%s", entry->gauge_name, entry->view_type);
		int timeline_duration = device_state_get_int(timeline_path);
		uint32_t timeline_duration_ms = timeline_duration * 1000;
		bar_graph_gauge_set_timeline_duration(entry->gauge, timeline_duration_ms);
	}
}

// Update timeline for all gauge instances that use a specific data type
void power_monitor_update_data_type_timeline_duration(power_monitor_data_type_t data_type, const char* view_type)
{
	// Find all gauge instances that use this data type
	for (int i = 0; i < POWER_MONITOR_GAUGE_COUNT; i++) {
		const gauge_map_entry_t* entry = &gauge_map[i];

		// Check if this gauge uses the same data type and view type
		// Convert data_type to gauge name for comparison
		const char* target_gauge_name = NULL;
		switch (data_type) {
			case POWER_MONITOR_DATA_STARTER_VOLTAGE:
				target_gauge_name = "starter_voltage";
				break;
			case POWER_MONITOR_DATA_STARTER_CURRENT:
				target_gauge_name = "starter_current";
				break;
			case POWER_MONITOR_DATA_HOUSE_VOLTAGE:
				target_gauge_name = "house_voltage";
				break;
			case POWER_MONITOR_DATA_HOUSE_CURRENT:
				target_gauge_name = "house_current";
				break;
			case POWER_MONITOR_DATA_SOLAR_VOLTAGE:
				target_gauge_name = "solar_voltage";
				break;
			case POWER_MONITOR_DATA_SOLAR_CURRENT:
				target_gauge_name = "solar_current";
				break;
			default:
				continue;
		}

		if (strcmp(entry->gauge_name, target_gauge_name) == 0 && strcmp(entry->view_type, view_type) == 0) {
			// Update this gauge instance
			if (entry->gauge && entry->gauge->initialized) {
				char timeline_path[256];
				snprintf(timeline_path, sizeof(timeline_path), "power_monitor.gauge_timeline_settings.%s.%s", entry->gauge_name, entry->view_type);
				int timeline_duration = device_state_get_int(timeline_path);
				uint32_t timeline_duration_ms = timeline_duration * 1000;
				bar_graph_gauge_set_timeline_duration(entry->gauge, timeline_duration_ms);
			}
		}
	}
}

// Update single view gauge pointer at runtime
void power_monitor_update_single_view_gauge_pointer(void)
{
	// Update starter voltage
	if (single_view_starter_voltage && single_view_starter_voltage->initialized) {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_STARTER_VOLTAGE].gauge = &single_view_starter_voltage->gauge;
	} else {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_STARTER_VOLTAGE].gauge = NULL;
	}
	// Update house voltage
	if (single_view_house_voltage && single_view_house_voltage->initialized) {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_HOUSE_VOLTAGE].gauge = &single_view_house_voltage->gauge;
	} else {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_HOUSE_VOLTAGE].gauge = NULL;
	}
	// Update solar voltage
	if (single_view_solar_voltage && single_view_solar_voltage->initialized) {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_SOLAR_VOLTAGE].gauge = &single_view_solar_voltage->gauge;
	} else {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_SOLAR_VOLTAGE].gauge = NULL;
	}
	// Update starter current
	if (single_view_starter_current && single_view_starter_current->initialized) {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_STARTER_CURRENT].gauge = &single_view_starter_current->gauge;
	} else {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_STARTER_CURRENT].gauge = NULL;
	}
	// Update house current
	if (single_view_house_current && single_view_house_current->initialized) {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_HOUSE_CURRENT].gauge = &single_view_house_current->gauge;
	} else {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_HOUSE_CURRENT].gauge = NULL;
	}
	// Update solar current
	if (single_view_solar_current && single_view_solar_current->initialized) {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_SOLAR_CURRENT].gauge = &single_view_solar_current->gauge;
	} else {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_SOLAR_CURRENT].gauge = NULL;
	}
	// Update starter power
	if (single_view_starter_power && single_view_starter_power->initialized) {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_STARTER_POWER].gauge = &single_view_starter_power->gauge;
	} else {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_STARTER_POWER].gauge = NULL;
	}
	// Update house power
	if (single_view_house_power && single_view_house_power->initialized) {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_HOUSE_POWER].gauge = &single_view_house_power->gauge;
	} else {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_HOUSE_POWER].gauge = NULL;
	}
	// Update solar power
	if (single_view_solar_power && single_view_solar_power->initialized) {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_SOLAR_POWER].gauge = &single_view_solar_power->gauge;
	} else {
		gauge_map[POWER_MONITOR_GAUGE_SINGLE_SOLAR_POWER].gauge = NULL;
	}
}

void power_monitor_cleanup(void)
{

	power_monitor_cleanup_internal();
}

// Button configurations for power monitor
static detail_button_config_t power_monitor_buttons[] = {
	{"ALERTS", power_monitor_handle_alerts_button},
	{"TIMELINE", power_monitor_handle_timeline_button}
};

// Callback functions for detail screen
static void power_monitor_on_current_view_created(lv_obj_t* container)
{
	printf("[I] power_monitor: Current view container created callback\n");
	power_monitor_create_current_view_content(container);
}

static void power_monitor_on_gauges_created(lv_obj_t* container)
{
	printf("[I] power_monitor: Gauges container created callback\n");
	power_monitor_create_detail_gauges(container);
	// Force layout so gauges have correct widths
	if (container && lv_obj_is_valid(container)) {
		lv_obj_update_layout(container);
	}

	// Gauges are now seeded directly when bar_graph_gauge_add_data_point is called
}


static void power_monitor_on_sensor_data_created(lv_obj_t* container)
{
	printf("[I] power_monitor: Sensor data container created callback\n");
	power_monitor_create_sensor_labels_in_detail_screen(container);
}

static void power_monitor_on_view_clicked(void)
{
	printf("[I] power_monitor: *** VIEW CLICKED CALLBACK CALLED ***\n");
	printf("[I] power_monitor: View clicked callback - cycling current view\n");
	power_monitor_cycle_current_view();
	printf("[I] power_monitor: View cycling call completed\n");
}

// Power monitor specific sensor label creation function
void power_monitor_create_sensor_labels_in_detail_screen(lv_obj_t* container)
{
	if (!container) {
		printf("[E] power_monitor: Container is NULL for sensor labels\n");
		return;
	}

	printf("[I] power_monitor: Creating sensor data labels in detail screen\n");

	// Create sensor data labels with horizontal layout (label: value on same line)
	lv_color_t labelColor = PALETTE_GRAY;
	lv_color_t valueColor = PALETTE_GREEN;
	lv_color_t groupColor = PALETTE_WHITE;

	// Sensor data layout: Group headers + horizontal label:value pairs
	const char* group_names[] = {"Starter Battery", "House Battery", "Solar Input"};
	const char* value_labels[] = {"Volts:", "Amperes:"};

	// Create 3 groups (Starter, House, Solar)
	for (int group = 0; group < 3; group++) {

		// Group header
		lv_obj_t* group_label = lv_label_create(container);
		lv_obj_set_style_text_font(group_label, &lv_font_montserrat_16, 0);
		lv_obj_set_style_text_color(group_label, groupColor, 0);
		lv_label_set_text(group_label, group_names[group]);
		lv_obj_set_style_pad_top(group_label, group == 0 ? 5 : 10, 0); // Extra spacing between groups

		// Create 2 value pairs (Volts, Amperes) for each group
		for (int value_type = 0; value_type < 2; value_type++) {

			// Create horizontal container for label:value pair
			lv_obj_t* value_row = lv_obj_create(container);
			lv_obj_set_size(value_row, LV_PCT(100), LV_SIZE_CONTENT);
			lv_obj_set_style_bg_color(value_row, PALETTE_BLACK, 0);
			lv_obj_set_style_bg_opa(value_row, LV_OPA_COVER, 0);
			lv_obj_set_style_border_width(value_row, 0, 0);
			lv_obj_set_style_pad_all(value_row, 2, 0);
			lv_obj_clear_flag(value_row, LV_OBJ_FLAG_SCROLLABLE);

			// Set up horizontal flexbox for label:value layout
			lv_obj_set_flex_flow(value_row, LV_FLEX_FLOW_ROW);
			lv_obj_set_flex_align(value_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

			// Label (left side)
			lv_obj_t* label = lv_label_create(value_row);
			lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0);
			lv_obj_set_style_text_color(label, labelColor, 0);
			lv_label_set_text(label, value_labels[value_type]);

			// Value (right side) - will be updated by power_monitor_update_sensor_labels_in_detail_screen
			lv_obj_t* value = lv_label_create(value_row);
			lv_obj_set_style_text_font(value, &lv_font_noplato_24, 0);
			lv_obj_set_style_text_color(value, valueColor, 0);
			lv_obj_set_style_text_align(value, LV_TEXT_ALIGN_RIGHT, 0);
			lv_label_set_text(value, "0.0");

			// Store reference for direct updates to data
			power_monitor_data_t* data = power_monitor_get_data();
			if (data) {
				if (group == 0) { // Starter Battery
					if (value_type == 0) data->sensor_labels.starter_voltage = value;
					else data->sensor_labels.starter_current = value;
				} else if (group == 1) { // House Battery
					if (value_type == 0) data->sensor_labels.house_voltage = value;
					else data->sensor_labels.house_current = value;
				} else if (group == 2) { // Solar Input
					if (value_type == 0) data->sensor_labels.solar_voltage = value;
					else data->sensor_labels.solar_current = value;
				}
			}
		}
	}

	printf("[I] power_monitor: Sensor data labels created successfully\n");
}

// Power monitor specific sensor label update function
void power_monitor_update_sensor_labels_in_detail_screen(lv_obj_t* sensor_section, const lerp_power_monitor_data_t* lerp_data)
{
	if (!lerp_data) {
		return;
	}

	// Get alert thresholds for voltage values
	int starter_lo = device_state_get_int("power_monitor.starter_alert_low_voltage_v");
	int starter_hi = device_state_get_int("power_monitor.starter_alert_high_voltage_v");
	int house_lo = device_state_get_int("power_monitor.house_alert_low_voltage_v");
	int house_hi = device_state_get_int("power_monitor.house_alert_high_voltage_v");
	int solar_lo = device_state_get_int("power_monitor.solar_alert_low_voltage_v");
	int solar_hi = device_state_get_int("power_monitor.solar_alert_high_voltage_v");

	// Get alert thresholds for current values
	int starter_current_lo = device_state_get_int("power_monitor.starter_alert_low_current_a");
	int starter_current_hi = device_state_get_int("power_monitor.starter_alert_high_current_a");
	int house_current_lo = device_state_get_int("power_monitor.house_alert_low_current_a");
	int house_current_hi = device_state_get_int("power_monitor.house_alert_high_current_a");
	int solar_current_lo = device_state_get_int("power_monitor.solar_alert_low_current_a");
	int solar_current_hi = device_state_get_int("power_monitor.solar_alert_high_current_a");

	// Get raw values for threshold checking
	float starter_v_raw = lerp_value_get_raw(&lerp_data->starter_voltage);
	float house_v_raw = lerp_value_get_raw(&lerp_data->house_voltage);
	float solar_v_raw = lerp_value_get_raw(&lerp_data->solar_voltage);
	float starter_c_raw = lerp_value_get_raw(&lerp_data->starter_current);
	float house_c_raw = lerp_value_get_raw(&lerp_data->house_current);
	float solar_c_raw = lerp_value_get_raw(&lerp_data->solar_current);

	// Check if values are outside alert thresholds
	bool starter_alert = (starter_v_raw <= (float)starter_lo) || (starter_v_raw >= (float)starter_hi);
	bool house_alert = (house_v_raw <= (float)house_lo) || (house_v_raw >= (float)house_hi);
	bool solar_alert = (solar_v_raw <= (float)solar_lo) || (solar_v_raw >= (float)solar_hi);
	bool starter_current_alert = (starter_c_raw <= (float)starter_current_lo) || (starter_c_raw >= (float)starter_current_hi);
	bool house_current_alert = (house_c_raw <= (float)house_current_lo) || (house_c_raw >= (float)house_current_hi);
	bool solar_current_alert = (solar_c_raw <= (float)solar_current_lo) || (solar_c_raw >= (float)solar_current_hi);

	// Update values using direct references - much cleaner!
	char sensor_text[32];

	// Update starter battery values
	power_monitor_data_t* power_data_pointer = power_monitor_get_data();
	if (!power_data_pointer) return;

	// Error states are being checked and passed to formatting function

	number_formatting_config_t config = {
		.label = power_data_pointer->sensor_labels.starter_voltage,
		.font = &lv_font_noplato_24, // Use monospace font
		.color = PALETTE_WHITE,
		.warning_color = PALETTE_YELLOW,
		.error_color = lv_color_hex(0xFF0000), // Red for errors
		.show_warning = starter_alert && !power_data_pointer->starter_battery.voltage.error,
		.show_error = power_data_pointer->starter_battery.voltage.error,
		.warning_icon_size = WARNING_ICON_SIZE_30,
		.number_alignment = LABEL_ALIGN_RIGHT, // Right-justified for detail screen
		.warning_alignment = LABEL_ALIGN_RIGHT
	};
	format_and_display_number(lerp_value_get_display(&lerp_data->starter_voltage), &config);

	config.label = power_data_pointer->sensor_labels.starter_current;
	config.show_warning = starter_current_alert && !power_data_pointer->starter_battery.current.error;
	config.show_error = power_data_pointer->starter_battery.current.error;
	format_and_display_number(lerp_value_get_display(&lerp_data->starter_current), &config);

	// Update house battery values
	config.label = power_data_pointer->sensor_labels.house_voltage;
	config.show_warning = house_alert && !power_data_pointer->house_battery.voltage.error;
	config.show_error = power_data_pointer->house_battery.voltage.error;
	format_and_display_number(lerp_value_get_display(&lerp_data->house_voltage), &config);

	config.label = power_data_pointer->sensor_labels.house_current;
	config.show_warning = house_current_alert && !power_data_pointer->house_battery.current.error;
	config.show_error = power_data_pointer->house_battery.current.error;
	format_and_display_number(lerp_value_get_display(&lerp_data->house_current), &config);

	// Update solar input values
	config.label = power_data_pointer->sensor_labels.solar_voltage;
	config.show_warning = solar_alert && !power_data_pointer->solar_input.voltage.error;
	config.show_error = power_data_pointer->solar_input.voltage.error;
	format_and_display_number(lerp_value_get_display(&lerp_data->solar_voltage), &config);

	config.label = power_data_pointer->sensor_labels.solar_current;
	config.show_warning = solar_current_alert && !power_data_pointer->solar_input.current.error;
	config.show_error = power_data_pointer->solar_input.current.error;
	format_and_display_number(lerp_value_get_display(&lerp_data->solar_current), &config);
}

// Detail screen management
void power_monitor_create_detail_screen(void)
{
	printf("[I] power_monitor: === CREATING DETAIL SCREEN (V2) ===\n");

	if (detail_screen) {
		printf("[W] power_monitor: Detail screen already exists\n");
		return;
	}

	// Create detail screen using shared template
	detail_screen_config_t config = {
		.module_name = "power-monitor",
		.display_name = "POWER MONITOR",
		.show_gauges_section = true,
		.show_settings_button = true,
		.show_status_indicators = false,
		.setting_buttons = power_monitor_buttons,
		.setting_buttons_count = 2,
		.on_back_clicked = power_monitor_handle_back_button,
		.on_view_clicked = power_monitor_on_view_clicked,
		.on_current_view_created = power_monitor_on_current_view_created,
		.on_gauges_created = power_monitor_on_gauges_created,
		.on_sensor_data_created = power_monitor_on_sensor_data_created,
		.overlay_creator = NULL,
		.overlay_user_data = NULL
	};

	detail_screen = detail_screen_create(&config);
	if (detail_screen) {
		printf("[I] power_monitor: Detail screen created successfully\n");
		// Content creation is now handled by callbacks
	} else {
		printf("[E] power_monitor: Failed to create detail screen\n");
	}
}

void power_monitor_show_detail_screen(void)
{
	printf("[I] power_monitor: === SHOW DETAIL SCREEN (V2) ===\n");

	// Always recreate to ensure fresh layout and seeding from device state
	if (detail_screen) {
		power_monitor_destroy_detail_screen();
	}
	power_monitor_create_detail_screen();

	if (detail_screen) {
		detail_screen_show(detail_screen);
		printf("[I] power_monitor: Detail screen shown\n");

		// Debug: Log container size after initial content is added
		if (detail_screen->current_view_container) {
			printf("[I] power_monitor: Current view container size after initial content: %dx%d\n",
				lv_obj_get_width(detail_screen->current_view_container),
				lv_obj_get_height(detail_screen->current_view_container));
		}
	} else {
		printf("[E] power_monitor: Detail screen unavailable\n");
	}
}

void power_monitor_destroy_detail_screen(void)
{
	printf("[I] power_monitor: === DESTROY DETAIL SCREEN ===\n");

	// Clear sensor label references
	power_monitor_data_t* data = power_monitor_get_data();
	if (data) {
		memset(&data->sensor_labels, 0, sizeof(power_monitor_sensor_labels_t));
	}

	// Destroy the detail screen UI fully so next show will recreate and seed
	if (detail_screen) {
		detail_screen_destroy(detail_screen);
		detail_screen = NULL;
	}
}

// Touch event handler for detail screen
void power_monitor_handle_detail_touch(void)
{
		printf("[I] power_monitor: === HANDLE DETAIL TOUCH (V2) ===\n");
	power_monitor_cycle_view();
}

// Get current view type
power_monitor_view_type_t power_monitor_get_current_view_type(void)
{
	return get_current_view_type();
}

// Set current view type
void power_monitor_set_current_view_type(power_monitor_view_type_t view_type)
{
	// Find the view type in our available views array
	int view_count = sizeof(available_views) / sizeof(available_views[0]);
	for (int i = 0; i < view_count; i++) {
		if (available_views[i] == view_type) {
			// Note: We can't directly set the index in the shared manager
			// This would require adding a set_index function to the shared manager
			printf("[I] power_monitor: View type %d found at index %d (setting not implemented)\n", view_type, i);
			return;
		}
	}
	printf("[W] power_monitor: View type %d not found in available views, keeping current\n", view_type);
}

// System reset function to clear corrupted state

// Modal functions moved to detail_screen.c

// Handle back button - return to home screen
void power_monitor_handle_back_button(void)
{
	printf("[I] power_monitor: === BACK BUTTON CLICKED ===\n");

	// Clean up modals before destroying detail screen
	extern void detail_screen_reset_modal_tracking(void);
	detail_screen_reset_modal_tracking();

	// Reset all static variables
	s_ui_state.rendering_in_progress = false;

	// Reset view state
	power_monitor_voltage_grid_view_reset_state();
	power_monitor_power_grid_view_reset_state();

	// Handle back button - this will destroy the detail screen and clean up containers
	power_monitor_navigation_hide_detail_screen();

}

// Modal toggle functions using generic detail_screen system
static void power_monitor_toggle_timeline_modal(void)
{
	printf("[I] power_monitor: Toggling timeline modal\n");
	extern void detail_screen_toggle_modal(const char* modal_name,
		void* (*create_func)(void* config, void (*on_close)(void)),
		void (*destroy_func)(void* modal),
		void (*show_func)(void* modal),
		bool (*is_visible_func)(void* modal),
		void* config,
		void (*on_close_callback)(void));

	detail_screen_toggle_modal("timeline",
		(void*(*)(void*, void(*)(void)))timeline_modal_create,
		(void(*)(void*))timeline_modal_destroy,
		(void(*)(void*))timeline_modal_show,
		(bool(*)(void*))timeline_modal_is_visible,
		(void*)&power_monitor_timeline_modal_config,
		NULL
	);
}

static void power_monitor_toggle_alerts_modal(void)
{
	printf("[I] power_monitor: Toggling alerts modal\n");
	extern void detail_screen_toggle_modal(const char* modal_name,
		void* (*create_func)(void* config, void (*on_close)(void)),
		void (*destroy_func)(void* modal),
		void (*show_func)(void* modal),
		bool (*is_visible_func)(void* modal),
		void* config,
		void (*on_close_callback)(void));

	detail_screen_toggle_modal("alerts",
		(void*(*)(void*, void(*)(void)))alerts_modal_create,
		(void(*)(void*))alerts_modal_destroy,
		(void(*)(void*))alerts_modal_show,
		(bool(*)(void*))alerts_modal_is_visible,
		(void*)&battery_alerts_config,
		NULL
	);
}

// Handle alerts button - simple toggle
void power_monitor_handle_alerts_button(void)
{
	printf("[I] power_monitor: === ALERTS BUTTON CLICKED ===\n");
	power_monitor_toggle_alerts_modal();
}

// Modal configuration functions removed - no longer needed

// Handle timeline button - simple toggle
void power_monitor_handle_timeline_button(void)
{
	printf("[I] power_monitor: === TIMELINE BUTTON CLICKED ===\n");
	power_monitor_toggle_timeline_modal();
}

// Simple current view rendering - reuse existing view containers
void power_monitor_render_current_view(lv_obj_t* container)
{
	printf("[I] power_monitor: === RENDER CURRENT VIEW START ===\n");

	// Prevent recursive rendering
	if (s_ui_state.rendering_in_progress) {
		printf("[I] power_monitor: Rendering already in progress, skipping\n");
		return;
	}
	s_ui_state.rendering_in_progress = true;

	printf("[I] power_monitor: Rendering flag set, proceeding with view creation\n");

	printf("[I] power_monitor: === RENDER CURRENT VIEW: About to create view containers ===\n");

	// Make container clickable for touch navigation
	lv_obj_add_flag(container, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);


	// Get current view type from shared manager
	power_monitor_view_type_t current_type = get_current_view_type();
		printf("[I] power_monitor: Showing view type: %d (index: %d)\n", current_type, current_view_manager_get_index());

	// Always render the current view fresh - no complex cleanup logic
	if (current_type == POWER_MONITOR_VIEW_BAR_GRAPH) {

		power_monitor_voltage_grid_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_AMPERAGE_GRID) {

		power_monitor_amperage_grid_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_POWER) {

		power_monitor_power_grid_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_NUMERICAL) {

		power_monitor_starter_voltage_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_HOUSE_VOLTAGE) {

		power_monitor_house_voltage_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_SOLAR_VOLTAGE) {

		power_monitor_solar_voltage_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_STARTER_CURRENT) {

		power_monitor_starter_current_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_HOUSE_CURRENT) {

		power_monitor_house_current_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_SOLAR_CURRENT) {

		power_monitor_solar_current_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_STARTER_POWER) {

		power_monitor_starter_power_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_HOUSE_POWER) {

		power_monitor_house_power_view_render(container);
	} else if (current_type == POWER_MONITOR_VIEW_SOLAR_POWER) {

		power_monitor_solar_power_view_render(container);
	}

	// Timeline settings are applied when gauges are created, not every frame

	// Check if content was actually created
	int child_count = lv_obj_get_child_cnt(container);
	printf("[I] power_monitor: After rendering, container has %d children\n", child_count);

	// Reset rendering flag
	s_ui_state.rendering_in_progress = false;

	// CRITICAL: Ensure view state is consistent after rendering
	printf("[I] power_monitor: === RENDER CURRENT VIEW: Final state check ===\n");
	printf("[I] power_monitor: Current view type: %d, index: %d\n", current_type, current_view_manager_get_index());
	// Global containers removed - no need to check them

	printf("[I] power_monitor: === RENDER CURRENT VIEW COMPLETE ===\n");
}


// Touch callback for home current view - navigates to detail screen
static void power_monitor_home_current_view_touch_cb(lv_event_t * e)
{
	lv_event_code_t code = lv_event_get_code(e);
	printf("[I] power_monitor: *** HOME TOUCH EVENT: code=%d (CLICKED=%d) ***\n", code, LV_EVENT_CLICKED);

	if (code != LV_EVENT_CLICKED) {
		printf("[I] power_monitor: Not a click event, ignoring\n");
		return;
	}

	static int touch_count = 0;
	touch_count++;
	printf("[I] power_monitor: *** HOME TOUCH CALLBACK CALLED #%d ***\n", touch_count);

	// Prevent recursive calls
	if (s_ui_state.reset_in_progress) {
		printf("[W] power_monitor: Home touch callback ignored - navigation in progress\n");
		return;
	}

	printf("[I] power_monitor: Home current view touched - navigating to detail screen\n");

	// Update detail container with current view before navigating
	// Detail screen is handled by the detail screen template

	// Use the proper navigation function to update device state
	extern void screen_navigation_request_detail_view(const char *module_name);
	screen_navigation_request_detail_view("power-monitor");
}

// Touch callback for detail current view - cycles through views
static void power_monitor_detail_current_view_touch_cb(lv_event_t * e)
{
	static int touch_count = 0;
	touch_count++;
	printf("[I] power_monitor: *** DETAIL TOUCH CALLBACK CALLED #%d ***\n", touch_count);

	// Debug: Check event details
	if (e) {
		printf("[I] power_monitor: Event type: %d, target: %p, current target: %p\n",
			lv_event_get_code(e), lv_event_get_target(e), lv_event_get_current_target(e));
	}

	// Check if we're actually on the detail screen
	extern screen_type_t screen_navigation_get_current_screen(void);
	screen_type_t current_screen = screen_navigation_get_current_screen();
		printf("[I] power_monitor: Current screen: %d (detail=%d)\n", current_screen, SCREEN_DETAIL_VIEW);

	if (current_screen != SCREEN_DETAIL_VIEW) {
		printf("[W] power_monitor: Detail touch callback called but not on detail screen, ignoring\n");
		return;
	}

	// Prevent recursive calls
	if (s_ui_state.reset_in_progress) {
		printf("[W] power_monitor: Detail touch callback ignored - navigation in progress\n");
		return;
	}

	printf("[I] power_monitor: Detail current view touched - cycling views\n");
	power_monitor_cycle_current_view();
}

// =============================================================================
// VIEW STATE MANAGEMENT
// =============================================================================

/**
 * @brief Get current view index from device state (with fallback)
 */
static int power_monitor_get_view_index(void)
{
	// Try to get from device state first
	extern int module_screen_view_get_view_index(const char *module_name);
	int index = module_screen_view_get_view_index("power-monitor");

	// Validate and clamp the index
	if (index < 0 || index >= POWER_MONITOR_VIEW_COUNT) {
		printf("[W] power_monitor: Invalid view index %d from device state, using 0\n", index);
		index = 0;
	}

	return index;
}

/**
 * @brief Set current view index in device state
 */
static void power_monitor_set_view_index(int index)
{
	// Validate and clamp the index
	if (index < 0 || index >= POWER_MONITOR_VIEW_COUNT) {
		printf("[W] power_monitor: Invalid view index %d, clamping to valid range\n", index);
		index = (index < 0) ? 0 : (POWER_MONITOR_VIEW_COUNT - 1);
	}

	// Set in device state
	extern void module_screen_view_set_view_index(const char *module_name, int view_index);
	module_screen_view_set_view_index("power-monitor", index);

	// Save device state to persist across resets
	extern void device_state_save(void);
	device_state_save();
}

// =============================================================================
// VIEW LIFECYCLE MANAGEMENT
// =============================================================================

/**
 * @brief Destroy the current view objects properly
 * @param old_view_index The index of the view being destroyed (before the new view is set)
 */
static void power_monitor_destroy_current_view(int old_view_index)
{
	if (s_ui_state.view_destroy_in_progress) {
		printf("[W] power_monitor: View destroy in progress, skipping\n");
		return;
	}
	s_ui_state.view_destroy_in_progress = true;
	int view_index = old_view_index; // Use the old view index that's being destroyed
	printf("[I] power_monitor: Destroying current view objects for index %d\n", view_index);

	// CRITICAL: Cleanup gauge canvas buffers BEFORE destroying LVGL objects
	// This prevents memory leaks from malloc'd canvas buffers
	printf("[I] power_monitor: Cleaning up gauge canvas buffers before LVGL object destruction\n");

	// Call the appropriate reset function based on current view
	switch(view_index) {
		case 0: { // Voltage grid view
			extern void power_monitor_reset_static_gauges(void);
			power_monitor_reset_static_gauges();
			break;
		}
		case 1: { // Amperage grid view
			extern void power_monitor_reset_amperage_static_gauges(void);
			power_monitor_reset_amperage_static_gauges();
			break;
		}
		case 2: { // Power grid view
			extern void power_monitor_power_grid_view_reset_state(void);
			power_monitor_power_grid_view_reset_state();
			break;
		}
		case 3: { // Starter voltage view
			extern void power_monitor_reset_starter_voltage_static_gauge(void);
			power_monitor_reset_starter_voltage_static_gauge();
			break;
		}
		case 4: { // House voltage view
			extern void power_monitor_reset_house_voltage_static_gauge(void);
			power_monitor_reset_house_voltage_static_gauge();
			break;
		}
		case 5: { // Solar voltage view
			extern void power_monitor_reset_solar_voltage_static_gauge(void);
			power_monitor_reset_solar_voltage_static_gauge();
			break;
		}
		case 6: { // Starter current view
			extern void power_monitor_reset_starter_current_static_gauge(void);
			power_monitor_reset_starter_current_static_gauge();
			break;
		}
		case 7: { // House current view
			extern void power_monitor_reset_house_current_static_gauge(void);
			power_monitor_reset_house_current_static_gauge();
			break;
		}
		case 8: { // Solar current view
			extern void power_monitor_reset_solar_current_static_gauge(void);
			power_monitor_reset_solar_current_static_gauge();
			break;
		}
		case 9: { // Starter power view
			extern void power_monitor_reset_starter_power_static_gauge(void);
			power_monitor_reset_starter_power_static_gauge();
			break;
		}
		case 10: { // House power view
			extern void power_monitor_reset_house_power_static_gauge(void);
			power_monitor_reset_house_power_static_gauge();
			break;
		}
		case 11: { // Solar power view
			extern void power_monitor_reset_solar_power_static_gauge(void);
			power_monitor_reset_solar_power_static_gauge();
			break;
		}
		default:
			printf("[W] power_monitor: Unknown view index %d, no reset function\n", view_index);
			break;
	}

	// Get the detail screen container to clean
	extern detail_screen_t* detail_screen;
	if (detail_screen && detail_screen->current_view_container) {
		printf("[I] power_monitor: Cleaning current view container\n");
		lv_obj_clean(detail_screen->current_view_container);

		// Re-apply container styling after clean (clean removes styling)
		lv_obj_set_style_bg_color(detail_screen->current_view_container, lv_color_hex(0x000000), 0);
		lv_obj_set_style_border_width(detail_screen->current_view_container, 1, 0);
		lv_obj_set_style_border_color(detail_screen->current_view_container, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_radius(detail_screen->current_view_container, 4, 0);
		lv_obj_clear_flag(detail_screen->current_view_container, LV_OBJ_FLAG_SCROLLABLE);
	} else {
		printf("[W] power_monitor: No detail screen container to clean\n");
	}

	printf("[I] power_monitor: Current view objects destroyed for index %d\n", view_index);
	s_ui_state.view_destroy_in_progress = false;
}

// =============================================================================
// STANDARDIZED MODULE INTERFACE IMPLEMENTATION
// =============================================================================

/**
 * @brief Standardized module initialization function
 */
static void power_monitor_module_init(void)
{
	printf("[I] power_monitor: Power monitor module initializing via standardized interface\n");
	power_monitor_init();
}

/**
 * @brief Standardized module update function
 *
 * This function is called every main loop tick to update module data.
 * It should NOT create or destroy UI elements, only update data.
 */
static void power_monitor_module_update(void)
{
	static int update_count = 0;
	update_count++;

	extern screen_type_t screen_navigation_get_current_screen(void);
	bool is_detail_screen = screen_navigation_get_current_screen() == SCREEN_DETAIL_VIEW;

	// Skip updates during teardown or view rebuild on any screen
	if( s_ui_state.navigation_teardown_in_progress || s_ui_state.view_destroy_in_progress ){
		return;
	}

	// Update persistent gauge histories (data collection, every frame)
	power_monitor_update_all_gauge_histories();

	// ALWAYS update data - detail gauges need continuous updates
	// Current view gauges (power_grid/starter_voltage) also need updates
	power_monitor_update_data_only();

	// Handle delayed detail view refresh after view cycling
	if( s_ui_state.detail_view_needs_refresh ){

		if( is_detail_screen ){

			extern detail_screen_t* detail_screen;

			if( detail_screen && detail_screen->current_view_container ){

				printf("[I] power_monitor: Performing delayed re-render of detail view after cycle\n");

				// DESTROY: Properly destroy the current view object
				// The view index has already been incremented by current_view_manager_cycle_to_next()
				// So we need to get the previous index
				int current_index = power_monitor_get_view_index();
				// Calculate previous index (decrement with wrap-around)
				int old_view_index = (current_index - 1 + POWER_MONITOR_VIEW_COUNT) % POWER_MONITOR_VIEW_COUNT;
				printf("[I] power_monitor: Switching from view %d to view %d\n", old_view_index, current_index);
				power_monitor_destroy_current_view(old_view_index);

				// Ensure layout is calculated on parent container before creating new view
				// Use detail screen's reusable layout preparation function for consistency
				if (!detail_screen_prepare_current_view_layout(detail_screen)) {
					printf("[E] power_monitor: Failed to prepare current view layout during cycling\n");
					s_ui_state.detail_view_needs_refresh = false;
					return;
				}

				// CREATE: Create new view object with updated index
				// current_index was already calculated above

				// Use the callback system instead of direct call
				if( detail_screen->on_current_view_created ){

					detail_screen->on_current_view_created( detail_screen->current_view_container );
				}

				s_ui_state.detail_view_needs_refresh = false;
			}
		} else {

			// Not on detail screen anymore, clear the flag
			s_ui_state.detail_view_needs_refresh = false;
		}
	}
}

/**
 * @brief Standardized module cleanup function
 */
static void power_monitor_module_cleanup(void)
{
	printf("[I] power_monitor: Power monitor module cleaning up via standardized interface\n");
	power_monitor_cleanup();
}

/**
 * @brief Power monitor module interface definition
 *
 * This structure provides the standardized interface that main.c
 * can use to interact with this module without knowing its internals.
 */
const display_module_t power_monitor_module = {
	.name = "power-monitor",
	.init = power_monitor_module_init,
	.update = power_monitor_module_update,
	.cleanup = power_monitor_module_cleanup
};