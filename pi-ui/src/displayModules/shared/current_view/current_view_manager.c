#include "current_view_manager.h"
#include "../../../state/device_state.h"
#include "../../../esp_compat.h"
#include <string.h>

static const char *TAG = "current_view_manager";

void current_view_manager_init(int available_views_count)
{
	ESP_LOGI(TAG, "Initializing shared current view manager with %d available views", available_views_count);

	if (available_views_count <= 0) {
		ESP_LOGE(TAG, "Invalid available views count: %d", available_views_count);
		return;
	}

	// Initialize the view lifecycle (separated from state management)
	current_view_initialize(available_views_count);

	ESP_LOGI(TAG, "Shared current view manager initialized with %d views", available_views_count);
}

int current_view_manager_get_index(void)
{
	// Use module-specific state for power-monitor
	return module_screen_view_get_view_index("power-monitor");
}

void current_view_manager_cycle_to_next(void)
{
	ESP_LOGI(TAG, "=== REQUESTING VIEW CYCLE ===");

	// Use module-specific state management for cycling
	// For now, assume power-monitor module (this should be made configurable)
	module_screen_view_cycle_to_next("power-monitor");

	ESP_LOGI(TAG, "View cycle requested, current index: %d", module_screen_view_get_view_index("power-monitor"));
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
	ESP_LOGI(TAG, "Cleaning up shared current view manager");

	// Clean up view lifecycle
	current_view_cleanup();

	// Reset state management
	view_state_set_cycling_in_progress(false);

	ESP_LOGI(TAG, "Shared current view manager cleanup complete");
}
