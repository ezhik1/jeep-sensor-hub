#ifndef NUMBER_FORMATTING_H
#define NUMBER_FORMATTING_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Text alignment options
typedef enum {
	LABEL_ALIGN_LEFT,
	LABEL_ALIGN_CENTER,
	LABEL_ALIGN_RIGHT
} number_align_t;

// Number formatting configuration
typedef struct {
	lv_obj_t* label;           // LVGL label object to update
	const lv_font_t* font;     // Font to use
	lv_color_t color;          // Text color
	lv_color_t warning_color;  // Warning color (yellow)
	lv_color_t error_color;    // Error color (red)
	bool show_warning;         // Whether to show warning icon
	bool show_error;           // Whether to show error state
	lv_coord_t warning_icon_size; // Size of warning icon
	number_align_t number_alignment;  // Number text alignment (left, center, right)
	number_align_t warning_alignment; // Warning icon alignment (left, center, right)
} number_formatting_config_t;

// Format and display a number with smart decimal handling
// - Numbers < 100: display with 1 decimal place (e.g., "12.3")
// - Numbers >= 100: display as whole numbers (e.g., "123")
void format_and_display_number(float value, const number_formatting_config_t* config);

// Create a warning icon next to the label
void create_warning_icon(lv_obj_t* parent, lv_obj_t* label, lv_coord_t icon_size, number_align_t alignment);

// Hide warning icon
void hide_warning_icon(lv_obj_t* parent);

// Format values with magnitude suffixes (k for thousands, M for millions)
void format_value_with_magnitude(float value, char* buffer, size_t buffer_size);

// Generic alert flashing - applies color based on threshold and blink state
// Returns true if alert is active (value outside thresholds)
bool apply_alert_flashing(lv_obj_t* label, float value, float threshold_low, float threshold_high, bool blink_on);

#ifdef __cplusplus
}
#endif

#endif // NUMBER_FORMATTING_H
