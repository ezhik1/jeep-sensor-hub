#include <stdio.h>
#include "numberpad.h"
#include <string.h>
#include <stdlib.h>
#include "../utils/positioning/positioning.h"
#include "../utils/number_formatting/number_formatting.h"

static const char* TAG = "numberpad";

// Default configuration
const numberpad_config_t NUMBERPAD_DEFAULT_CONFIG = {
	.max_digits = 4,
	.decimal_places = 1,
	.auto_decimal = true,
	.clear_text = "CLEAR",
	.bg_color = {0x00, 0x00, 0x00},
	.border_color = {0xFF, 0xFF, 0xFF},
	.text_color = {0xFF, 0xFF, 0xFF},
	.button_width = 65,
	.button_height = 55,
	.button_gap = 5
};

// Forward declarations
static void numberpad_button_cb(lv_event_t* e);
static void set_numberpad_smart(numberpad_t* numpad, lv_obj_t* target_field);
static void set_numberpad_smart_outside_container(numberpad_t* numpad, lv_obj_t* target_field, lv_obj_t* container);
static void update_target_field(numberpad_t* numpad);
static void add_digit(numberpad_t* numpad, char digit);
static void clear_value(numberpad_t* numpad);
static void enter_value(numberpad_t* numpad);
static void toggle_negative(numberpad_t* numpad);
static void reset_negative_state(numberpad_t* numpad);
static void reset_negative_state_silent(numberpad_t* numpad);
static void set_value_for_fresh_input(numberpad_t* numpad, const char* value);
static void cancel_value(numberpad_t* numpad);
static void update_display_value(numberpad_t* numpad);

// Enhanced positioning algorithm - ensures no obscuring and always fits screen

