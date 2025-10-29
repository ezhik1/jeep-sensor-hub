#include "number_formatting.h"
#include "../warning_icon/warning_icon.h"
#include "../../palette.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static const char* TAG = "number_formatting";

void format_and_display_number(float value, const number_formatting_config_t* config)
{
	if (!config || !config->label) return;

	// Safety check: ensure the label is still valid
	if (!lv_obj_is_valid(config->label)) {
		printf("[W] number_formatting: Label is not valid, skipping format\n");
		return;
	}

	// Get the parent container (this is the value label itself)
	lv_obj_t* value_label = config->label;

	// Check if this is a detail screen sensor data row (has flexbox with SPACE_BETWEEN)
	bool is_detail_screen_row = false;
	lv_obj_t* row_container = lv_obj_get_parent(value_label);
	if (row_container && lv_obj_is_valid(row_container) && lv_obj_get_style_flex_flow(row_container, 0) == LV_FLEX_FLOW_ROW) {
		lv_flex_align_t main_align = lv_obj_get_style_flex_main_place(row_container, 0);
		if (main_align == LV_FLEX_ALIGN_SPACE_BETWEEN) {
			is_detail_screen_row = true;
		}
	}

	if (is_detail_screen_row && row_container && lv_obj_is_valid(row_container)) {
		// For detail screen rows, create a fixed-size container for the value area
		// This container will be right-aligned within the flexbox
		lv_obj_t* value_container = lv_obj_create(row_container);
		lv_obj_set_size(value_container, 75, 30); // Fixed size - twice as wide for better fit
		lv_obj_set_style_bg_opa(value_container, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(value_container, PALETTE_BLACK, 0);
		lv_obj_set_style_border_width(value_container, 0, 0);
		lv_obj_set_style_pad_all(value_container, 0, 0);
		lv_obj_clear_flag(value_container, LV_OBJ_FLAG_SCROLLABLE);

		// Move the value label into the new container
		lv_obj_set_parent(value_label, value_container);

		// Force layout update to ensure container size is applied
		lv_obj_update_layout(value_container);
		lv_obj_update_layout(row_container);

		// Apply alignment based on number alignment configuration
		switch (config->number_alignment) {
			case LABEL_ALIGN_LEFT:
				lv_obj_align(value_label, LV_ALIGN_LEFT_MID, 0, 0);
				break;
			case LABEL_ALIGN_CENTER:
				lv_obj_align(value_label, LV_ALIGN_CENTER, 0, 0);
				break;
			case LABEL_ALIGN_RIGHT:
			default:
				lv_obj_align(value_label, LV_ALIGN_RIGHT_MID, 0, 0);
				break;
		}

		// Update the parent reference
		row_container = value_container;
	} else {
		// For other contexts, use the existing parent but don't resize it
		// Align the label within its existing container based on config
		row_container = lv_obj_get_parent(value_label);

		// Apply alignment based on number alignment configuration
		switch (config->number_alignment) {
			case LABEL_ALIGN_LEFT:
				lv_obj_align(value_label, LV_ALIGN_LEFT_MID, 0, 0);
				break;
			case LABEL_ALIGN_CENTER:
				lv_obj_align(value_label, LV_ALIGN_CENTER, 0, 0);
				break;
			case LABEL_ALIGN_RIGHT:
			default:
				lv_obj_align(value_label, LV_ALIGN_RIGHT_MID, 0, 0);
				break;
		}
	}

	// Handle error state - show ONLY the warning icon
	if (config->show_error) {
		// Hide the label text when there's an error
		lv_obj_add_flag(value_label, LV_OBJ_FLAG_HIDDEN);

		// Create error icon in the same container as the value label
		// For detail screen rows, this will be the value_container we created
		// For other contexts, this will be the original parent
		lv_obj_t* value_container = lv_obj_get_parent(value_label);
		create_warning_icon(value_container, value_label, config->warning_icon_size, config->warning_alignment);

		// For non-detail-screen contexts (like power grid view), position the icon
		// at the same location as the value label

		lv_obj_t* warning_icon = lv_obj_get_child(value_container, lv_obj_get_child_cnt(value_container) - 1);
		if (warning_icon && lv_obj_has_flag(warning_icon, LV_OBJ_FLAG_USER_1)) {
			// Apply alignment based on warning alignment configuration
			switch (config->warning_alignment) {
				case LABEL_ALIGN_LEFT:
					lv_obj_align(warning_icon, LV_ALIGN_LEFT_MID, 0, 0);
					break;
				case LABEL_ALIGN_CENTER:
					lv_obj_align(warning_icon, LV_ALIGN_CENTER, 0, 0);
					break;
				case LABEL_ALIGN_RIGHT:
				default:
					lv_obj_align(warning_icon, LV_ALIGN_RIGHT_MID, 0, 0);
					break;
			}
		}

		return;
	}

	// Make sure label is visible (in case it was hidden due to previous error)
	lv_obj_clear_flag(value_label, LV_OBJ_FLAG_HIDDEN);

	// Hide any existing warning icon when showing numbers
	lv_obj_t* value_container = lv_obj_get_parent(value_label);
	hide_warning_icon(value_container);

	// Format the number with thousands handling
	char number_text[32];
	char suffix_text[8] = "";
	bool has_suffix = false;

	// Use the same formatting logic as numberpad to avoid crashes
	// Handle both positive and negative numbers with the same rules
	if (value >= 1000.0f || value <= -1000.0f) {
		// Format as k notation (both positive and negative)
		format_value_with_magnitude(value, number_text, sizeof(number_text));
	} else if ((value >= 100.0f && value < 1000.0f) || (value <= -100.0f && value > -1000.0f)) {
		// Format as whole number for values 100-999 and -100 to -999
		snprintf(number_text, sizeof(number_text), "%.0f", value);
	} else {
		// Format as regular number with decimal
		snprintf(number_text, sizeof(number_text), "%.1f", value);
	}

	// Update main label text
	lv_label_set_text(value_label, number_text);

	// No suffix handling needed since 'k' is included directly in the text

	// Apply font if provided, otherwise use monospace font
	if (config->font) {
		lv_obj_set_style_text_font(value_label, config->font, 0);
	} else {
		// Use monospace font for consistent character width
		lv_obj_set_style_text_font(value_label, &lv_font_montserrat_16, 0);
	}

	// Set text alignment based on number alignment configuration
	switch (config->number_alignment) {
		case LABEL_ALIGN_LEFT:
			lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_LEFT, 0);
			break;
		case LABEL_ALIGN_CENTER:
			lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_CENTER, 0);
			break;
		case LABEL_ALIGN_RIGHT:
		default:
			lv_obj_set_style_text_align(value_label, LV_TEXT_ALIGN_RIGHT, 0);
			break;
	}

	// Apply color (use warning color if warning is enabled)
	lv_color_t text_color = config->show_warning ? config->warning_color : config->color;
	lv_obj_set_style_text_color(value_label, text_color, 0);
}

