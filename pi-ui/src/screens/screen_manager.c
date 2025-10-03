#include "screen_manager.h"
#include "home_screen/home_screen.h"
#include "../displayModules/power-monitor/power-monitor.h"
#include "../lvgl_port_pi.h"
#include "esp_compat.h"
#include <string.h>

static const char *TAG = "screen_manager";

// Current screen state
static screen_type_t current_screen_type = SCREEN_NONE;
static char current_module_name[32] = {0};

// Transition protection
static uint32_t last_transition_time = 0;
#define MIN_TRANSITION_INTERVAL_MS 50  // Minimum 50ms between transitions

// Screen definitions - all screens that can be created
static const screen_definition_t screen_definitions[] = {
	{
		.screen_type = SCREEN_HOME,
		.module_name = NULL,
		.create_func = home_screen_show,
		.destroy_func = home_screen_destroy
	},
	{
		.screen_type = SCREEN_DETAIL_VIEW,
		.module_name = "power-monitor",
		.create_func = power_monitor_show_detail_screen,
		.destroy_func = power_monitor_destroy_detail_screen
	}
	// Add more screens here as needed
};

#define SCREEN_DEFINITIONS_COUNT (sizeof(screen_definitions) / sizeof(screen_definitions[0]))

/**
 * @brief Find screen definition by type and module name
 */
static const screen_definition_t* find_screen_definition(screen_type_t screen_type, const char *module_name)
{
	for (size_t i = 0; i < SCREEN_DEFINITIONS_COUNT; i++) {
		const screen_definition_t *def = &screen_definitions[i];

		if (def->screen_type != screen_type) {
			continue;
		}

		// Check module name match (treat empty string as NULL)
		bool module_is_empty = (module_name == NULL || strlen(module_name) == 0);
		bool def_module_is_empty = (def->module_name == NULL || strlen(def->module_name) == 0);

		if (module_is_empty && def_module_is_empty) {
			return def; // Both empty/NULL - match
		}

		if (!module_is_empty && !def_module_is_empty &&
			strcmp(module_name, def->module_name) == 0) {
			return def; // Both non-empty and equal - match
		}
	}

	return NULL;
}

/**
 * @brief Destroy the current screen completely
 */
static void destroy_current_screen(void)
{
	if (current_screen_type == SCREEN_NONE) {
		ESP_LOGD(TAG, "No current screen to destroy");
		return;
	}

	ESP_LOGI(TAG, "Destroying current screen: type=%d, module=%s",
			 current_screen_type, current_module_name[0] ? current_module_name : "none");

	// Find the current screen definition
	const screen_definition_t *def = find_screen_definition(current_screen_type,
														   current_module_name[0] ? current_module_name : NULL);

	if (def && def->destroy_func) {
		ESP_LOGI(TAG, "Calling destroy function for screen type %d", current_screen_type);
		def->destroy_func();
		ESP_LOGI(TAG, "Screen destroyed successfully");
	} else {
		ESP_LOGW(TAG, "No destroy function found for screen type %d", current_screen_type);
	}

	// Clear current screen state
	current_screen_type = SCREEN_NONE;
	memset(current_module_name, 0, sizeof(current_module_name));
}

/**
 * @brief Create and show a new screen
 */
static void create_and_show_screen(screen_type_t screen_type, const char *module_name)
{
	ESP_LOGI(TAG, "Creating new screen: type=%d, module=%s",
			 screen_type, module_name ? module_name : "none");

	// Find the screen definition
	const screen_definition_t *def = find_screen_definition(screen_type, module_name);

	if (!def) {
		ESP_LOGE(TAG, "No screen definition found for type %d, module %s",
				 screen_type, module_name ? module_name : "none");
		return;
	}

	if (!def->create_func) {
		ESP_LOGE(TAG, "No create function found for screen type %d", screen_type);
		return;
	}

	// Update current screen state BEFORE creating (in case create function checks it)
	current_screen_type = screen_type;
	if (module_name) {
		strncpy(current_module_name, module_name, sizeof(current_module_name) - 1);
		current_module_name[sizeof(current_module_name) - 1] = '\0';
	} else {
		memset(current_module_name, 0, sizeof(current_module_name));
	}

	// Create the new screen
	ESP_LOGI(TAG, "Calling create function for screen type %d", screen_type);
	def->create_func();
	ESP_LOGI(TAG, "Screen created successfully");

	// Update global device state to match
	screen_navigation_set_current_screen(screen_type, module_name);
}