numberpad_t* numberpad_create(const numberpad_config_t* config, lv_obj_t* parent) {
	if (!config || !parent) return NULL;

	numberpad_t* numpad = malloc(sizeof(numberpad_t));
	if (!numpad) return NULL;

	memset(numpad, 0, sizeof(numberpad_t));
	numpad->config = *config;
	numpad->buffer_size = config->max_digits + config->decimal_places + 10; // +10 for decimal point, 'k' notation, and safety margin
	numpad->value_buffer = malloc(numpad->buffer_size);
	numpad->display_buffer = malloc(numpad->buffer_size);
	if (!numpad->value_buffer || !numpad->display_buffer) {
		free(numpad->value_buffer);
		free(numpad->display_buffer);
		free(numpad);
		return NULL;
	}
	numpad->value_buffer[0] = '\0';
	numpad->display_buffer[0] = '\0';

	// Create background
	numpad->background = lv_obj_create(parent);
	lv_obj_set_style_bg_color(numpad->background, config->bg_color, 0);
	lv_obj_set_style_border_width(numpad->background, 0, 0); // Remove border
	lv_obj_set_style_radius(numpad->background, 3, 0);
	lv_obj_set_style_pad_all(numpad->background, 0, 0);
	lv_obj_clear_flag(numpad->background, LV_OBJ_FLAG_SCROLLABLE);

	// Make sure the background can receive touch events
	lv_obj_clear_flag(numpad->background, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(numpad->background, LV_OBJ_FLAG_CLICKABLE);

	// Calculate numberpad size - now 5 rows with negative and cancel buttons
	lv_coord_t numpad_width = (config->button_width * 3) + (config->button_gap * 2);
	lv_coord_t numpad_height = (config->button_height * 5) + (config->button_gap * 4);
	lv_obj_set_size(numpad->background, numpad_width, numpad_height);

	// Create number buttons (1-9)
	const char* number_labels[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9"};
	for (int i = 0; i < 9; i++) {
		numpad->buttons[i] = lv_button_create(numpad->background);
		lv_obj_set_size(numpad->buttons[i], config->button_width, config->button_height);

		int row = i / 3;
		int col = i % 3;
		lv_obj_set_pos(numpad->buttons[i],
					  col * (config->button_width + config->button_gap),
					  row * (config->button_height + config->button_gap));

		lv_obj_set_style_bg_color(numpad->buttons[i], lv_color_hex(0x556b2f), 0); // Dark olive green
		lv_obj_set_style_bg_color(numpad->buttons[i], lv_color_hex(0x3d4f1f), LV_STATE_PRESSED); // Darker olive green when pressed
		lv_obj_set_style_border_width(numpad->buttons[i], 0, 0); // Remove border
		lv_obj_set_style_radius(numpad->buttons[i], 3, 0);
		lv_obj_set_style_shadow_width(numpad->buttons[i], 0, 0); // Remove drop shadow

		lv_obj_t* label = lv_label_create(numpad->buttons[i]);
		lv_label_set_text(label, number_labels[i]);
		lv_obj_set_style_text_color(label, config->text_color, 0);
		lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
		lv_obj_center(label);

		lv_obj_add_event_cb(numpad->buttons[i], numberpad_button_cb, LV_EVENT_CLICKED, numpad);
	}

	// Create 0 button
	numpad->buttons[9] = lv_button_create(numpad->background);
	lv_obj_set_size(numpad->buttons[9], config->button_width, config->button_height);
	lv_obj_set_pos(numpad->buttons[9], 0, 3 * (config->button_height + config->button_gap));

	lv_obj_set_style_bg_color(numpad->buttons[9], lv_color_hex(0x556b2f), 0); // Dark olive green
	lv_obj_set_style_bg_color(numpad->buttons[9], lv_color_hex(0x3d4f1f), LV_STATE_PRESSED); // Darker olive green when pressed
	lv_obj_set_style_border_width(numpad->buttons[9], 0, 0); // Remove border
	lv_obj_set_style_radius(numpad->buttons[9], 3, 0);
	lv_obj_set_style_shadow_width(numpad->buttons[9], 0, 0); // Remove drop shadow

	lv_obj_t* zero_label = lv_label_create(numpad->buttons[9]);
	lv_label_set_text(zero_label, "0");
	lv_obj_set_style_text_color(zero_label, config->text_color, 0);
	lv_obj_set_style_text_font(zero_label, &lv_font_montserrat_20, 0);
	lv_obj_center(zero_label);
	lv_obj_add_event_cb(numpad->buttons[9], numberpad_button_cb, LV_EVENT_CLICKED, numpad);

	// Create CLEAR button (2 units wide)
	numpad->buttons[10] = lv_button_create(numpad->background);
	lv_obj_set_size(numpad->buttons[10], (config->button_width * 2) + config->button_gap, config->button_height);
	lv_obj_set_pos(numpad->buttons[10],
				  config->button_width + config->button_gap,
				  3 * (config->button_height + config->button_gap));

	lv_obj_set_style_bg_color(numpad->buttons[10], lv_color_hex(0xba3232), 0); // Red
	lv_obj_set_style_bg_color(numpad->buttons[10], lv_color_hex(0x8b2525), LV_STATE_PRESSED); // Darker red when pressed
	lv_obj_set_style_border_width(numpad->buttons[10], 0, 0); // Remove border
	lv_obj_set_style_radius(numpad->buttons[10], 3, 0);
	lv_obj_set_style_shadow_width(numpad->buttons[10], 0, 0); // Remove drop shadow

	lv_obj_t* clear_label = lv_label_create(numpad->buttons[10]);
	lv_label_set_text(clear_label, config->clear_text);
	lv_obj_set_style_text_color(clear_label, config->text_color, 0);
	lv_obj_set_style_text_font(clear_label, &lv_font_montserrat_16, 0);
	lv_obj_center(clear_label);
	lv_obj_add_event_cb(numpad->buttons[10], numberpad_button_cb, LV_EVENT_CLICKED, numpad);

	// Create NEGATIVE button (below 0)
	numpad->buttons[11] = lv_button_create(numpad->background);
	lv_obj_set_size(numpad->buttons[11], config->button_width, config->button_height);
	lv_obj_set_pos(numpad->buttons[11], 0, 4 * (config->button_height + config->button_gap));

	lv_obj_set_style_bg_color(numpad->buttons[11], lv_color_hex(0x556b2f), 0); // Dark olive green
	lv_obj_set_style_bg_color(numpad->buttons[11], lv_color_hex(0x3d4f1f), LV_STATE_PRESSED); // Darker olive green when pressed
	lv_obj_set_style_border_width(numpad->buttons[11], 0, 0); // Remove border
	lv_obj_set_style_radius(numpad->buttons[11], 3, 0);
	lv_obj_set_style_shadow_width(numpad->buttons[11], 0, 0); // Remove drop shadow

	lv_obj_t* negative_label = lv_label_create(numpad->buttons[11]);
	lv_label_set_text(negative_label, "+-");
	lv_obj_set_style_text_color(negative_label, config->text_color, 0);
	lv_obj_set_style_text_font(negative_label, &lv_font_montserrat_20, 0);
	lv_obj_center(negative_label);
	lv_obj_add_event_cb(numpad->buttons[11], numberpad_button_cb, LV_EVENT_CLICKED, numpad);

	// Create CANCEL button (next to negative, same size as clear)
	numpad->buttons[12] = lv_button_create(numpad->background);
	lv_obj_set_size(numpad->buttons[12], (config->button_width * 2) + config->button_gap, config->button_height);
	lv_obj_set_pos(numpad->buttons[12],
				  config->button_width + config->button_gap,
				  4 * (config->button_height + config->button_gap));

	lv_obj_set_style_bg_color(numpad->buttons[12], lv_color_hex(0x87CEEB), 0); // Light blue
	lv_obj_set_style_bg_color(numpad->buttons[12], lv_color_hex(0x5F9EA0), LV_STATE_PRESSED); // Darker blue when pressed
	lv_obj_set_style_border_width(numpad->buttons[12], 0, 0); // Remove border
	lv_obj_set_style_radius(numpad->buttons[12], 3, 0);
	lv_obj_set_style_shadow_width(numpad->buttons[12], 0, 0); // Remove drop shadow

	lv_obj_t* cancel_label = lv_label_create(numpad->buttons[12]);
	lv_label_set_text(cancel_label, "CANCEL");
	lv_obj_set_style_text_color(cancel_label, config->text_color, 0);
	lv_obj_set_style_text_font(cancel_label, &lv_font_montserrat_16, 0);
	lv_obj_center(cancel_label);
	lv_obj_add_event_cb(numpad->buttons[12], numberpad_button_cb, LV_EVENT_CLICKED, numpad);

	// No ENTER button - save on close

	// Initially hidden
	lv_obj_add_flag(numpad->background, LV_OBJ_FLAG_HIDDEN);
	numpad->is_visible = false;

		printf("[I] numberpad: Numberpad created with size %dx%d\n", numpad_width, numpad_height);
	return numpad;
}

void numberpad_destroy(numberpad_t* numpad) {
	if (!numpad) return;

	if (numpad->value_buffer) {
		free(numpad->value_buffer);
	}
	if (numpad->display_buffer) {
		free(numpad->display_buffer);
	}

	if (numpad->background) {
		lv_obj_del(numpad->background);
	}

	free(numpad);
}

void numberpad_show(numberpad_t* numpad, lv_obj_t* target_field) {
	if (!numpad || !target_field) return;

	numpad->target_field = target_field;
	set_numberpad_smart(numpad, target_field);
	lv_obj_clear_flag(numpad->background, LV_OBJ_FLAG_HIDDEN);
	numpad->is_visible = true;
	numpad->is_first_digit = true; // First digit input will clear current value
	numpad->is_negative = false; // Initialize negative flag

	printf("[I] numberpad: Numberpad shown for target field\n");
}

void numberpad_show_outside_container(numberpad_t* numpad, lv_obj_t* target_field, lv_obj_t* container) {
	if (!numpad || !target_field || !container) return;

	numpad->target_field = target_field;
	set_numberpad_smart_outside_container(numpad, target_field, container);
	lv_obj_clear_flag(numpad->background, LV_OBJ_FLAG_HIDDEN);
	numpad->is_visible = true;
	numpad->is_first_digit = true; // First digit input will clear current value
	numpad->is_negative = false; // Initialize negative flag

	printf("[I] numberpad: Numberpad shown outside container, aligned to field\n");
}

void numberpad_show_with_first_digit_flag(numberpad_t* numpad, lv_obj_t* target_field, bool is_first_digit) {
	if (!numpad || !target_field) return;

	numpad->target_field = target_field;
	set_numberpad_smart(numpad, target_field);
	lv_obj_clear_flag(numpad->background, LV_OBJ_FLAG_HIDDEN);
	numpad->is_visible = true;
	numpad->is_first_digit = is_first_digit; // Use provided flag
	numpad->is_negative = false; // Initialize negative flag

		printf("[I] numberpad: Numberpad shown for target field (first_digit=%s)\n", is_first_digit ? "true" : "false");
}

void numberpad_hide(numberpad_t* numpad) {
	if (!numpad) return;

	lv_obj_add_flag(numpad->background, LV_OBJ_FLAG_HIDDEN);
	numpad->is_visible = false;
	numpad->target_field = NULL;

	// Reset state when hiding
	numpad->is_first_digit = false;
	numpad->digit_count = 0;

	// Reset all button colors to default
	for (int i = 0; i < 11; i++) {
		if (numpad->buttons[i]) {
			// Reset button state to remove any pressed/highlighted appearance
			lv_obj_clear_state(numpad->buttons[i], LV_STATE_PRESSED);
			lv_obj_clear_state(numpad->buttons[i], LV_STATE_FOCUSED);
			lv_obj_clear_state(numpad->buttons[i], LV_STATE_FOCUS_KEY);

			// Reset to default colors
			if (i < 10) {
				// Number buttons (0-9): dark olive green background, white text
				lv_obj_set_style_bg_color(numpad->buttons[i], lv_color_hex(0x556b2f), 0);
				lv_obj_set_style_bg_color(numpad->buttons[i], lv_color_hex(0x556b2f), LV_STATE_PRESSED);
				lv_obj_set_style_bg_color(numpad->buttons[i], lv_color_hex(0x556b2f), LV_STATE_FOCUSED);
			} else {
				// CLEAR button: red background, white text
				lv_obj_set_style_bg_color(numpad->buttons[i], lv_color_hex(0xba3232), 0);
				lv_obj_set_style_bg_color(numpad->buttons[i], lv_color_hex(0xba3232), LV_STATE_PRESSED);
				lv_obj_set_style_bg_color(numpad->buttons[i], lv_color_hex(0xba3232), LV_STATE_FOCUSED);
			}

			// Reset text color to white for all buttons
			lv_obj_t* label = lv_obj_get_child(numpad->buttons[i], 0);
			if (label) {
				lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
				lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_STATE_PRESSED);
				lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_STATE_FOCUSED);
			}
		}
	}

	printf("[I] numberpad: Numberpad hidden and button colors reset\n");
}

