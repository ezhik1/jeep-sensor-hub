#ifndef NUMBERPAD_H
#define NUMBERPAD_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Numberpad configuration
typedef struct {
	int max_digits;           // Maximum number of digits allowed
	int decimal_places;       // Number of decimal places (0 = no decimal)
	bool auto_decimal;        // Automatically add decimal point
	const char* clear_text;   // Text for clear button
	lv_color_t bg_color;      // Background color
	lv_color_t border_color;  // Border color
	lv_color_t text_color;    // Text color
	lv_coord_t button_width;  // Width of each button
	lv_coord_t button_height; // Height of each button
	lv_coord_t button_gap;    // Gap between buttons
} numberpad_config_t;

// Numberpad instance
typedef struct {
	lv_obj_t* background;
	lv_obj_t* buttons[11];    // 0-9, CLEAR (no ENTER button)
	lv_obj_t* target_field;   // Field being edited
	char* value_buffer;       // Current input value
	int buffer_size;
	int current_length;
	numberpad_config_t config;
	void (*on_value_changed)(const char* value, void* user_data);
	void (*on_clear)(void* user_data);
	void (*on_enter)(const char* value, void* user_data);
	void* user_data;
	bool is_visible;
	bool is_first_digit;      // True when first digit input after opening numberpad
	int digit_count;          // Number of digits entered
} numberpad_t;

// Default configuration
extern const numberpad_config_t NUMBERPAD_DEFAULT_CONFIG;

// Create numberpad
numberpad_t* numberpad_create(const numberpad_config_t* config, lv_obj_t* parent);

// Destroy numberpad
void numberpad_destroy(numberpad_t* numpad);

// Show numberpad positioned relative to target field
void numberpad_show(numberpad_t* numpad, lv_obj_t* target_field);

// Show numberpad with first digit flag
void numberpad_show_with_first_digit_flag(numberpad_t* numpad, lv_obj_t* target_field, bool is_first_digit);

// Hide numberpad
void numberpad_hide(numberpad_t* numpad);

// Set callbacks
void numberpad_set_callbacks(numberpad_t* numpad,
							void (*on_value_changed)(const char* value, void* user_data),
							void (*on_clear)(void* user_data),
							void (*on_enter)(const char* value, void* user_data),
							void* user_data);

// Get current value
const char* numberpad_get_value(numberpad_t* numpad);

// Set current value
void numberpad_set_value(numberpad_t* numpad, const char* value);

// Check if numberpad is visible
bool numberpad_is_visible(numberpad_t* numpad);

#ifdef __cplusplus
}
#endif

#endif // NUMBERPAD_H