void screen_manager_init(void)
{
	ESP_LOGI(TAG, "Initializing screen manager");

	// Initialize state
	current_screen_type = SCREEN_NONE;
	memset(current_module_name, 0, sizeof(current_module_name));
	last_transition_time = 0;

	// Get initial screen from device state
	screen_type_t initial_screen = screen_navigation_get_current_screen();
	const char *initial_module = screen_navigation_get_current_module();

	ESP_LOGI(TAG, "Initial screen from state: type=%d, module=%s",
			 initial_screen, initial_module ? initial_module : "none");

	// Show the initial screen
	create_and_show_screen(initial_screen, initial_module);

	ESP_LOGI(TAG, "Screen manager initialized");
}

void screen_manager_update(void)
{
	// Check if there's a pending screen transition
	if (!screen_navigation_is_transition_pending()) {
		return; // No transition needed
	}

	// Check transition rate limiting
		uint32_t current_time = xTaskGetTickCount() * 1;
		if (current_time - last_transition_time < MIN_TRANSITION_INTERVAL_MS) {
		ESP_LOGD(TAG, "Transition rate limited, skipping");
			return;
		}

	ESP_LOGI(TAG, "Processing screen transition");

	// Get the requested screen
		screen_type_t requested_screen = screen_navigation_get_requested_screen();
	const char *requested_module = screen_navigation_get_requested_module();

	ESP_LOGI(TAG, "Requested transition: type=%d, module=%s",
			 requested_screen, requested_module ? requested_module : "none");

	// Store module name locally before processing transitions (which may clear it)
		char local_module_name[32] = {0};
		if (requested_module) {
			strncpy(local_module_name, requested_module, sizeof(local_module_name) - 1);
			local_module_name[sizeof(local_module_name) - 1] = '\0';
		}

	// Process the transition in device state (this clears the pending flag)
		screen_navigation_process_transitions();

	// Perform the actual screen transition
	screen_manager_show_screen(requested_screen,
							  local_module_name[0] ? local_module_name : NULL);

		// Update transition time
	last_transition_time = current_time;

	ESP_LOGI(TAG, "Screen transition completed");
}

void screen_manager_show_screen(screen_type_t screen_type, const char *module_name)
{
	ESP_LOGI(TAG, "=== SCREEN TRANSITION START ===");
	ESP_LOGI(TAG, "From: type=%d, module=%s", current_screen_type,
			 current_module_name[0] ? current_module_name : "none");
	ESP_LOGI(TAG, "To: type=%d, module=%s", screen_type, module_name ? module_name : "none");

	// Check if this is actually a different screen
	bool same_screen = (current_screen_type == screen_type);
	bool same_module = false;

	if (module_name == NULL && current_module_name[0] == '\0') {
		same_module = true; // Both NULL/empty
	} else if (module_name != NULL && current_module_name[0] != '\0' &&
			   strcmp(module_name, current_module_name) == 0) {
		same_module = true; // Both non-NULL and equal
	}

	if (same_screen && same_module) {
		ESP_LOGI(TAG, "Same screen requested, no transition needed");
		return;
	}

	// Lock LVGL for the entire transition
	if (!lvgl_port_lock(100)) { // 100ms timeout
		ESP_LOGE(TAG, "Failed to lock LVGL for screen transition");
		return;
	}

	// Step 1: Destroy current screen
	destroy_current_screen();

	// Step 2: Create new screen
	create_and_show_screen(screen_type, module_name);

	// Unlock LVGL
	lvgl_port_unlock();

	ESP_LOGI(TAG, "=== SCREEN TRANSITION COMPLETE ===");
}

void screen_manager_cleanup(void)
{
	ESP_LOGI(TAG, "Cleaning up screen manager");

	// Destroy current screen
	destroy_current_screen();

	ESP_LOGI(TAG, "Screen manager cleanup complete");
}

screen_type_t screen_manager_get_current_screen(void)
{
	return current_screen_type;
}

const char* screen_manager_get_current_module(void)
{
	return current_module_name[0] ? current_module_name : NULL;
}