void numberpad_set_callbacks(numberpad_t* numpad,
							void (*on_value_changed)(const char* value, void* user_data),
							void (*on_clear)(void* user_data),
							void (*on_enter)(const char* value, void* user_data),
							void (*on_cancel)(void* user_data),
							void* user_data) {
	if (!numpad) return;

	numpad->on_value_changed = on_value_changed;
	numpad->on_clear = on_clear;
	numpad->on_enter = on_enter;
	numpad->on_cancel = on_cancel;
	numpad->user_data = user_data;
}

const char* numberpad_get_value(numberpad_t* numpad) {
	return numpad ? numpad->value_buffer : NULL;
}

void numberpad_set_value(numberpad_t* numpad, const char* value) {
	if (!numpad || !value) return;

	strncpy(numpad->value_buffer, value, numpad->buffer_size - 1);
	numpad->value_buffer[numpad->buffer_size - 1] = '\0';
	numpad->current_length = strlen(numpad->value_buffer);

	// No need to determine fresh input state

	update_target_field(numpad);
}

bool numberpad_is_visible(numberpad_t* numpad) {
	return numpad ? numpad->is_visible : false;
}

// Enhanced smart positioning algorithm - ensures no obscuring and always fits screen
static void set_numberpad_smart(numberpad_t* numpad, lv_obj_t* target_field) {
	if (!numpad || !target_field) return;

	// Ensure layout is updated before getting coordinates
	lv_obj_update_layout(target_field);

	// Get target field coordinates
	lv_area_t field_coords;
	lv_obj_get_coords(target_field, &field_coords);

	// Get screen dimensions
	lv_coord_t screen_width = lv_obj_get_width(lv_screen_active());
	lv_coord_t screen_height = lv_obj_get_height(lv_screen_active());

	// Get numberpad dimensions
	lv_coord_t pad_width = lv_obj_get_width(numpad->background);
	lv_coord_t pad_height = lv_obj_get_height(numpad->background);

	// Minimum gap between numberpad and field
	const lv_coord_t min_gap = 10;
	// Screen margins
	const lv_coord_t screen_margin = 5;

	// Calculate field center
	lv_coord_t field_center_x = field_coords.x1 + lv_area_get_width(&field_coords) / 2;
	lv_coord_t field_center_y = field_coords.y1 + lv_area_get_height(&field_coords) / 2;

	// Try different positioning strategies in order of preference
	lv_coord_t best_x = 0, best_y = 0;
	bool found_position = false;

	// Strategy 1: Position below field (preferred)
	best_x = field_center_x - pad_width / 2;
	best_y = field_coords.y2 + min_gap;

	if (best_x >= screen_margin &&
		best_x + pad_width <= screen_width - screen_margin &&
		best_y >= screen_margin &&
		best_y + pad_height <= screen_height - screen_margin) {
		found_position = true;
		numpad->position = NUMBERPAD_POSITION_BELOW;
		printf("[I] numberpad: Positioned numberpad below field\n");
	}

	// Strategy 2: Position above field
	if (!found_position) {
		best_x = field_center_x - pad_width / 2;
		best_y = field_coords.y1 - pad_height - min_gap;

		if (best_x >= screen_margin &&
			best_x + pad_width <= screen_width - screen_margin &&
			best_y >= screen_margin &&
			best_y + pad_height <= screen_height - screen_margin) {
			found_position = true;
			numpad->position = NUMBERPAD_POSITION_ABOVE;
			printf("[I] numberpad: Positioned numberpad above field\n");
		}
	}

	// Strategy 3: Position to the right of field
	if (!found_position) {
		best_x = field_coords.x2 + min_gap;
		best_y = field_center_y - pad_height / 2;

		if (best_x >= screen_margin &&
			best_x + pad_width <= screen_width - screen_margin &&
			best_y >= screen_margin &&
			best_y + pad_height <= screen_height - screen_margin) {
			found_position = true;
			numpad->position = NUMBERPAD_POSITION_RIGHT;
			printf("[I] numberpad: Positioned numberpad to the right of field\n");
		}
	}

	// Strategy 4: Position to the left of field
	if (!found_position) {
		best_x = field_coords.x1 - pad_width - min_gap;
		best_y = field_center_y - pad_height / 2;

		if (best_x >= screen_margin &&
			best_x + pad_width <= screen_width - screen_margin &&
			best_y >= screen_margin &&
			best_y + pad_height <= screen_height - screen_margin) {
			found_position = true;
			numpad->position = NUMBERPAD_POSITION_LEFT;
			printf("[I] numberpad: Positioned numberpad to the left of field\n");
		}
	}

	// Strategy 5: Try to position below field, but adjust if it goes off screen
	if (!found_position) {
		best_x = field_center_x - pad_width / 2;
		best_y = field_coords.y2 + min_gap;

		// Adjust if it goes off screen
		if (best_x < screen_margin) best_x = screen_margin;
		if (best_x + pad_width > screen_width - screen_margin) best_x = screen_width - pad_width - screen_margin;
		if (best_y + pad_height > screen_height - screen_margin) best_y = screen_height - pad_height - screen_margin;

		found_position = true;
		numpad->position = NUMBERPAD_POSITION_BELOW;
		printf("[I] numberpad: Positioned numberpad below field (adjusted for screen)\n");
	}

	// Strategy 6: Try to position above field, but adjust if it goes off screen
	if (!found_position) {
		best_x = field_center_x - pad_width / 2;
		best_y = field_coords.y1 - pad_height - min_gap;

		// Adjust if it goes off screen
		if (best_x < screen_margin) best_x = screen_margin;
		if (best_x + pad_width > screen_width - screen_margin) best_x = screen_width - pad_width - screen_margin;
		if (best_y < screen_margin) best_y = screen_margin;

		found_position = true;
		numpad->position = NUMBERPAD_POSITION_ABOVE;
		printf("[I] numberpad: Positioned numberpad above field (adjusted for screen)\n");
	}

	// Strategy 7: Try to position right of field, but adjust if it goes off screen
	if (!found_position) {
		best_x = field_coords.x2 + min_gap;
		best_y = field_center_y - pad_height / 2;

		// Adjust if it goes off screen
		if (best_x + pad_width > screen_width - screen_margin) best_x = screen_width - pad_width - screen_margin;
		if (best_y < screen_margin) best_y = screen_margin;
		if (best_y + pad_height > screen_height - screen_margin) best_y = screen_height - pad_height - screen_margin;

		found_position = true;
		numpad->position = NUMBERPAD_POSITION_RIGHT;
		printf("[I] numberpad: Positioned numberpad right of field (adjusted for screen)\n");
	}

	// Strategy 8: Try to position left of field, but adjust if it goes off screen
	if (!found_position) {
		best_x = field_coords.x1 - pad_width - min_gap;
		best_y = field_center_y - pad_height / 2;

		// Adjust if it goes off screen
		if (best_x < screen_margin) best_x = screen_margin;
		if (best_y < screen_margin) best_y = screen_margin;
		if (best_y + pad_height > screen_height - screen_margin) best_y = screen_height - pad_height - screen_margin;

		found_position = true;
		numpad->position = NUMBERPAD_POSITION_LEFT;
		printf("[I] numberpad: Positioned numberpad left of field (adjusted for screen)\n");
	}

	// Strategy 9: Center positioning as last resort
	if (!found_position) {
		best_x = (screen_width - pad_width) / 2;
		best_y = (screen_height - pad_height) / 2;
		numpad->position = NUMBERPAD_POSITION_OTHER;
		printf("[I] numberpad: Positioned numberpad at center (fallback)\n");
	}

	// Ensure final position is within screen bounds
	if (best_x < screen_margin) best_x = screen_margin;
	if (best_x + pad_width > screen_width - screen_margin) best_x = screen_width - pad_width - screen_margin;
	if (best_y < screen_margin) best_y = screen_margin;
	if (best_y + pad_height > screen_height - screen_margin) best_y = screen_height - pad_height - screen_margin;

	// Apply the position
	lv_obj_set_pos(numpad->background, best_x, best_y);

		printf("[I] numberpad: Numberpad positioned at (%d, %d) for field at (%d, %d)\n",
			 best_x, best_y, field_coords.x1, field_coords.y1);
}

