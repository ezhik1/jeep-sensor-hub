#include "time_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../../fonts/lv_font_noplato_24.h"
#include "../utils/positioning.h"
#include "../palette.h"

// Forward declarations
static void roller_changed_cb(lv_event_t* e);
static void preset_button_cb(lv_event_t* e);
static void background_click_cb(lv_event_t* e);
static void reset_preset_button_styles(time_input_t* time_input);
static void check_and_activate_preset(time_input_t* time_input);
static void set_time_input_smart_outside_container(time_input_t* time_input, lv_obj_t* target_field, lv_obj_t* container);

// Create time input component
time_input_t* time_input_create(const time_input_config_t* config, lv_obj_t* parent)
{
	printf("[I] time_input: Creating time input component\n");

	if (!config) {
		printf("[E] time_input: Configuration is required\n");
		return NULL;
	}

	time_input_t* time_input = malloc(sizeof(time_input_t));
	if (!time_input) {
		printf("[E] time_input: Failed to allocate memory for time input\n");
		return NULL;
	}

	memset(time_input, 0, sizeof(time_input_t));
	time_input->config = *config;

	// Create background container - appropriate width for 800px screen
	time_input->background = lv_obj_create(parent);
	lv_obj_set_size(time_input->background, 476, LV_SIZE_CONTENT); // positioned via smart_position_outside_container, so max width is screen width - border width 2px * 2 = 4px
	lv_obj_set_style_bg_color(time_input->background, PALETTE_BLACK, 0);
	lv_obj_set_style_bg_opa(time_input->background, LV_OPA_COVER, 0); // Use full opacity for better performance
	lv_obj_set_style_radius(time_input->background, 8, 0);
	lv_obj_set_style_border_color(time_input->background, PALETTE_WHITE, 0);
	lv_obj_set_style_border_width(time_input->background, 2, 0);
	lv_obj_set_style_pad_all(time_input->background, 10, 0);  // 5px padding all around
	lv_obj_clear_flag(time_input->background, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(time_input->background, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(time_input->background, background_click_cb, LV_EVENT_CLICKED, time_input);

	// Don't center - positioning will be handled by show function

	// Create content container
	time_input->content_container = lv_obj_create(time_input->background);
	lv_obj_set_size(time_input->content_container, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_layout(time_input->content_container, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(time_input->content_container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(time_input->content_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_bg_opa(time_input->content_container, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(time_input->content_container, 0, 0);
	lv_obj_set_style_pad_all(time_input->content_container, 0, 0);
	lv_obj_clear_flag(time_input->content_container, LV_OBJ_FLAG_SCROLLABLE);

	// Create preset buttons container (on top) - only for first 5 presets
	lv_obj_t* preset_container = lv_obj_create(time_input->content_container);
	lv_obj_set_size(preset_container, LV_PCT(100), 90);  // Increased height for more padding
	lv_obj_set_layout(preset_container, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(preset_container, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(preset_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_bg_opa(preset_container, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(preset_container, 0, 0);
	lv_obj_set_style_pad_all(preset_container, 0, 0);
	lv_obj_clear_flag(preset_container, LV_OBJ_FLAG_SCROLLABLE);

	// Create preset buttons (first 5 only)
	const char* preset_texts[] = {"30\nSECONDS", "1\nMINUTE", "30\nMINUTES", "1\nHOUR", "3\nHOURS"};
	const int preset_values[][3] = {
		{0, 0, 30},    // 30s
		{0, 1, 0},     // 1min
		{0, 30, 0},    // 30min
		{1, 0, 0},     // 1hr
		{3, 0, 0}      // 3hr
	};

	for (int i = 0; i < 5; i++) {
		time_input->preset_buttons[i] = lv_obj_create(preset_container);
		lv_obj_set_size(time_input->preset_buttons[i], 80, 80);  // 10px wider and 10px taller
		lv_obj_set_style_bg_color(time_input->preset_buttons[i], PALETTE_BLACK, 0);
		lv_obj_set_style_border_color(time_input->preset_buttons[i], PALETTE_YELLOW, 0);
		lv_obj_set_style_border_width(time_input->preset_buttons[i], 1, 0);
		lv_obj_set_style_radius(time_input->preset_buttons[i], 5, 0);
		lv_obj_add_flag(time_input->preset_buttons[i], LV_OBJ_FLAG_CLICKABLE);

		// Store preset values as user data
		int* preset_data = malloc(sizeof(int) * 3);
		preset_data[0] = preset_values[i][0]; // hours
		preset_data[1] = preset_values[i][1]; // minutes
		preset_data[2] = preset_values[i][2]; // seconds
		lv_obj_set_user_data(time_input->preset_buttons[i], preset_data);

		// Create button label
		lv_obj_t* btn_label = lv_label_create(time_input->preset_buttons[i]);
		lv_label_set_text(btn_label, preset_texts[i]);
		lv_obj_set_style_text_color(btn_label, PALETTE_YELLOW, 0);
		lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_14, 0);
		lv_obj_set_style_text_align(btn_label, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_center(btn_label);

		// Add event callback
		lv_obj_add_event_cb(time_input->preset_buttons[i], preset_button_cb, LV_EVENT_CLICKED, time_input);

		// Make preset buttons non-scrollable
		lv_obj_clear_flag(time_input->preset_buttons[i], LV_OBJ_FLAG_SCROLLABLE);
	}

	// Create rollers container with realtime button on the left
	lv_obj_t* rollers_container = lv_obj_create(time_input->content_container);
	lv_obj_set_size(rollers_container, LV_PCT(100), LV_SIZE_CONTENT);
	lv_obj_set_layout(rollers_container, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(rollers_container, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(rollers_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_bg_opa(rollers_container, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(rollers_container, 0, 0);
	lv_obj_set_style_pad_all(rollers_container, 0, 0);
	lv_obj_clear_flag(rollers_container, LV_OBJ_FLAG_SCROLLABLE);

	// Create realtime button on the left side of rollers
	time_input->preset_buttons[5] = lv_obj_create(rollers_container);
	lv_obj_set_size(time_input->preset_buttons[5], 100, 60);  // Wider and shorter for this layout
	lv_obj_set_style_bg_color(time_input->preset_buttons[5], PALETTE_BLACK, 0);
	lv_obj_set_style_border_color(time_input->preset_buttons[5], PALETTE_GREEN, 0);
	lv_obj_set_style_border_width(time_input->preset_buttons[5], 2, 0);
	lv_obj_set_style_radius(time_input->preset_buttons[5], 5, 0);
	lv_obj_add_flag(time_input->preset_buttons[5], LV_OBJ_FLAG_CLICKABLE);

	// Store realtime preset values as user data
	int* realtime_data = malloc(sizeof(int) * 3);
	realtime_data[0] = 0; // hours
	realtime_data[1] = 0; // minutes
	realtime_data[2] = 0; // seconds
	lv_obj_set_user_data(time_input->preset_buttons[5], realtime_data);

	// Create realtime button label
	lv_obj_t* realtime_label = lv_label_create(time_input->preset_buttons[5]);
	lv_label_set_text(realtime_label, "REALTIME");
	lv_obj_set_style_text_color(realtime_label, PALETTE_GREEN, 0);
	lv_obj_set_style_text_font(realtime_label, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_align(realtime_label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_center(realtime_label);

	// Add event callback
	lv_obj_add_event_cb(time_input->preset_buttons[5], preset_button_cb, LV_EVENT_CLICKED, time_input);

	// Make realtime button non-scrollable
	lv_obj_clear_flag(time_input->preset_buttons[5], LV_OBJ_FLAG_SCROLLABLE);

	// Create rollers for hours, minutes, seconds
	const char* label_texts[] = {"HOURS", "MINUTES", "SECONDS"};
	const char* roller_options[] = {
		"0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23",
		"0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59",
		"0\n1\n2\n3\n4\n5\n6\n7\n8\n9\n10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n50\n51\n52\n53\n54\n55\n56\n57\n58\n59"
	};

	for (int i = 0; i < 3; i++) {
		// Create roller group container
		lv_obj_t* roller_group = lv_obj_create(rollers_container);
		lv_obj_set_size(roller_group, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_set_layout(roller_group, LV_LAYOUT_FLEX);
		lv_obj_set_flex_flow(roller_group, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(roller_group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
		lv_obj_set_style_bg_opa(roller_group, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(roller_group, 0, 0);
		lv_obj_set_style_pad_all(roller_group, 0, 0);
		lv_obj_clear_flag(roller_group, LV_OBJ_FLAG_SCROLLABLE);

		// Create label
		time_input->labels[i] = lv_label_create(roller_group);
		lv_label_set_text(time_input->labels[i], label_texts[i]);
		lv_obj_set_size(time_input->labels[i], LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_set_style_text_color(time_input->labels[i], PALETTE_WHITE, 0);
		lv_obj_set_style_text_font(time_input->labels[i], &lv_font_montserrat_16, 0);
		lv_obj_set_style_bg_opa(time_input->labels[i], LV_OPA_TRANSP, 0);
		lv_obj_set_style_text_align(time_input->labels[i], LV_TEXT_ALIGN_CENTER, 0);

		// Create roller
		time_input->rollers[i] = lv_roller_create(roller_group);
		lv_obj_set_size(time_input->rollers[i], 80, 150);
		lv_roller_set_options(time_input->rollers[i], roller_options[i], LV_ROLLER_MODE_INFINITE);
		lv_obj_set_style_text_font(time_input->rollers[i], &lv_font_noplato_24, 0);
		lv_obj_clear_flag(time_input->rollers[i], LV_OBJ_FLAG_SCROLLABLE);
		lv_roller_set_visible_row_count(time_input->rollers[i], 3);
		lv_obj_set_style_anim_time(time_input->rollers[i], 1000, LV_PART_MAIN);
		lv_obj_set_style_text_align(time_input->rollers[i], LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);

		// Roller styling
		lv_obj_set_style_bg_color(time_input->rollers[i], PALETTE_BLACK, LV_PART_MAIN);
		lv_obj_set_style_bg_opa(time_input->rollers[i], LV_OPA_COVER, LV_PART_MAIN);
		lv_obj_set_style_border_color(time_input->rollers[i], PALETTE_DARK_GRAY, LV_PART_MAIN);
		lv_obj_set_style_border_width(time_input->rollers[i], 1, LV_PART_MAIN);

		// Selected/active number styling - try multiple parts
		lv_obj_set_style_bg_color(time_input->rollers[i], PALETTE_DARK_GRAY, LV_PART_INDICATOR);
		lv_obj_set_style_bg_opa(time_input->rollers[i], LV_OPA_COVER, LV_PART_INDICATOR);
		lv_obj_set_style_text_color(time_input->rollers[i], PALETTE_WHITE, LV_PART_INDICATOR);
		lv_obj_set_style_text_opa(time_input->rollers[i], LV_OPA_COVER, LV_PART_INDICATOR);  // Full opacity for active text

		// Also try LV_PART_SELECTED for the active item
		lv_obj_set_style_bg_color(time_input->rollers[i], PALETTE_DARK_GRAY, LV_PART_SELECTED);
		lv_obj_set_style_bg_opa(time_input->rollers[i], LV_OPA_COVER, LV_PART_SELECTED);
		lv_obj_set_style_text_color(time_input->rollers[i], PALETTE_WHITE, LV_PART_SELECTED);
		lv_obj_set_style_text_opa(time_input->rollers[i], LV_OPA_COVER, LV_PART_SELECTED);  // Full opacity for active text

		// Inactive numbers styling - make them dim
		lv_obj_set_style_text_color(time_input->rollers[i], PALETTE_GRAY, LV_PART_MAIN);
		lv_obj_set_style_text_opa(time_input->rollers[i], LV_OPA_40, LV_PART_MAIN);  // Make inactive numbers dim

		// Additional styling properties
		lv_obj_set_style_radius(time_input->rollers[i], 5, LV_PART_MAIN);
		lv_obj_set_style_radius(time_input->rollers[i], 5, LV_PART_INDICATOR);
		lv_obj_set_style_radius(time_input->rollers[i], 5, LV_PART_SELECTED);
		lv_obj_set_style_pad_all(time_input->rollers[i], 5, LV_PART_MAIN);

		lv_obj_set_style_pad_all(time_input->rollers[i], 0, 0);
		lv_obj_add_flag(time_input->rollers[i], LV_OBJ_FLAG_SNAPPABLE);

		// Add event callback
		lv_obj_add_event_cb(time_input->rollers[i], roller_changed_cb, LV_EVENT_VALUE_CHANGED, time_input);
	}

	// Initialize values with defaults
	time_input->hours = 1;
	time_input->minutes = 30;
	time_input->seconds = 0;

	// Set default values in rollers
	lv_roller_set_selected(time_input->rollers[0], 1, LV_ANIM_OFF);  // 1 hour
	lv_roller_set_selected(time_input->rollers[1], 30, LV_ANIM_OFF); // 30 minutes
	lv_roller_set_selected(time_input->rollers[2], 0, LV_ANIM_OFF);  // 0 seconds

	// Hide initially
	lv_obj_add_flag(time_input->background, LV_OBJ_FLAG_HIDDEN);
	time_input->is_visible = false;

	printf("[I] time_input: Time input component created successfully\n");
	return time_input;
}

// Destroy time input component
void time_input_destroy(time_input_t* time_input)
{
	if (!time_input) return;

	printf("[I] time_input: Destroying time input component\n");

	// Hide first
	time_input_hide(time_input);

	// Free preset button data
	for (int i = 0; i < 5; i++) {
		if (time_input->preset_buttons[i]) {
			int* preset_data = (int*)lv_obj_get_user_data(time_input->preset_buttons[i]);
			if (preset_data) {
				free(preset_data);
			}
		}
	}

	// Free memory
	free(time_input);
}

// Show time input positioned relative to target field
void time_input_show(time_input_t* time_input, lv_obj_t* target_field)
{
	if (!time_input || !target_field) return;

	time_input->target_field = target_field;
	lv_obj_clear_flag(time_input->background, LV_OBJ_FLAG_HIDDEN);
	time_input->is_visible = true;

	// Check if current values match a preset and activate it
	check_and_activate_preset(time_input);

	printf("[I] time_input: Time input shown\n");
}

// Show time input aligned to field but positioned outside container
void time_input_show_outside_container(time_input_t* time_input, lv_obj_t* target_field, lv_obj_t* container)
{
	if (!time_input || !target_field || !container) return;

	time_input->target_field = target_field;
	set_time_input_smart_outside_container(time_input, target_field, container);
	lv_obj_clear_flag(time_input->background, LV_OBJ_FLAG_HIDDEN);
	time_input->is_visible = true;

	// Check if current values match a preset and activate it
	check_and_activate_preset(time_input);

	printf("[I] time_input: Time input shown outside container\n");
}

// Hide time input
void time_input_hide(time_input_t* time_input)
{
	if (!time_input) return;

	lv_obj_add_flag(time_input->background, LV_OBJ_FLAG_HIDDEN);
	time_input->is_visible = false;
	time_input->target_field = NULL;

	printf("[I] time_input: Time input hidden\n");
}

// Set callbacks
void time_input_set_callbacks(
	time_input_t* time_input,
	void (*on_value_changed)(int hours, int minutes, int seconds, void* user_data),
	void (*on_enter)(int hours, int minutes, int seconds, void* user_data),
	void (*on_cancel)(void* user_data),
	void* user_data)
{
	if (!time_input) return;

	time_input->on_value_changed = on_value_changed;
	time_input->on_enter = on_enter;
	time_input->on_cancel = on_cancel;
	time_input->user_data = user_data;
}

// Set current time values
void time_input_set_values(time_input_t* time_input, int hours, int minutes, int seconds)
{
	if (!time_input) return;

	time_input->hours = hours;
	time_input->minutes = minutes;
	time_input->seconds = seconds;

	// Update rollers
	lv_roller_set_selected(time_input->rollers[0], hours, LV_ANIM_OFF);
	lv_roller_set_selected(time_input->rollers[1], minutes, LV_ANIM_OFF);
	lv_roller_set_selected(time_input->rollers[2], seconds, LV_ANIM_OFF);
}

// Get current time values
void time_input_get_values(time_input_t* time_input, int* hours, int* minutes, int* seconds)
{
	if (!time_input) return;

	if (hours) *hours = time_input->hours;
	if (minutes) *minutes = time_input->minutes;
	if (seconds) *seconds = time_input->seconds;
}

// Check if time input is visible
bool time_input_is_visible(time_input_t* time_input)
{
	return time_input && time_input->is_visible;
}

// Roller changed event handler
static void roller_changed_cb(lv_event_t* e)
{
	time_input_t* time_input = (time_input_t*)lv_event_get_user_data(e);
	if (!time_input) return;

	// Get values from all rollers
	time_input->hours = lv_roller_get_selected(time_input->rollers[0]);
	time_input->minutes = lv_roller_get_selected(time_input->rollers[1]);
	time_input->seconds = lv_roller_get_selected(time_input->rollers[2]);

	// Check if current values match a preset and update highlighting
	check_and_activate_preset(time_input);

	// Call value changed callback
	if (time_input->on_value_changed) {
		time_input->on_value_changed(time_input->hours, time_input->minutes, time_input->seconds, time_input->user_data);
	}
}

// Preset button clicked event handler
static void preset_button_cb(lv_event_t* e)
{
	time_input_t* time_input = (time_input_t*)lv_event_get_user_data(e);
	lv_obj_t* button = lv_event_get_target(e);
	if (!time_input || !button) return;

	// Reset all preset buttons to default styling first
	reset_preset_button_styles(time_input);

	// Get preset values from user data
	int* preset_data = (int*)lv_obj_get_user_data(button);
	if (!preset_data) return;

	// Set the time values
	int hours = preset_data[0];
	int minutes = preset_data[1];
	int seconds = preset_data[2];

	// Update internal values
	time_input->hours = hours;
	time_input->minutes = minutes;
	time_input->seconds = seconds;

	// Update rollers
	lv_roller_set_selected(time_input->rollers[0], hours, LV_ANIM_ON);
	lv_roller_set_selected(time_input->rollers[1], minutes, LV_ANIM_ON);
	lv_roller_set_selected(time_input->rollers[2], seconds, LV_ANIM_ON);

	// Set pressed button styling (yellow background, black text)
	lv_obj_set_style_bg_color(button, PALETTE_YELLOW, 0);
	lv_obj_set_style_text_color(button, PALETTE_BLACK, 0);

	// Special border styling for realtime button when selected
	if (button == time_input->preset_buttons[5]) { // realtime button
		lv_obj_set_style_border_color(button, PALETTE_YELLOW, 0);
	} else {
		lv_obj_set_style_border_color(button, PALETTE_YELLOW, 0);
	}

	// Update the label color
	lv_obj_t* label = lv_obj_get_child(button, 0);
	if (label) {
		lv_obj_set_style_text_color(label, PALETTE_BLACK, 0);
	}

	// Call value changed callback
	if (time_input->on_value_changed) {
		time_input->on_value_changed(hours, minutes, seconds, time_input->user_data);
	}

	printf("[I] time_input: Preset button clicked: %d:%02d:%02d\n", hours, minutes, seconds);
}

// Background click event handler - deactivates time input
static void background_click_cb(lv_event_t* e)
{
	time_input_t* time_input = (time_input_t*)lv_event_get_user_data(e);
	if (!time_input) return;

	// Only hide if clicking on the background itself (not on child elements)
	lv_obj_t* target = lv_event_get_target(e);
	if (target == time_input->background) {
		printf("[I] time_input: Background clicked, hiding time input\n");
		time_input_hide(time_input);
	}
}

// Reset all preset buttons to default styling
static void reset_preset_button_styles(time_input_t* time_input)
{
	for (int i = 0; i < 6; i++) {
		if (time_input->preset_buttons[i]) {
			// Reset to default styling
			lv_obj_set_style_bg_color(time_input->preset_buttons[i], PALETTE_BLACK, 0);

			// Special styling for realtime button
			if (i == 5) { // realtime button
				lv_obj_set_style_text_color(time_input->preset_buttons[i], PALETTE_GREEN, 0);
				lv_obj_set_style_border_color(time_input->preset_buttons[i], PALETTE_GREEN, 0);
			} else {
				lv_obj_set_style_text_color(time_input->preset_buttons[i], PALETTE_YELLOW, 0);
				lv_obj_set_style_border_color(time_input->preset_buttons[i], PALETTE_YELLOW, 0);
			}

			// Update the label color
			lv_obj_t* label = lv_obj_get_child(time_input->preset_buttons[i], 0);
			if (label) {
				if (i == 5) { // realtime button
					lv_obj_set_style_text_color(label, PALETTE_GREEN, 0);
				} else {
					lv_obj_set_style_text_color(label, PALETTE_YELLOW, 0);
				}
			}
		}
	}
}

// Check if current values match a preset and activate the appropriate button
static void check_and_activate_preset(time_input_t* time_input)
{
	if (!time_input) return;

	// Reset all buttons first
	reset_preset_button_styles(time_input);

	// Check each preset to see if it matches current values
	for (int i = 0; i < 6; i++) {
		if (time_input->preset_buttons[i]) {
			int* preset_data = (int*)lv_obj_get_user_data(time_input->preset_buttons[i]);
			if (preset_data) {
				// Check if current values match this preset
				if (time_input->hours == preset_data[0] &&
					time_input->minutes == preset_data[1] &&
					time_input->seconds == preset_data[2]) {

					// Activate this preset button
					lv_obj_set_style_bg_color(time_input->preset_buttons[i], PALETTE_YELLOW, 0);
					lv_obj_set_style_text_color(time_input->preset_buttons[i], PALETTE_BLACK, 0);

					// Set border color to yellow when selected (for all buttons including realtime)
					lv_obj_set_style_border_color(time_input->preset_buttons[i], PALETTE_YELLOW, 0);

					// Update the label color
					lv_obj_t* label = lv_obj_get_child(time_input->preset_buttons[i], 0);
					if (label) {
						lv_obj_set_style_text_color(label, PALETTE_BLACK, 0);
					}

					printf("[I] time_input: Preset %d activated for current values %d:%02d:%02d\n",
						   i, time_input->hours, time_input->minutes, time_input->seconds);
					break; // Only one preset can be active at a time
				}
			}
		}
	}
}

// Smart positioning using generic utility function
static void set_time_input_smart_outside_container(time_input_t* time_input, lv_obj_t* target_field, lv_obj_t* container)
{
	if (!time_input || !target_field || !container) return;

	// Use generic positioning algorithm
	smart_position_outside_container_default(time_input->background, target_field, container);
}

