#include "numberpad.h"
#include "../../../esp_compat.h"
#include <string.h>
#include <stdlib.h>

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
	.button_width = 60,
	.button_height = 50,
	.button_gap = 5
};

// Forward declarations
static void numberpad_button_cb(lv_event_t* e);
static void position_numberpad_smartly(numberpad_t* numpad, lv_obj_t* target_field);
static void update_target_field(numberpad_t* numpad);
static void add_digit(numberpad_t* numpad, char digit);
static void clear_value(numberpad_t* numpad);
static void enter_value(numberpad_t* numpad);

// Enhanced positioning algorithm - ensures no obscuring and always fits screen

numberpad_t* numberpad_create(const numberpad_config_t* config, lv_obj_t* parent) {
	if (!config || !parent) return NULL;

	numberpad_t* numpad = malloc(sizeof(numberpad_t));
	if (!numpad) return NULL;

	memset(numpad, 0, sizeof(numberpad_t));
	numpad->config = *config;
	numpad->buffer_size = config->max_digits + config->decimal_places + 2; // +2 for decimal point and null terminator
	numpad->value_buffer = malloc(numpad->buffer_size);
	if (!numpad->value_buffer) {
		free(numpad);
		return NULL;
	}
	numpad->value_buffer[0] = '\0';

	// Create background
	numpad->background = lv_obj_create(parent);
	lv_obj_set_style_bg_color(numpad->background, config->bg_color, 0);
	lv_obj_set_style_border_width(numpad->background, 0, 0); // Remove border
	lv_obj_set_style_radius(numpad->background, 3, 0);
	lv_obj_set_style_pad_all(numpad->background, 0, 0);
	lv_obj_clear_flag(numpad->background, LV_OBJ_FLAG_SCROLLABLE);

	// Calculate numberpad size - CLEAR button is 2 units wide
	lv_coord_t numpad_width = (config->button_width * 3) + (config->button_gap * 2);
	lv_coord_t numpad_height = (config->button_height * 4) + (config->button_gap * 3);
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
		lv_obj_set_style_border_width(numpad->buttons[i], 0, 0); // Remove border
		lv_obj_set_style_radius(numpad->buttons[i], 3, 0);

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
	lv_obj_set_style_border_width(numpad->buttons[9], 0, 0); // Remove border
	lv_obj_set_style_radius(numpad->buttons[9], 3, 0);

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
	lv_obj_set_style_border_width(numpad->buttons[10], 0, 0); // Remove border
	lv_obj_set_style_radius(numpad->buttons[10], 3, 0);

	lv_obj_t* clear_label = lv_label_create(numpad->buttons[10]);
	lv_label_set_text(clear_label, config->clear_text);
	lv_obj_set_style_text_color(clear_label, config->text_color, 0);
	lv_obj_set_style_text_font(clear_label, &lv_font_montserrat_16, 0);
	lv_obj_center(clear_label);
	lv_obj_add_event_cb(numpad->buttons[10], numberpad_button_cb, LV_EVENT_CLICKED, numpad);

	// No ENTER button - save on close

	// Initially hidden
	lv_obj_add_flag(numpad->background, LV_OBJ_FLAG_HIDDEN);
	numpad->is_visible = false;

	ESP_LOGI(TAG, "Numberpad created with size %dx%d", numpad_width, numpad_height);
	return numpad;
}

void numberpad_destroy(numberpad_t* numpad) {
	if (!numpad) return;

	if (numpad->value_buffer) {
		free(numpad->value_buffer);
	}

	if (numpad->background) {
		lv_obj_del(numpad->background);
	}

	free(numpad);
}

void numberpad_show(numberpad_t* numpad, lv_obj_t* target_field) {
	if (!numpad || !target_field) return;

	numpad->target_field = target_field;
	position_numberpad_smartly(numpad, target_field);
	lv_obj_clear_flag(numpad->background, LV_OBJ_FLAG_HIDDEN);
	numpad->is_visible = true;
	numpad->is_first_digit = true; // First digit input will clear current value

	ESP_LOGI(TAG, "Numberpad shown for target field");
}

void numberpad_show_with_first_digit_flag(numberpad_t* numpad, lv_obj_t* target_field, bool is_first_digit) {
	if (!numpad || !target_field) return;

	numpad->target_field = target_field;
	position_numberpad_smartly(numpad, target_field);
	lv_obj_clear_flag(numpad->background, LV_OBJ_FLAG_HIDDEN);
	numpad->is_visible = true;
	numpad->is_first_digit = is_first_digit; // Use provided flag

	ESP_LOGI(TAG, "Numberpad shown for target field (first_digit=%s)", is_first_digit ? "true" : "false");
}

void numberpad_hide(numberpad_t* numpad) {
	if (!numpad) return;

	lv_obj_add_flag(numpad->background, LV_OBJ_FLAG_HIDDEN);
	numpad->is_visible = false;
	numpad->target_field = NULL;

	// Reset state when hiding
	numpad->is_first_digit = false;
	numpad->digit_count = 0;

	ESP_LOGI(TAG, "Numberpad hidden");
}