// Enhanced smart positioning algorithm - aligns to field but positions outside container
static void set_numberpad_smart_outside_container(numberpad_t* numpad, lv_obj_t* target_field, lv_obj_t* container) {
	if (!numpad || !target_field || !container) return;

	// Use generic positioning algorithm
	smart_position_outside_container_default(numpad->background, target_field, container);

	// Set position tracking for numberpad
	numpad->position = NUMBERPAD_POSITION_BELOW; // Default, could be enhanced to track actual position
}

static void numberpad_button_cb(lv_event_t* e) {
	lv_obj_t* btn = lv_event_get_target(e);
	numberpad_t* numpad = lv_event_get_user_data(e);

	if (!numpad) return;

	// Find which button was clicked
	for (int i = 0; i < 13; i++) {
		if (numpad->buttons[i] == btn) {
			if (i < 9) {
				// Number buttons 1-9
				add_digit(numpad, '1' + i);
			} else if (i == 9) {
				// 0 button
				add_digit(numpad, '0');
			} else if (i == 10) {
				// CLEAR button
				clear_value(numpad);
			} else if (i == 11) {
				// NEGATIVE button
				toggle_negative(numpad);
			} else if (i == 12) {
				// CANCEL button
				cancel_value(numpad);
			}
			break;
		}
	}
}

