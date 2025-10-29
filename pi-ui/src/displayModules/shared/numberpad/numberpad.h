#ifndef NUMBERPAD_H
#define NUMBERPAD_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Numberpad position relative to target field
typedef enum {
	NUMBERPAD_POSITION_BELOW,
	NUMBERPAD_POSITION_ABOVE,
	NUMBERPAD_POSITION_RIGHT,
	NUMBERPAD_POSITION_LEFT,
	NUMBERPAD_POSITION_OTHER
} numberpad_position_t;

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
	lv_obj_t* buttons[13];    // 0-9, CLEAR, NEGATIVE, CANCEL
	lv_obj_t* target_field;   // Field being edited
	char* value_buffer;       // Current input value (numeric)
	char* display_buffer;     // Display value (with k notation)
	int buffer_size;
	int current_length;
	numberpad_config_t config;
	void (*on_value_changed)(const char* value, void* user_data);
	void (*on_clear)(void* user_data);
	void (*on_enter)(const char* value, void* user_data);
	void (*on_cancel)(void* user_data);
	void* user_data;
	bool is_visible;
	bool is_first_digit;      // True when first digit input after opening numberpad
	int digit_count;          // Number of digits entered
	numberpad_position_t position; // Position relative to target field
	bool is_negative;         // True if current value is negative
} numberpad_t;

// Default configuration
extern const numberpad_config_t NUMBERPAD_DEFAULT_CONFIG;

// Create numberpad
numberpad_t* numberpad_create(const numberpad_config_t* config, lv_obj_t* parent);

// Destroy numberpad
void numberpad_destroy(numberpad_t* numpad);

// Show numberpad positioned relative to target field
void numberpad_show(numberpad_t* numpad, lv_obj_t* target_field);

// Show numberpad aligned to field but positioned outside container
void numberpad_show_outside_container(numberpad_t* numpad, lv_obj_t* target_field, lv_obj_t* container);

// Show numberpad with first digit flag
void numberpad_show_with_first_digit_flag(numberpad_t* numpad, lv_obj_t* target_field, bool is_first_digit);

// Hide numberpad
void numberpad_hide(numberpad_t* numpad);

// Set callbacks
void numberpad_set_callbacks(
	numberpad_t* numpad,
	void (*on_value_changed)(const char* value, void* user_data),
	void (*on_clear)(void* user_data),
	void (*on_enter)(const char* value, void* user_data),
	void (*on_cancel)(void* user_data),
	void* user_data);

// Get current value
const char* numberpad_get_value(numberpad_t* numpad);

// Set current value
void numberpad_set_value(numberpad_t* numpad, const char* value);

// Check if numberpad is visible
bool numberpad_is_visible(numberpad_t* numpad);

// Get numberpad position relative to target field
numberpad_position_t numberpad_get_position(numberpad_t* numpad);

// Reset negative state to positive (used when value triggers a clamp)
void numberpad_reset_negative_state(numberpad_t* numpad);

// Set value and prepare for fresh input (next digit will clear and start fresh)
void numberpad_set_value_for_fresh_input(numberpad_t* numpad, const char* value);

// Convert display value (with k notation) to numeric value
float numberpad_get_numeric_value(const char* display_value);

// Convert numeric value to display value (with k notation for values >= 1000)
void numberpad_format_display_value(float numeric_value, char* display_buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // NUMBERPAD_H
