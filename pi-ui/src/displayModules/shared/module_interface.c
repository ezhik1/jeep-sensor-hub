#include "module_interface.h"
#include "../../esp_compat.h"

static const char *TAG = "module_interface";

// Forward declarations for module interfaces
extern const display_module_t power_monitor_module;

// Registry of all display modules
static const display_module_t* registered_modules[] = {
	&power_monitor_module,
	// Add other modules here as they are created
};

#define NUM_MODULES (sizeof(registered_modules) / sizeof(registered_modules[0]))

void display_modules_init_all(void)
{
	ESP_LOGI(TAG, "Initializing %d display modules", NUM_MODULES);

	for (size_t i = 0; i < NUM_MODULES; i++) {
		const display_module_t* module = registered_modules[i];
		if (module && module->init) {
			ESP_LOGI(TAG, "Initializing module: %s", module->name);
			module->init();
		} else {
			ESP_LOGW(TAG, "Module %zu has no init function", i);
		}
	}

	ESP_LOGI(TAG, "All display modules initialized");
}

void display_modules_update_all(void)
{
	for (size_t i = 0; i < NUM_MODULES; i++) {
		const display_module_t* module = registered_modules[i];
		if (module && module->update) {
			module->update();
		}
	}
}

void display_modules_cleanup_all(void)
{
	ESP_LOGI(TAG, "Cleaning up %d display modules", NUM_MODULES);

	for (size_t i = 0; i < NUM_MODULES; i++) {
		const display_module_t* module = registered_modules[i];
		if (module && module->cleanup) {
			ESP_LOGI(TAG, "Cleaning up module: %s", module->name);
			module->cleanup();
		}
	}

	ESP_LOGI(TAG, "All display modules cleaned up");
}
