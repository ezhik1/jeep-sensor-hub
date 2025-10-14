#include <stdio.h>
#include "current_view_manager.h"
#include "../../../state/device_state.h"

#include <string.h>

static const char *TAG = "current_view_manager";

void current_view_manager_init(int available_views_count)
{
		printf("[I] current_view_manager: Initializing shared current view manager with %d available views\n", available_views_count);

	if (available_views_count <= 0) {
		printf("[E] current_view_manager: Invalid available views count: %d\n", available_views_count);
		return;
	}

	// Initialize the view lifecycle (separated from state management)
	current_view_initialize(available_views_count);

		printf("[I] current_view_manager: Shared current view manager initialized with %d views\n", available_views_count);
}

int current_view_manager_get_index(void)
{
	// Use module-specific state for power-monitor
	return module_screen_view_get_view_index("power-monitor");
}

void current_view_manager_cycle_to_next(void)
{
	printf("[I] current_view_manager: === REQUESTING VIEW CYCLE ===\n");

	// Use module-specific state management for cycling
	// For now, assume power-monitor module (this should be made configurable)
	module_screen_view_cycle_to_next("power-monitor");

		printf("[I] current_view_manager: View cycle requested, current index: %d\n", module_screen_view_get_view_index("power-monitor"));
}

bool current_view_manager_is_cycling_in_progress(void)
{
	// Use module-specific state for power-monitor
	return module_screen_view_is_cycling_in_progress("power-monitor");
}

void current_view_manager_set_cycling_in_progress(bool in_progress)
{
	// Use module-specific state for power-monitor
	module_screen_view_set_cycling_in_progress("power-monitor", in_progress);
}

int current_view_manager_get_count(void)
{
	// Use module-specific state for power-monitor
	return module_screen_view_get_views_count("power-monitor");
}

bool current_view_manager_is_visible(void)
{
	// Use module-specific state for power-monitor
	return module_screen_view_is_visible("power-monitor");
}

void current_view_manager_set_visible(bool visible)
{
	// Use module-specific state for power-monitor
	module_screen_view_set_visible("power-monitor", visible);
}

void current_view_manager_cleanup(void)
{
	printf("[I] current_view_manager: Cleaning up shared current view manager\n");

	// Clean up view lifecycle
	current_view_cleanup();

	// Reset state management
	view_state_set_cycling_in_progress(false);

	printf("[I] current_view_manager: Shared current view manager cleanup complete\n");
}

// Working implementations for current view management
static int s_available_views_count = 0;
static bool s_cycling_in_progress = false;

void current_view_initialize(int available_views_count) {
	s_available_views_count = available_views_count;
	printf("[I] current_view_manager: Initialized with %d available views\n", available_views_count);
}

void current_view_cleanup(void) {
	s_available_views_count = 0;
	s_cycling_in_progress = false;
	printf("[I] current_view_manager: Cleaned up\n");
}

int module_screen_view_get_view_index(const char* module_name) {
	char path[128];
	snprintf(path, sizeof(path), "modules.%s.current_view_index", module_name ? module_name : "unknown");
	int view_index = device_state_get_int(path);
	// Removed excessive logging - this function is called too frequently
	return view_index;
}

void module_screen_view_set_view_index(const char* module_name, int view_index) {
	char path[128];
	snprintf(path, sizeof(path), "modules.%s.current_view_index", module_name ? module_name : "unknown");
	device_state_set_int(path, view_index);
	printf("[I] current_view_manager: Set module %s view index to %d\n", module_name ? module_name : "unknown", view_index);
}

void module_screen_view_cycle_to_next(const char* module_name) {
	if (s_cycling_in_progress) {
		printf("[I] current_view_manager: Cycling already in progress for module %s\n", module_name ? module_name : "unknown");
		return;
	}

	int current_index = module_screen_view_get_view_index(module_name);
	int next_index = (current_index + 1) % s_available_views_count;

	printf("[I] current_view_manager: Cycling module %s from view %d to %d\n",
		   module_name ? module_name : "unknown", current_index, next_index);

	module_screen_view_set_view_index(module_name, next_index);
}

bool module_screen_view_is_cycling_in_progress(const char* module_name) {
	return s_cycling_in_progress;
}

void module_screen_view_set_cycling_in_progress(const char* module_name, bool in_progress) {
	s_cycling_in_progress = in_progress;
	printf("[I] current_view_manager: Set cycling in progress for module %s: %s\n",
		   module_name ? module_name : "unknown", in_progress ? "true" : "false");
}

int module_screen_view_get_views_count(const char* module_name) {
	printf("[I] current_view_manager: Module %s has %d views\n", module_name ? module_name : "unknown", s_available_views_count);
	return s_available_views_count;
}

bool module_screen_view_is_visible(const char* module_name) {
	char path[128];
	snprintf(path, sizeof(path), "modules.%s.visible", module_name ? module_name : "unknown");
	bool visible = device_state_get_bool(path);
	printf("[I] current_view_manager: Module %s visibility: %s\n", module_name ? module_name : "unknown", visible ? "true" : "false");
	return visible;
}

void module_screen_view_set_visible(const char* module_name, bool visible) {
	char path[128];
	snprintf(path, sizeof(path), "modules.%s.visible", module_name ? module_name : "unknown");
	device_state_set_bool(path, visible);
	printf("[I] current_view_manager: Set module %s visibility to %s\n", module_name ? module_name : "unknown", visible ? "true" : "false");
}

void view_state_set_cycling_in_progress(bool in_progress) {
	s_cycling_in_progress = in_progress;
	printf("[I] current_view_manager: Set global cycling in progress: %s\n", in_progress ? "true" : "false");
}
