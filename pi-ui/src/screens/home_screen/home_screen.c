#include <stdio.h>
#include "home_screen.h"
#include "../../lvgl_port_pi.h"
#include "../../displayModules/power-monitor/power-monitor.h"
#include "../../displayModules/shared/display_module_base.h"
#include "../../fonts/lv_font_noplato_10.h"
#include "../../fonts/lv_font_noplato_18.h"
#include <lvgl.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *TAG = "home_screen";

// Track if home screen has been initialized
static bool s_home_screen_initialized = false;

// Uptime tracking
static time_t s_device_start_time = 0;
static lv_timer_t *s_uptime_timer = NULL;

// Forward declarations for module render functions
extern void power_monitor_render_current_view(lv_obj_t *container);
extern void power_monitor_show_in_container_home(lv_obj_t *container);


// Simple display module interface (local definition)
typedef struct {
	void (*renderCurrentView)(lv_obj_t *container);
} display_module_interface_t;

// Simple display module structure (local definition)
typedef struct {
	lv_obj_t *container;
	char module_name[32];
	display_module_interface_t interface;
	bool rendered_once;  // Track if module has been rendered to avoid repeated calls
} display_module_t;

// Module registry structure
typedef struct {
	const char *module_name;
	display_module_interface_t interface;
} module_registry_entry_t;

// Placeholder module render functions
static void cooling_management_render(lv_obj_t *container) {
	lv_obj_t *label = lv_label_create(container);
	lv_label_set_text(label, "COOLING\nMANAGEMENT");
	lv_obj_center(label);
	lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
}

static void environmental_render(lv_obj_t *container) {
	lv_obj_t *label = lv_label_create(container);
	lv_label_set_text(label, "ENVIRONMENTAL\nCONDITIONS");
	lv_obj_center(label);
	lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
}

static void tpms_render(lv_obj_t *container) {
	lv_obj_t *label = lv_label_create(container);
	lv_label_set_text(label, "TPMS\nSYSTEM");
	lv_obj_center(label);
	lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
}

static void inclinometer_render(lv_obj_t *container) {
	lv_obj_t *label = lv_label_create(container);
	lv_label_set_text(label, "INCLINOMETER");
	lv_obj_center(label);
	lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
}

static void gps_render(lv_obj_t *container) {
	lv_obj_t *label = lv_label_create(container);
	lv_label_set_text(label, "GPS");
	lv_obj_center(label);
	lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
}

static void engine_management_render(lv_obj_t *container) {
	lv_obj_t *label = lv_label_create(container);
	lv_label_set_text(label, "ENGINE\nMANAGEMENT");
	lv_obj_center(label);
	lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
}

// Module registry - maps module names to their interfaces
static const module_registry_entry_t module_registry[] = {
	{"cooling-management", {.renderCurrentView = cooling_management_render}},
	{"environmental", {.renderCurrentView = environmental_render}},
	{"tpms", {.renderCurrentView = tpms_render}},
	{"inclinometer", {.renderCurrentView = inclinometer_render}},
	{"power-monitor", {.renderCurrentView = power_monitor_show_in_container_home}},
	{"gps", {.renderCurrentView = gps_render}},
	{"engine-management", {.renderCurrentView = engine_management_render}}
};



lv_obj_t *home_container = NULL;
static lv_obj_t *content_container = NULL;
static lv_obj_t *context_panel = NULL;
static lv_obj_t *connection_status_label = NULL;
static lv_obj_t *signal_type_label = NULL;
static lv_obj_t *telemetry_label = NULL;
static lv_obj_t *uptime_time_label = NULL;

// Display modules
#define MAX_MODULES 8
static display_module_t display_modules[MAX_MODULES];
static int module_count = 0;

// Uptime timer callback
static void uptime_timer_cb(lv_timer_t *timer)
{
	if (!uptime_time_label) return; // Update only the time numbers

	time_t current_time = time(NULL); // Get actual system time
	time_t uptime_seconds = current_time - s_device_start_time;

	// Convert to hours:minutes:seconds
	uint32_t hours = (uint32_t)(uptime_seconds / 3600);
	uint32_t minutes = (uint32_t)((uptime_seconds % 3600) / 60);
	uint32_t seconds = (uint32_t)(uptime_seconds % 60);

	char time_str[16];
	snprintf(time_str, sizeof(time_str), "%02lu:%02lu:%02lu",
			 hours, minutes, seconds);

	lv_label_set_text(uptime_time_label, time_str);
}