static void add_digit(numberpad_t* numpad, char digit) {
	if (!numpad) return;

	printf("[I] numberpad: add_digit: digit='%c', is_first_digit=%s, current='%s'\n",
			 digit, numpad->is_first_digit ? "true" : "false", numpad->value_buffer);

	// If this is the first digit input, clear current value and start fresh
	if (numpad->is_first_digit) {
		printf("[I] numberpad: First digit: clearing current value and starting fresh\n");
		// Clear current value and start fresh
		numpad->value_buffer[0] = '\0';
		numpad->current_length = 0;
		numpad->is_first_digit = false;
		numpad->digit_count = 0;
		numpad->is_negative = false; // Reset negative flag on first digit
	}

	// Handle 4-digit input with k notation: 1→0.1, 15→1.5, 150→15.0, 1500→1.5k, 3000→3.0k
	if (numpad->digit_count == 0) {
		// First digit: "1" → "0.1" or "-1" → "-0.1"
		if (numpad->is_negative) {
			snprintf(numpad->value_buffer, numpad->buffer_size, "-0.%c", digit);
			numpad->current_length = 4;
		} else {
			snprintf(numpad->value_buffer, numpad->buffer_size, "0.%c", digit);
			numpad->current_length = 3;
		}
		numpad->digit_count = 1;
	} else if (numpad->digit_count == 1) {
		// Second digit: "0.1" → "1.5" or "-0.1" → "-1.5"
		if (numpad->is_negative) {
			snprintf(numpad->value_buffer, numpad->buffer_size, "-%c.%c", numpad->value_buffer[2], digit);
			numpad->current_length = 4;
		} else {
			snprintf(numpad->value_buffer, numpad->buffer_size, "%c.%c", numpad->value_buffer[2], digit);
			numpad->current_length = 3;
		}
		numpad->digit_count = 2;
	} else if (numpad->digit_count == 2) {
		// Third digit: "1.5" → "15.0" or "-1.5" → "-15.0"
		if (numpad->is_negative) {
			snprintf(numpad->value_buffer, numpad->buffer_size, "-%c%c.%c", numpad->value_buffer[1], numpad->value_buffer[3], digit);
			numpad->current_length = 6;
		} else {
			snprintf(numpad->value_buffer, numpad->buffer_size, "%c%c.%c", numpad->value_buffer[0], numpad->value_buffer[2], digit);
			numpad->current_length = 5;
		}
		numpad->digit_count = 3;
	} else if (numpad->digit_count == 3) {
		// Fourth digit: "15.0" → "150.0" (regular progression)
		// Calculate: "15.0" * 10 + digit = 150.0
		float numeric_value;
		if (numpad->is_negative) {
			numeric_value = -(atof(&numpad->value_buffer[1]) * 10.0f + (digit - '0'));
		} else {
			numeric_value = (atof(numpad->value_buffer) * 10.0f + (digit - '0'));
		}

		// Store the actual numeric value in the buffer
		snprintf(numpad->value_buffer, numpad->buffer_size, "%.1f", numeric_value);
		numpad->current_length = strlen(numpad->value_buffer);

		// Format for display using the display formatting function
		numberpad_format_display_value(numeric_value, numpad->display_buffer, numpad->buffer_size);

		numpad->digit_count = 4; // Now at 4 digits
		printf("[I] numberpad: Fourth digit - numeric: '%s', display: '%s'\n", numpad->value_buffer, numpad->display_buffer);

		// Update the target field display
		update_target_field(numpad);
	} else if (numpad->digit_count == 4) {
		// Fifth digit: "150.0" → "1500.0" (multiply by 10 for thousands notation)
		// Calculate: "150.0" * 10 + digit = 1500.0
		float numeric_value;
		if (numpad->is_negative) {
			numeric_value = -(atof(&numpad->value_buffer[1]) * 10.0f + (digit - '0'));
		} else {
			numeric_value = (atof(numpad->value_buffer) * 10.0f + (digit - '0'));
		}

		// Store the actual numeric value in the buffer with bounds checking
		int written = snprintf(numpad->value_buffer, numpad->buffer_size, "%.1f", numeric_value);
		if (written >= numpad->buffer_size) {
			printf("[E] numberpad: Buffer overflow in value_buffer! Written: %d, Buffer size: %d\n", written, numpad->buffer_size);
			numpad->value_buffer[numpad->buffer_size - 1] = '\0';
		}
		numpad->current_length = strlen(numpad->value_buffer);

		// Format for display using the display formatting function
		numberpad_format_display_value(numeric_value, numpad->display_buffer, numpad->buffer_size);

		numpad->digit_count = 5; // Complete (5 digits with k notation)
		printf("[I] numberpad: Complete 5-digit number - numeric: '%s', display: '%s'\n", numpad->value_buffer, numpad->display_buffer);

		// Update the target field display
		printf("[D] numberpad: About to call update_target_field for 5th digit\n");
		update_target_field(numpad);
		printf("[D] numberpad: update_target_field completed for 5th digit\n");
	} else if (numpad->digit_count >= 5) {
		// Already at maximum (5 digits), clear and start fresh with new digit
		printf("[I] numberpad: Maximum digits reached, clearing and starting fresh with '%c'\n", digit);

		// Clear current value and start fresh
		numpad->value_buffer[0] = '\0';
		numpad->current_length = 0;
		numpad->is_first_digit = false;
		numpad->digit_count = 0;
		numpad->is_negative = false; // Reset negative flag

		// Start fresh with the new digit
		if (numpad->is_negative) {
			snprintf(numpad->value_buffer, numpad->buffer_size, "-0.%c", digit);
			numpad->current_length = 4;
		} else {
			snprintf(numpad->value_buffer, numpad->buffer_size, "0.%c", digit);
			numpad->current_length = 3;
		}
		numpad->digit_count = 1;
	}

		printf("[I] numberpad: Result: '%s'\n", numpad->value_buffer);

	// Update display buffer for regular formatting (not 5-digit k notation)
	if (numpad->digit_count < 5) {
		float numeric_value = atof(numpad->value_buffer);
		numberpad_format_display_value(numeric_value, numpad->display_buffer, numpad->buffer_size);
	}

	if (numpad->on_value_changed) {
		// Safety check: ensure callback is still valid
		printf("[D] numberpad: Calling on_value_changed callback\n");
		numpad->on_value_changed(numpad->value_buffer, numpad->user_data);
		printf("[D] numberpad: on_value_changed callback completed\n");
	}

	// Update target field display AFTER callback to ensure formatted display is shown
	printf("[D] numberpad: Updating target field display\n");
	update_target_field(numpad);
	printf("[D] numberpad: Target field display updated\n");
}

