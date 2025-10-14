#include <stdio.h>
#include "module_interface.h"


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
		printf("[I] module_interface: Initializing %d display modules\n", NUM_MODULES);

	for (size_t i = 0; i < NUM_MODULES; i++) {
		const display_module_t* module = registered_modules[i];
		if (module && module->init) {
			printf("[I] module_interface: Initializing module: %s\n", module->name);
			module->init();
		} else {
			printf("[W] module_interface: Module %zu has no init function\n", i);
		}
	}

	printf("[I] module_interface: All display modules initialized\n");
}

void display_modules_update_all(void)
{
	// Updating all modules

	for (size_t i = 0; i < NUM_MODULES; i++) {
		const display_module_t* module = registered_modules[i];
		if (module && module->update) {
			module->update();
		}
	}
}

void display_modules_cleanup_all(void)
{
		printf("[I] module_interface: Cleaning up %d display modules\n", NUM_MODULES);

	for (size_t i = 0; i < NUM_MODULES; i++) {
		const display_module_t* module = registered_modules[i];
		if (module && module->cleanup) {
			printf("[I] module_interface: Cleaning up module: %s\n", module->name);
			module->cleanup();
		}
	}

	printf("[I] module_interface: All display modules cleaned up\n");
}