// Simple module management functions (local implementations)
static void display_module_init(display_module_t *module, lv_obj_t *parent, int x, int y, int width, int height)
{
	if (!module || !parent) return;

	// Create container for this module
	module->container = lv_obj_create(parent);

	// Check parent size
	int parent_width = lv_obj_get_width(parent);
	int parent_height = lv_obj_get_height(parent);

	lv_obj_set_size(module->container, width, height);
	lv_obj_set_pos(module->container, x, y);

	// Force the container to have the correct size
	lv_obj_update_layout(module->container);

	// Debug: Log the actual size after setting
	lv_area_t area;
	lv_obj_get_coords(module->container, &area);
		printf("[W] home_screen: Home module created: requested=(%d,%d %dx%d) actual=(%d,%d %dx%d)\n",
		x, y, width, height, area.x1, area.y1, lv_area_get_width(&area), lv_area_get_height(&area));
	// Clear any default padding that might be causing the 17px offset
	lv_obj_set_style_pad_all(module->container, 0, 0);
	lv_obj_set_style_pad_left(module->container, 0, 0);
	lv_obj_set_style_pad_right(module->container, 0, 0);
	lv_obj_set_style_pad_top(module->container, 0, 0);
	lv_obj_set_style_pad_bottom(module->container, 0, 0);

	lv_obj_set_style_bg_opa(module->container, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(module->container, 1, 0);
	lv_obj_set_style_border_color(module->container, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_radius(module->container, 4, 0);
	lv_obj_clear_flag(module->container, LV_OBJ_FLAG_SCROLLABLE);

	// Initialize rendered flag
	module->rendered_once = false;

	// No direct touch callbacks - current view template handles all touch events
}



static void display_module_set_touch_callback(display_module_t *module, const char *module_name, void *callback)
{
	if (!module || !module_name) return;

	// Store module name
	strncpy(module->module_name, module_name, sizeof(module->module_name) - 1);
	module->module_name[sizeof(module->module_name) - 1] = '\0';
}

static void display_module_set_interface(display_module_t *module, display_module_interface_t interface)
{
	if (!module) return;
	module->interface = interface;
}

static void display_module_cleanup(display_module_t *module)
{
	if (!module || !module->container) return;
	lv_obj_del(module->container);
	module->container = NULL;
	memset(module->module_name, 0, sizeof(module->module_name));
	module->rendered_once = false;  // Reset rendered flag for fresh rendering
}

void home_screen_init(void)
{
	// Get display dimensions from LVGL port (NO individual screen configs!)
	uint32_t screen_width, screen_height;
	lvgl_port_get_display_size(&screen_width, &screen_height);

	// Set screen background to black
	lv_obj_t *scr = lv_scr_act();
	lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);

	// Force screen to use correct dimensions
	lvgl_port_force_screen_dimensions(scr);

	// Mark as initialized
	s_home_screen_initialized = true;

	// Create main container - no padding, fill entire screen
	home_container = lv_obj_create(scr);
	lv_obj_set_size(home_container, screen_width, screen_height); // Use port dimensions
	lv_obj_set_style_pad_all(home_container, 0, 0); // No padding
	lv_obj_set_style_bg_color(home_container, lv_color_hex(0x000000), 0); // Black background
	lv_obj_set_style_border_width(home_container, 0, 0);
	lv_obj_set_style_radius(home_container, 0, 0); // No border radius
	lv_obj_clear_flag(home_container, LV_OBJ_FLAG_SCROLLABLE); // Disable scrolling

	// Touch events are handled by individual modules

	// Create inner content container - no padding
	content_container = lv_obj_create(home_container);
	lv_obj_set_size(content_container, screen_width, screen_height - 40); // Use port dimensions - 40px context panel
	lv_obj_set_style_pad_all(content_container, 0, 0); // No padding
	lv_obj_set_style_bg_color(content_container, lv_color_hex(0x000000), 0); // Black background
	lv_obj_set_style_border_width(content_container, 0, 0);
	lv_obj_set_style_radius(content_container, 0, 0); // No radius on inner container
	lv_obj_clear_flag(content_container, LV_OBJ_FLAG_SCROLLABLE); // Disable scrolling

	// Create context panel - header with no background
	context_panel = lv_obj_create(content_container);
	lv_obj_set_size(context_panel, LV_PCT(100), 40);
	lv_obj_align(context_panel, LV_ALIGN_TOP_MID, 0, 0);
	lv_obj_set_style_pad_all(context_panel, 15, 0);
	lv_obj_set_style_bg_opa(context_panel, LV_OPA_TRANSP, 0); // No background
	lv_obj_set_style_border_width(context_panel, 0, 0); // No border
	lv_obj_clear_flag(context_panel, LV_OBJ_FLAG_SCROLLABLE); // Disable scrolling

	// Create ECU status text
	connection_status_label = lv_label_create(context_panel);
	lv_label_set_text(connection_status_label, "ECU: ");
	lv_obj_set_style_text_font(connection_status_label, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(connection_status_label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_align(connection_status_label, LV_ALIGN_LEFT_MID, 0, 0);

	// Create ECU status indicator
	lv_obj_t *ecu_status_indicator = lv_label_create(context_panel);
	lv_label_set_text(ecu_status_indicator, "ONLINE");
	lv_obj_set_style_text_font(ecu_status_indicator, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(ecu_status_indicator, lv_color_hex(0x00FF00), 0); // Green for online
	lv_obj_align_to(ecu_status_indicator, connection_status_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	// Create WEBSERVER status text
	signal_type_label = lv_label_create(context_panel);
	lv_label_set_text(signal_type_label, "  WEBSERVER: ");
	lv_obj_set_style_text_font(signal_type_label, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(signal_type_label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_align_to(signal_type_label, ecu_status_indicator, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	// Create WEBSERVER status indicator
	lv_obj_t *webserver_status_indicator = lv_label_create(context_panel);
	lv_label_set_text(webserver_status_indicator, "ONLINE");
	lv_obj_set_style_text_font(webserver_status_indicator, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(webserver_status_indicator, lv_color_hex(0x00FF00), 0); // Green for online
	lv_obj_align_to(webserver_status_indicator, signal_type_label, LV_ALIGN_OUT_RIGHT_MID, 0, 0);

	// Create device uptime text label
	telemetry_label = lv_label_create(context_panel);
	lv_label_set_text(telemetry_label, "DEVICE UPTIME: ");
	lv_obj_set_style_text_font(telemetry_label, &lv_font_montserrat_12, 0); // Regular font for text
	lv_obj_set_style_text_color(telemetry_label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_align(telemetry_label, LV_ALIGN_RIGHT_MID, -70, 0);

	// Create uptime time label (numbers only) - positioned to the right of the text
	uptime_time_label = lv_label_create(context_panel);
	lv_label_set_text(uptime_time_label, "00:00:00");
	lv_obj_set_style_text_font(uptime_time_label, &lv_font_noplato_18, 0); // Monospace font for numbers
	lv_obj_set_style_text_color(uptime_time_label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_align(uptime_time_label, LV_TEXT_ALIGN_RIGHT, 0); // Right justify numbers
	lv_obj_align_to(uptime_time_label, telemetry_label, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

	// Initialize device start time and create uptime timer
	s_device_start_time = time(NULL); // Start time in actual system time
	s_uptime_timer = lv_timer_create(uptime_timer_cb, 1000, NULL); // Update every second (1000ms)
	lv_timer_set_repeat_count(s_uptime_timer, -1); // Repeat indefinitely

	module_count = 0;
	// Use dimensions from LVGL port (already retrieved above)
	int context_panel_height = 40;


	// Calculate module dimensions for 2x3 grid layout with margins
	// Outer margins: 4px on left/right/top, 8px on bottom for state module
	// Inner margins: 6px between modules
	int outer_margin = 4;
	int inner_margin = 6;
	int state_module_height = 80; // Increased height for state/messages module at bottom
	int state_module_margin = 8; // Additional margin around state module
	int available_width = screen_width - (2 * outer_margin) - inner_margin; // 2 outer + 1 inner
	int available_height = (screen_height - context_panel_height) - outer_margin - state_module_height - state_module_margin - (2 * inner_margin); // top outer + state module + margin + 2 inner

	int module_width = available_width / 2; // 2 columns
	int module_height = available_height / 3.2;

	// Position modules with margins
	int start_x = outer_margin; // Start with left margin
	int start_y = context_panel_height; // Start right after context panel (no top margin since context panel is at top)

	// Force layout update to ensure containers have proper dimensions
	lv_obj_update_layout(home_container);
	lv_obj_update_layout(content_container);

	// Verify container dimensions after layout update
	int actual_home_width = lv_obj_get_width(home_container);
	int actual_home_height = lv_obj_get_height(home_container);
	int actual_content_width = lv_obj_get_width(content_container);
	int actual_content_height = lv_obj_get_height(content_container);

	// Create modules from registry - each module has a .render_current_view() function
	int modules_to_show = sizeof(module_registry) / sizeof(module_registry[0]);

	for (int i = 0; i < modules_to_show; i++) {
		const module_registry_entry_t *entry = &module_registry[i];

		// Regular 2x3 grid layout: 2 columns, 3 rows
		int col = i % 2; // Column (0 or 1)
		int row = i / 2; // Row (0, 1, or 2)
		int x = start_x + col * (module_width + inner_margin); // Position based on column with inner margin
		int y = start_y + row * (module_height + inner_margin); // Position based on row with inner margin
		int width = module_width;
		int height = module_height;

		display_module_init(&display_modules[module_count], content_container, x, y, width, height);

		// Enable touch on this module
		display_module_set_touch_callback(&display_modules[module_count], entry->module_name, NULL);

		// Set the interface for this module - allows calling module.renderCurrentView()
		display_module_set_interface(&display_modules[module_count], entry->interface);

		module_count++;
	}

	// Create state/messages module at the bottom - fill ALL remaining space
	int state_x = outer_margin;
	int state_y = context_panel_height + (3 * module_height) + (2 * inner_margin); // Position after all modules (no top margin)
	int state_width = screen_width - (2 * outer_margin);
	int state_height = screen_height - state_y; // Fill ALL remaining space to bottom of screen

	// Create state module container as direct child of home_container
	lv_obj_t *state_container = lv_obj_create(home_container);
	lv_obj_set_size(state_container, state_width, state_height);
	lv_obj_set_pos(state_container, state_x, state_y);
	lv_obj_set_style_pad_all(state_container, 8, 0);
	lv_obj_set_style_bg_opa(state_container, LV_OPA_TRANSP, 0); // No background
	lv_obj_set_style_border_width(state_container, 1, 0);
	lv_obj_set_style_border_color(state_container, lv_color_hex(0xFFFFFF), 0); // White border
	lv_obj_set_style_radius(state_container, 4, 0); // Same radius as display modules
	lv_obj_clear_flag(state_container, LV_OBJ_FLAG_SCROLLABLE);

	// Create state title
	lv_obj_t *state_title = lv_label_create(state_container);
	lv_label_set_text(state_title, "SYSTEM STATUS");
	lv_obj_set_style_text_font(state_title, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(state_title, lv_color_hex(0xFFFFFF), 0);
	lv_obj_align(state_title, LV_ALIGN_TOP_LEFT, 0, 0);

	// Create state message
	lv_obj_t *state_message = lv_label_create(state_container);
	lv_label_set_text(state_message, "All systems operational");
	lv_obj_set_style_text_font(state_message, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(state_message, lv_color_hex(0x00FF00), 0);
	lv_obj_align(state_message, LV_ALIGN_TOP_LEFT, 0, 20);

	// Create message count
	lv_obj_t *message_count = lv_label_create(state_container);
	lv_label_set_text(message_count, "Messages: 0");
	lv_obj_set_style_text_font(message_count, &lv_font_montserrat_12, 0);
	lv_obj_set_style_text_color(message_count, lv_color_hex(0x888888), 0);
	lv_obj_align(message_count, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

	// Render all modules after they are created
	home_screen_update_modules();
}

void home_screen_update_status(const char *status)
{
	// This function is now handled by the context panel
	// Connection status, signal type, and telemetry are updated separately
}

void home_screen_update_context_panel(const char *connection_status, const char *signal_type, const char *telemetry)
{
	// Check if home screen is actually visible before updating context panel
	if (home_container && lv_obj_has_flag(home_container, LV_OBJ_FLAG_HIDDEN)) {

		return;
	}

	// Static variables removed - no longer needed for this implementation

	// NO RATE LIMITING - Always update context panel
	// Note: LVGL mutex lock removed to prevent deadlocks - simple label updates don't need it

	if (connection_status_label && connection_status) {
		char status_text[64];
		snprintf(status_text, sizeof(status_text), "WiFi: %s", connection_status);
		lv_label_set_text(connection_status_label, status_text);

		// Only update color if connection status changed
		// Temporarily disabled to test if color updates cause dirty areas
		// bool is_connected = (strstr(connection_status, "Connected") || strstr(connection_status, "connected"));
		// if (is_connected != last_connection_connected) {
		// 	lv_obj_set_style_text_color(connection_status_label, is_connected ? lv_color_hex(0x00FF00) : lv_color_hex(0xFF0000), 0);
		// 	last_connection_connected = is_connected;
		// }
	}

	if (signal_type_label && signal_type) {
		char signal_text[64];
		snprintf(signal_text, sizeof(signal_text), "Signal: %s", signal_type);
		lv_label_set_text(signal_type_label, signal_text);

		// Only update color if signal strength changed
		// Temporarily disabled to test if color updates cause dirty areas
		// bool is_strong = (strstr(signal_type, "Strong") || strstr(signal_type, "strong"));
		// bool is_weak = (strstr(signal_type, "Weak") || strstr(signal_type, "weak"));

		// if (is_strong != last_signal_strong || is_weak != last_signal_weak) {
		// 	if (is_strong) {
		// 		lv_obj_set_style_text_color(signal_type_label, lv_color_hex(0x00FF00), 0);
		// 	} else if (is_weak) {
		// 		lv_obj_set_style_text_color(signal_type_label, lv_color_hex(0xFFAA00), 0);
		// 	} else {
		// 		lv_obj_set_style_text_color(signal_type_label, lv_color_hex(0x00AAFF), 0);
		// 	}
		// 	last_signal_strong = is_strong;
		// 	last_signal_weak = is_weak;
		// }
	}

	if (telemetry_label && telemetry) {
		lv_label_set_text(telemetry_label, telemetry);
	}

	// LVGL mutex unlock removed - no longer needed
}

void home_screen_update_modules(void)
{
	// Check if home screen is actually visible before updating
	if (home_container && lv_obj_has_flag(home_container, LV_OBJ_FLAG_HIDDEN)) {
		return;
	}

	// Safety check - make sure modules are initialized
	if (module_count == 0) {
		return;
	}

	// Create module UIs on first render
	for (int i = 0; i < module_count; i++) {
		// Safety check - make sure the module is valid
		if (i >= module_count || i < 0) {
			break;
		}

		// Get the module's container
		lv_obj_t *module_container = display_modules[i].container;
		if (module_container) {
			// Only create the module UI once
			if (!display_modules[i].rendered_once) {
				if (strcmp(display_modules[i].module_name, "power-monitor") == 0) {
					// Use display_module_base lifecycle for power-monitor
					extern void power_monitor_create(void);
					extern display_module_base_t* power_monitor_get_module_base(void);

					// Ensure module is created (idempotent)
					power_monitor_create();

					// Create UI in container
					display_module_base_t* base = power_monitor_get_module_base();
					if (base) {
						display_module_base_create(base, module_container);
					}
				} else if (display_modules[i].interface.renderCurrentView) {
					// Legacy modules - use old interface
					display_modules[i].interface.renderCurrentView(module_container);
				}
				display_modules[i].rendered_once = true;
			}
		}
	}
}





void home_screen_cleanup(void)
{
	// Cleanup uptime timer
	if (s_uptime_timer) {
		lv_timer_delete(s_uptime_timer);
		s_uptime_timer = NULL;
	}

	// Cleanup display modules
	for (int i = 0; i < module_count; i++) {
		display_module_cleanup(&display_modules[i]);
	}
	module_count = 0;

	if (home_container) {
		lv_obj_del(home_container);
		home_container = NULL;
		content_container = NULL;
		context_panel = NULL;
		connection_status_label = NULL;
		signal_type_label = NULL;
		telemetry_label = NULL;
	}

	// Reset initialization flag
	s_home_screen_initialized = false;
}

// Screen manager wrapper functions
void home_screen_show(void)
{

	// Always create fresh home screen - rely on device state machine for configuration
	home_screen_cleanup();  // Complete cleanup first

	// Only initialize if not already initialized
	if (!s_home_screen_initialized) {
		home_screen_init();     // Fresh initialization
	}
}

void home_screen_destroy(void)
{
	// Complete destruction - remove everything
	home_screen_cleanup();
}

void home_screen_update_module_data(void)
{
	// Check if home screen is actually visible before updating data
	if (home_container && lv_obj_has_flag(home_container, LV_OBJ_FLAG_HIDDEN)) {

		return; // Skip data updates if home screen is hidden
	}
}


// Function to get power monitor container for safe UI updates
lv_obj_t* get_power_monitor_container(void)
{
	// Find the power monitor module container
	for (int i = 0; i < module_count; i++) {
		if (strcmp(display_modules[i].module_name, "power-monitor") == 0) {
			return display_modules[i].container;
		}
	}
	return NULL;
}