static void clear_value(numberpad_t* numpad) {
	if (!numpad) return;

	// Reset to empty
	numpad->value_buffer[0] = '\0';
	numpad->current_length = 0;
	numpad->is_first_digit = false; // Clear is not first digit
	numpad->digit_count = 0; // Reset digit count

	update_target_field(numpad);

	if (numpad->on_clear) {
		numpad->on_clear(numpad->user_data);
	}
}

static void enter_value(numberpad_t* numpad) {
	if (!numpad) return;

	if (numpad->on_enter) {
		numpad->on_enter(numpad->value_buffer, numpad->user_data);
	}

	numberpad_hide(numpad);
}

static void update_target_field(numberpad_t* numpad) {
	if (!numpad || !numpad->target_field) return;

	// Safety check: ensure target field is still valid
	if (!lv_obj_is_valid(numpad->target_field)) {
		printf("[W] numberpad: Target field is no longer valid, skipping update\n");
		return;
	}

	// Update the target field's label with current value
	lv_obj_t* label = lv_obj_get_child(numpad->target_field, 0);
	if (label && lv_obj_is_valid(label) && lv_obj_check_type(label, &lv_label_class)) {
		// Use display buffer if available (for k notation), otherwise use value buffer
		const char* display_text = (numpad->display_buffer && strlen(numpad->display_buffer) > 0)
			? numpad->display_buffer : numpad->value_buffer;

		// Safety check: ensure display text is valid
		if (display_text && strlen(display_text) > 0) {
			lv_label_set_text(label, display_text);
		}
	} else {
		printf("[W] numberpad: Label is invalid or not found, skipping update\n");
	}
}

