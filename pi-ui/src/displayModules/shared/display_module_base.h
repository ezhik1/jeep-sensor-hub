#ifndef DISPLAY_MODULE_BASE_H
#define DISPLAY_MODULE_BASE_H

#include <lvgl.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Display Module Base - common lifecycle for all display modules
 *
 * All display modules should implement this lifecycle:
 * - create: Initialize UI elements once (called when module becomes visible)
 * - destroy: Clean up all UI elements (called when module becomes hidden)
 * - render: Per-frame UI updates (called each frame, no data writes)
 *
 * Modules subscribe to app_data_store for their data - they don't own it.
 */

typedef struct display_module_base_s {
	const char* name;           // Module name (e.g., "power-monitor")
	void* instance;             // Module-specific instance data (UI state only)

	// Lifecycle methods
	void (*create)(lv_obj_t* container);  // Create UI in container
	void (*destroy)(void);                // Destroy all UI
	void (*render)(void);                 // Per-frame UI updates

	// State
	bool is_created;            // Whether UI is currently created
	lv_obj_t* container;        // Current container (if created)
} display_module_base_t;

/**
 * @brief Initialize a display module base
 */
static inline void display_module_base_init(
	display_module_base_t* module,
	const char* name,
	void* instance,
	void (*create)(lv_obj_t*),
	void (*destroy)(void),
	void (*render)(void)
) {
	if (!module) return;
	module->name = name;
	module->instance = instance;
	module->create = create;
	module->destroy = destroy;
	module->render = render;
	module->is_created = false;
	module->container = NULL;
}

/**
 * @brief Create the module UI in a container
 */
static inline void display_module_base_create(display_module_base_t* module, lv_obj_t* container) {
	if (!module || !container || module->is_created) return;
	if (module->create) {
		module->create(container);
		module->is_created = true;
		module->container = container;
	}
}

/**
 * @brief Destroy the module UI
 */
static inline void display_module_base_destroy(display_module_base_t* module) {
	if (!module || !module->is_created) return;
	if (module->destroy) {
		module->destroy();
	}
	module->is_created = false;
	module->container = NULL;
}

/**
 * @brief Render the module (per-frame updates)
 */
static inline void display_module_base_render(display_module_base_t* module) {
	if (!module || !module->is_created) return;
	if (module->render) {
		module->render();
	}
}

/**
 * @brief Clear the module base
 */
static inline void display_module_base_clear(display_module_base_t* module) {
	if (!module) return;
	module->name = NULL;
	module->instance = NULL;
	module->create = NULL;
	module->destroy = NULL;
	module->render = NULL;
	module->is_created = false;
	module->container = NULL;
}

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_MODULE_BASE_H


