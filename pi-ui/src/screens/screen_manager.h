#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include "../state/device_state.h"

#ifdef __cplusplus
extern "C" {
#endif

// Screen types
typedef enum {
    SCREEN_NONE = 0,
    SCREEN_BOOT,
    SCREEN_HOME,
    SCREEN_DETAIL_VIEW,
    SCREEN_COUNT
} screen_type_t;

/**
 * @brief Screen Manager - Create/Destroy Pattern
 *
 * This screen manager follows a clean create/destroy pattern:
 * - Every screen transition destroys the current screen completely
 * - Every screen transition creates the new screen from scratch
 * - No state is maintained between transitions (state is in device_state)
 * - Clean, predictable behavior with proper resource management
 */

// Screen creation function type - creates screen from scratch
typedef void (*screen_create_func_t)(void);

// Screen destruction function type - completely destroys screen
typedef void (*screen_destroy_func_t)(void);

// Screen definition
typedef struct {
	screen_type_t screen_type;
	const char *module_name;  // NULL for non-module screens
	screen_create_func_t create_func;
	screen_destroy_func_t destroy_func;
} screen_definition_t;

/**
 * @brief Initialize the screen manager
 */
void screen_manager_init(void);

/**
 * @brief Process any pending screen transitions
 * This is the main function called from the UI update loop
 */
void screen_manager_update(void);

/**
 * @brief Show a specific screen (destroys current, creates new)
 * @param screen_type The type of screen to show
 * @param module_name Module name for detail screens (can be NULL)
 */
void screen_manager_show_screen(screen_type_t screen_type, const char *module_name);

/**
 * @brief Cleanup the screen manager and destroy current screen
 */
void screen_manager_cleanup(void);

/**
 * @brief Get the currently active screen type
 */
screen_type_t screen_manager_get_current_screen(void);

/**
 * @brief Get the currently active module name
 */
const char* screen_manager_get_current_module(void);

// Screen navigation functions (for backward compatibility)
screen_type_t screen_navigation_get_current_screen(void);
const char* screen_navigation_get_current_module(void);
screen_type_t screen_navigation_get_requested_screen(void);
const char* screen_navigation_get_requested_module(void);

// Missing function declarations - these need to be implemented
void screen_navigation_set_current_screen(screen_type_t screen_type, const char* module_name);
bool screen_navigation_is_transition_pending(void);
void screen_navigation_process_transitions(void);
void screen_navigation_request_home_screen(void);
void screen_navigation_request_detail_view(const char* module_name);
void module_screen_view_set_view_index(const char* module_name, int view_index);
void view_state_check_timeout(void);

#ifdef __cplusplus
}
#endif

#endif // SCREEN_MANAGER_H