// Toggle negative sign
static void toggle_negative(numberpad_t* numpad) {
	if (!numpad) return;

	numpad->is_negative = !numpad->is_negative;
	update_display_value(numpad);

	if (numpad->on_value_changed) {
		numpad->on_value_changed(numpad->value_buffer, numpad->user_data);
	}
}

// Reset negative state to positive (used when value triggers a clamp)
static void reset_negative_state(numberpad_t* numpad) {
	if (!numpad) return;

	numpad->is_negative = false;
	update_display_value(numpad);

	if (numpad->on_value_changed) {
		numpad->on_value_changed(numpad->value_buffer, numpad->user_data);
	}
}

// Public function to reset negative state
void numberpad_reset_negative_state(numberpad_t* numpad) {
	reset_negative_state(numpad);
}

// Reset negative state silently (without triggering value changed callback)
static void reset_negative_state_silent(numberpad_t* numpad) {
	if (!numpad) return;

	numpad->is_negative = false;
	update_display_value(numpad);
	// Don't call on_value_changed callback to avoid infinite loops
}

// Set value and prepare for fresh input (next digit will clear and start fresh)
static void set_value_for_fresh_input(numberpad_t* numpad, const char* value) {
	if (!numpad || !value) return;

	strncpy(numpad->value_buffer, value, numpad->buffer_size - 1);
	numpad->value_buffer[numpad->buffer_size - 1] = '\0';
	numpad->current_length = strlen(numpad->value_buffer);

	// Parse the value to determine digit count and format
	numpad->digit_count = 0;
	numpad->is_negative = (numpad->value_buffer[0] == '-');

	// Count digits (excluding decimal point, negative sign, and 'k' notation)
	for (int i = 0; i < numpad->current_length; i++) {
		if (numpad->value_buffer[i] >= '0' && numpad->value_buffer[i] <= '9') {
			numpad->digit_count++;
		}
	}

	// Check for 'k' notation
	bool has_k_notation = (numpad->value_buffer[numpad->current_length - 1] == 'k');

	// Determine if this is a decimal value or whole number
	// If it has a decimal point and ends with ".0", treat as whole number
	bool has_decimal = (strchr(numpad->value_buffer, '.') != NULL);
	bool ends_with_zero = (numpad->value_buffer[numpad->current_length - 1] == '0');

	if (has_k_notation) {
		// This is a k notation value like "1.5k" - keep as is
		printf("[I] numberpad: Setting k notation value: '%s'\n", numpad->value_buffer);
	} else if (has_decimal && ends_with_zero && numpad->digit_count >= 3) {
		// This is a whole number like "150.0" - convert to "150"
		char temp_buffer[16];
		int start = numpad->is_negative ? 1 : 0;
		int whole_digits = numpad->digit_count - 1; // Exclude the trailing zero

		if (numpad->is_negative) {
			snprintf(temp_buffer, sizeof(temp_buffer), "-%.*s", whole_digits, &numpad->value_buffer[1]);
		} else {
			snprintf(temp_buffer, sizeof(temp_buffer), "%.*s", whole_digits, numpad->value_buffer);
		}

		strncpy(numpad->value_buffer, temp_buffer, numpad->buffer_size - 1);
		numpad->value_buffer[numpad->buffer_size - 1] = '\0';
		numpad->current_length = strlen(numpad->value_buffer);
		numpad->digit_count = whole_digits;
	}

	// Set flag so next digit input will clear and start fresh
	numpad->is_first_digit = true;

	update_target_field(numpad);
}