void create_warning_icon(lv_obj_t* parent, lv_obj_t* label, lv_coord_t icon_size, number_align_t alignment)
{
	if (!parent || !label) return;

	// Safety check: ensure objects are still valid
	if (!lv_obj_is_valid(parent) || !lv_obj_is_valid(label)) {
		printf("[W] number_formatting: Parent or label is not valid, skipping warning icon\n");
		return;
	}

	// Check if warning icon already exists
	uint32_t child_count = lv_obj_get_child_cnt(parent);
	for (uint32_t i = 0; i < child_count; i++) {
		lv_obj_t* child = lv_obj_get_child(parent, i);
		if (child && lv_obj_has_flag(child, LV_OBJ_FLAG_USER_1)) {
			// Warning icon already exists, just show it
			lv_obj_clear_flag(child, LV_OBJ_FLAG_HIDDEN);
			return;
		}
	}

	// Get appropriate icon size
	warning_icon_size_t icon_size_enum = warning_icon_get_size_from_coord(icon_size);

	// Create warning icon using bitmap
	lv_obj_t* warning_icon = warning_icon_create(parent, icon_size_enum, PALETTE_YELLOW);
	// Note: alignment is handled by the caller after creation to ensure correct parent context
}

void hide_warning_icon(lv_obj_t* parent)
{
	if (!parent) return;

	// Safety check: ensure parent is still valid
	if (!lv_obj_is_valid(parent)) {
		printf("[W] number_formatting: Parent is not valid, skipping hide warning icon\n");
		return;
	}

	// Find and DELETE any existing warning icons
	uint32_t child_count = lv_obj_get_child_cnt(parent);
	for (int i = child_count - 1; i >= 0; i--) {
		lv_obj_t* child = lv_obj_get_child(parent, i);
		if (child && lv_obj_is_valid(child) && lv_obj_has_flag(child, LV_OBJ_FLAG_USER_1)) {
			lv_obj_del(child);
			// Continue to delete ALL warning icons to prevent duplicates
		}
	}
}

// Format values with magnitude suffixes (k for thousands, M for millions)
void format_value_with_magnitude(float value, char* buffer, size_t buffer_size)
{
	if (fabsf(value) >= 1000000.0f) {
		// Format with 'M' suffix for millions
		snprintf(buffer, buffer_size, "%.1fm", value / 1000000.0f);
	} else if (fabsf(value) > 999.0f) {
		// Format with 'k' suffix for thousands
		snprintf(buffer, buffer_size, "%.1fk", value / 1000.0f);
	} else {
		// Format as whole number for values <= 999
		snprintf(buffer, buffer_size, "%.0f", value);
	}
}

// Generic alert flashing - applies color based on threshold and blink state
bool apply_alert_flashing(lv_obj_t* label, float value, float threshold_low, float threshold_high, bool blink_on)
{
	if (!label || !lv_obj_is_valid(label)) return false;

	// Check if value is outside thresholds
	bool alert = (value <= threshold_low) || (value >= threshold_high);

	if (alert) {
		// Only change color, no hiding/showing to prevent layout shifts
		if (blink_on) {
			lv_obj_set_style_text_color(label, PALETTE_YELLOW, 0);
		} else {
			lv_obj_set_style_text_color(label, PALETTE_WHITE, 0);
		}
	} else {
		// Normal state - always white
		lv_obj_set_style_text_color(label, PALETTE_WHITE, 0);
	}

	return alert;
}
