#ifndef TIME_INPUT_H
#define TIME_INPUT_H

#include <lvgl.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Time input configuration
typedef struct {
	int max_hours;      // Maximum hours (default: 23)
	int max_minutes;    // Maximum minutes (default: 59)
	int max_seconds;    // Maximum seconds (default: 59)
} time_input_config_t;

// Default configuration
#define TIME_INPUT_DEFAULT_CONFIG { \
	.max_hours = 23, \
	.max_minutes = 59, \
	.max_seconds = 59 \
}

// Time input structure
typedef struct {
	lv_obj_t* background;           // Main background container
	lv_obj_t* content_container;    // Content container

	// Rollers for hours, minutes, seconds
	lv_obj_t* rollers[3];           // Hours, minutes, seconds rollers
	lv_obj_t* labels[3];            // Labels for each roller

	// Preset buttons
	lv_obj_t* preset_buttons[6];    // 30s, 1min, 30min, 1hr, 3hr, realtime buttons

	// Current values
	int hours;
	int minutes;
	int seconds;

	// Configuration
	time_input_config_t config;

	// State
	bool is_visible;
	lv_obj_t* target_field;         // Field this time input is targeting

	// Callbacks
	void (*on_value_changed)(int hours, int minutes, int seconds, void* user_data);
	void (*on_enter)(int hours, int minutes, int seconds, void* user_data);
	void (*on_cancel)(void* user_data);
	void* user_data;
} time_input_t;

/**
 * @brief Create a new time input component
 * @param config Configuration for the time input
 * @param parent Parent object to attach to
 * @return Pointer to created time input, or NULL on failure
 */
time_input_t* time_input_create(const time_input_config_t* config, lv_obj_t* parent);

/**
 * @brief Destroy the time input component
 * @param time_input Pointer to the time input to destroy
 */
void time_input_destroy(time_input_t* time_input);

/**
 * @brief Show time input positioned relative to target field
 * @param time_input Pointer to the time input
 * @param target_field Field to position relative to
 */
void time_input_show(time_input_t* time_input, lv_obj_t* target_field);

/**
 * @brief Show time input aligned to field but positioned outside container
 * @param time_input Pointer to the time input
 * @param target_field Field to align to
 * @param container Container to position outside of
 */
void time_input_show_outside_container(time_input_t* time_input, lv_obj_t* target_field, lv_obj_t* container);

/**
 * @brief Hide time input
 * @param time_input Pointer to the time input
 */
void time_input_hide(time_input_t* time_input);

/**
 * @brief Set callbacks
 * @param time_input Pointer to the time input
 * @param on_value_changed Callback for value changes
 * @param on_enter Callback for enter/confirm
 * @param on_cancel Callback for cancel
 * @param user_data User data to pass to callbacks
 */
void time_input_set_callbacks(
	time_input_t* time_input,
	void (*on_value_changed)(int hours, int minutes, int seconds, void* user_data),
	void (*on_enter)(int hours, int minutes, int seconds, void* user_data),
	void (*on_cancel)(void* user_data),
	void* user_data);

/**
 * @brief Set current time values
 * @param time_input Pointer to the time input
 * @param hours Hours value
 * @param minutes Minutes value
 * @param seconds Seconds value
 */
void time_input_set_values(time_input_t* time_input, int hours, int minutes, int seconds);

/**
 * @brief Get current time values
 * @param time_input Pointer to the time input
 * @param hours Pointer to store hours value
 * @param minutes Pointer to store minutes value
 * @param seconds Pointer to store seconds value
 */
void time_input_get_values(time_input_t* time_input, int* hours, int* minutes, int* seconds);

/**
 * @brief Check if time input is visible
 * @param time_input Pointer to the time input
 * @return true if visible, false otherwise
 */
bool time_input_is_visible(time_input_t* time_input);

#ifdef __cplusplus
}
#endif

#endif // TIME_INPUT_H