// Public function to set value for fresh input
void numberpad_set_value_for_fresh_input(numberpad_t* numpad, const char* value) {
	set_value_for_fresh_input(numpad, value);
}

// Convert display value (with k notation) to numeric value
float numberpad_get_numeric_value(const char* display_value) {
	if (!display_value) return 0.0f;

	float value = atof(display_value);

	// Check if it ends with 'k' notation
	size_t len = strlen(display_value);
	if (len > 0 && display_value[len - 1] == 'k') {
		value *= 1000.0f; // Convert k to actual value
	}

	return value;
}

// Convert numeric value to display value (with k notation for values >= 1000)
void numberpad_format_display_value(float numeric_value, char* display_buffer, size_t buffer_size) {
	if (!display_buffer || buffer_size == 0) return;

	if (numeric_value >= 1000.0f) {
		// Format as k notation
		format_value_with_magnitude(numeric_value, display_buffer, buffer_size);
	} else if (numeric_value >= 100.0f && numeric_value < 1000.0f) {
		// Format as whole number for values 100-999
		snprintf(display_buffer, buffer_size, "%.0f", numeric_value);
	} else {
		// Format as regular number with decimal
		snprintf(display_buffer, buffer_size, "%.1f", numeric_value);
	}
}

// Cancel value and close numberpad
static void cancel_value(numberpad_t* numpad) {
	if (!numpad) return;

	if (numpad->on_cancel) {
		numpad->on_cancel(numpad->user_data);
	}

	numberpad_hide(numpad);
}

// Update display value with proper negative sign
static void update_display_value(numberpad_t* numpad) {
	if (!numpad) return;

	// Get the numeric value without sign
	const char* numeric_value = numpad->value_buffer;
	if (numeric_value[0] == '-') {
		numeric_value++; // Skip the minus sign
	}

	// Create new display value with proper sign
	char display_value[32];
	if (numpad->is_negative) {
		snprintf(display_value, sizeof(display_value), "-%s", numeric_value);
	} else {
		snprintf(display_value, sizeof(display_value), "%s", numeric_value);
	}

	// Update the buffer
	strncpy(numpad->value_buffer, display_value, numpad->buffer_size - 1);
	numpad->value_buffer[numpad->buffer_size - 1] = '\0';
	numpad->current_length = strlen(numpad->value_buffer);

	// Update the target field display
	update_target_field(numpad);
}

// Get numberpad position relative to target field
numberpad_position_t numberpad_get_position(numberpad_t* numpad) {
	return numpad ? numpad->position : NUMBERPAD_POSITION_OTHER;
}
