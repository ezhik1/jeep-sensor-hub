#ifndef MODULE_INTERFACE_H
#define MODULE_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Standardized interface for all display modules
 *
 * Each display module should implement these functions to provide
 * a consistent interface for initialization and updates.
 */

/**
 * @brief Initialize a display module
 *
 * This function should be called once during application startup
 * to initialize the module's internal state, create UI components,
 * and set up any required resources.
 */
typedef void (*module_init_func_t)(void);

/**
 * @brief Update a display module
 *
 * This function should be called periodically (e.g., every frame)
 * to update the module's data, refresh UI components, and handle
 * any periodic tasks. This should NOT create or destroy UI elements.
 */
typedef void (*module_update_func_t)(void);

/**
 * @brief Cleanup a display module
 *
 * This function should be called during application shutdown
 * to clean up resources and destroy UI components.
 */
typedef void (*module_cleanup_func_t)(void);

/**
 * @brief Display module interface structure
 */
typedef struct {
	const char *name;                    // Module name for identification
	module_init_func_t init;            // Initialization function
	module_update_func_t update;        // Update function (called every tick)
	module_cleanup_func_t cleanup;      // Cleanup function
} display_module_t;

/**
 * @brief Initialize all registered display modules
 */
void display_modules_init_all(void);

/**
 * @brief Update all registered display modules
 */
void display_modules_update_all(void);

/**
 * @brief Cleanup all registered display modules
 */
void display_modules_cleanup_all(void);

#ifdef __cplusplus
}
#endif

#endif // MODULE_INTERFACE_H