void numberpad_set_callbacks(numberpad_t* numpad,
							void (*on_value_changed)(const char* value, void* user_data),
							void (*on_clear)(void* user_data),
							void (*on_enter)(const char* value, void* user_data),
							void* user_data) {
	if (!numpad) return;

	numpad->on_value_changed = on_value_changed;
	numpad->on_clear = on_clear;
	numpad->on_enter = on_enter;
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
static void position_numberpad_smartly(numberpad_t* numpad, lv_obj_t* target_field) {
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
		ESP_LOGI(TAG, "Positioned numberpad below field");
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
			ESP_LOGI(TAG, "Positioned numberpad above field");
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
			ESP_LOGI(TAG, "Positioned numberpad to the right of field");
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
			ESP_LOGI(TAG, "Positioned numberpad to the left of field");
		}
	}

	// Strategy 5: Position in available space (avoiding field area)
	if (!found_position) {
		// Try top-left area
		best_x = screen_margin;
		best_y = screen_margin;

		if (best_x + pad_width <= field_coords.x1 - min_gap ||
			best_y + pad_height <= field_coords.y1 - min_gap) {
			found_position = true;
			ESP_LOGI(TAG, "Positioned numberpad in top-left area");
		}
	}

	// Strategy 6: Position in top-right area
	if (!found_position) {
		best_x = screen_width - pad_width - screen_margin;
		best_y = screen_margin;

		if (best_x >= field_coords.x2 + min_gap ||
			best_y + pad_height <= field_coords.y1 - min_gap) {
			found_position = true;
			ESP_LOGI(TAG, "Positioned numberpad in top-right area");
		}
	}

	// Strategy 7: Position in bottom-left area
	if (!found_position) {
		best_x = screen_margin;
		best_y = screen_height - pad_height - screen_margin;

		if (best_x + pad_width <= field_coords.x1 - min_gap ||
			best_y >= field_coords.y2 + min_gap) {
			found_position = true;
			ESP_LOGI(TAG, "Positioned numberpad in bottom-left area");
		}
	}

	// Strategy 8: Position in bottom-right area
	if (!found_position) {
		best_x = screen_width - pad_width - screen_margin;
		best_y = screen_height - pad_height - screen_margin;

		if (best_x >= field_coords.x2 + min_gap ||
			best_y >= field_coords.y2 + min_gap) {
			found_position = true;
			ESP_LOGI(TAG, "Positioned numberpad in bottom-right area");
		}
	}

	// Strategy 9: Center positioning as last resort
	if (!found_position) {
		best_x = (screen_width - pad_width) / 2;
		best_y = (screen_height - pad_height) / 2;
		ESP_LOGI(TAG, "Positioned numberpad at center (fallback)");
	}

	// Ensure final position is within screen bounds
	if (best_x < screen_margin) best_x = screen_margin;
	if (best_x + pad_width > screen_width - screen_margin) best_x = screen_width - pad_width - screen_margin;
	if (best_y < screen_margin) best_y = screen_margin;
	if (best_y + pad_height > screen_height - screen_margin) best_y = screen_height - pad_height - screen_margin;

	// Apply the position
	lv_obj_set_pos(numpad->background, best_x, best_y);

	ESP_LOGI(TAG, "Numberpad positioned at (%d, %d) for field at (%d, %d)",
			 best_x, best_y, field_coords.x1, field_coords.y1);
}

static void numberpad_button_cb(lv_event_t* e) {
	lv_obj_t* btn = lv_event_get_target(e);
	numberpad_t* numpad = lv_event_get_user_data(e);

	if (!numpad) return;

	// Find which button was clicked
	for (int i = 0; i < 11; i++) {
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
			}
			break;
		}
	}
}

static void add_digit(numberpad_t* numpad, char digit) {
	if (!numpad) return;

	ESP_LOGI(TAG, "add_digit: digit='%c', is_first_digit=%s, current='%s'",
			 digit, numpad->is_first_digit ? "true" : "false", numpad->value_buffer);

	// If this is the first digit input, clear current value and start fresh
	if (numpad->is_first_digit) {
		ESP_LOGI(TAG, "First digit: clearing current value and starting fresh");
		// Clear current value and start fresh
		numpad->value_buffer[0] = '\0';
		numpad->current_length = 0;
		numpad->is_first_digit = false;
		numpad->digit_count = 0;
	}

	// Just add digits and place decimal to left of rightmost
	if (numpad->current_length == 0) {
		// First digit: start with "0.X"
		snprintf(numpad->value_buffer, numpad->buffer_size, "0.%c", digit);
		numpad->current_length = 3;
		numpad->digit_count = 1;
	} else if (numpad->current_length == 3 && numpad->digit_count == 1) {
		// Second digit: "0.2" → "2.3" (remove leading zero and add new digit)
		snprintf(numpad->value_buffer, numpad->buffer_size, "%c.%c", numpad->value_buffer[2], digit);
		numpad->current_length = 3;
		numpad->digit_count = 2;
	} else if (numpad->current_length == 3 && numpad->digit_count == 2) {
		// Third digit: "2.3" → "23.4" (add new digit to left side)
		snprintf(numpad->value_buffer, numpad->buffer_size, "%c%c.%c", numpad->value_buffer[0], numpad->value_buffer[2], digit);
		numpad->current_length = 5;
		numpad->digit_count = 3;
	} else if (numpad->current_length == 5) {
		// After 3 digits, next input should start over and populate first number
		// Start over and populate first number as "0.X"
		snprintf(numpad->value_buffer, numpad->buffer_size, "0.%c", digit);
		numpad->current_length = 3;
		numpad->digit_count = 1;
		ESP_LOGI(TAG, "Started over and populated first number: '%s'", numpad->value_buffer);
	}

	ESP_LOGI(TAG, "Result: '%s'", numpad->value_buffer);

	update_target_field(numpad);

	if (numpad->on_value_changed) {
		numpad->on_value_changed(numpad->value_buffer, numpad->user_data);
	}
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

	// Update the target field's label with current value
	lv_obj_t* label = lv_obj_get_child(numpad->target_field, 0);
	if (label && lv_obj_check_type(label, &lv_label_class)) {
		lv_label_set_text(label, numpad->value_buffer);
	}
}
