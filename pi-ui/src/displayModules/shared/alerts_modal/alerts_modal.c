#include <stdio.h>
#include "alerts_modal.h"

#include "../../../state/device_state.h"
#include "../../../fonts/lv_font_noplato_24.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "alerts_modal";

// Battery names and field names for UI
static const char* battery_names[] = {"STARTER (V)", "HOUSE (V)", "SOLAR (W)"};
static const char* field_names[] = {"LOW", "HIGH", "LOW", "BASE", "HIGH"};
static const char* group_names[] = {"ALERTS", "GAUGE"};

// Default values for each field [battery][field]
static const float default_values[BATTERY_COUNT][FIELD_COUNT_PER_BATTERY] = {
	// STARTER: alert_low, alert_high, gauge_low, gauge_baseline, gauge_high
	{10.0f, 15.0f, 10.0f, 13.0f, 15.0f},
	// HOUSE: alert_low, alert_high, gauge_low, gauge_baseline, gauge_high
	{10.0f, 15.0f, 10.0f, 13.0f, 15.0f},
	// SOLAR: alert_low, alert_high, gauge_low, gauge_baseline, gauge_high
	{10.0f, 15.0f, 10.0f, 0.0f, 15.0f}  // Solar has no baseline
};

// Field validation ranges (min, max, default)
static const float field_ranges[FIELD_COUNT_PER_BATTERY][3] = {
	{0.0f, 20.0f, 10.0f},  // FIELD_ALERT_LOW: 0-20V, default 10V
	{0.0f, 20.0f, 15.0f},  // FIELD_ALERT_HIGH: 0-20V, default 15V
	{0.0f, 20.0f, 10.0f},  // FIELD_GAUGE_LOW: 0-20V, default 10V
	{0.0f, 20.0f, 13.0f},  // FIELD_GAUGE_BASELINE: 0-20V, default 13V
	{0.0f, 20.0f, 15.0f}   // FIELD_GAUGE_HIGH: 0-20V, default 15V
};

// Forward declarations
static void close_button_cb(lv_event_t *e);
static void field_button_cb(lv_event_t *e);
static void reset_current_field(alerts_modal_t* modal);
static void create_battery_section(alerts_modal_t* modal, battery_type_t battery, lv_obj_t* parent, int y_offset);
static void update_field_display(alerts_modal_t* modal, battery_type_t battery, field_type_t field);
static void load_values_from_device_state(alerts_modal_t* modal);
static void save_values_to_device_state(alerts_modal_t* modal);
static void highlight_current_field(alerts_modal_t* modal, battery_type_t battery, field_type_t field);
static void clear_field_highlights(alerts_modal_t* modal);
static void validate_and_apply_value_with_feedback(alerts_modal_t* modal, battery_type_t battery, field_type_t field, float value);
static void revert_border_color_timer(lv_timer_t* timer);

// Numberpad callbacks
static void numberpad_value_changed(const char* value, void* user_data);
static void numberpad_clear(void* user_data);
static void numberpad_enter(const char* value, void* user_data);

// Close button callback
static void close_button_cb(lv_event_t *e)
{
	alerts_modal_t* modal = (alerts_modal_t*)lv_event_get_user_data(e);
	if (!modal) return;

	// Hide numberpad and clear highlights if visible
	if (modal->numberpad && numberpad_is_visible(modal->numberpad)) {
		numberpad_hide(modal->numberpad);
		clear_field_highlights(modal);
		modal->current_battery = -1;
		modal->current_field = -1;
	}

	// Save values before closing
	save_values_to_device_state(modal);

	// Call the close callback (this will handle hide and destroy)
	if (modal->on_close) {
		modal->on_close();
	}
}

// Tap outside numberpad callback - save value but don't close modal
static void tap_outside_numberpad_cb(lv_event_t *e)
{
	alerts_modal_t* modal = (alerts_modal_t*)lv_event_get_user_data(e);
	if (!modal) return;

	// Only handle if numberpad is visible
	if (modal->numberpad && numberpad_is_visible(modal->numberpad)) {
		// Save current value
		save_values_to_device_state(modal);

		// Hide numberpad and clear highlights
		numberpad_hide(modal->numberpad);
		clear_field_highlights(modal);
		alerts_modal_restore_colors(modal);
		modal->current_battery = -1;
		modal->current_field = -1;

		printf("[I] alerts_modal: Value saved from tap outside numberpad\n");
	}
}

// Field button callback
static void reset_current_field(alerts_modal_t* modal)
{
	if (!modal) return;

	// Reset current field to invalid state
	modal->current_battery = -1;
	modal->current_field = -1;

	// Clear any highlights
	clear_field_highlights(modal);

	printf("[I] alerts_modal: Current field reset\n");
}

static void field_button_cb(lv_event_t *e)
{
	alerts_modal_t* modal = (alerts_modal_t*)lv_event_get_user_data(e);
	lv_obj_t* button = lv_event_get_target(e);

	if (!modal || !button || !modal->numberpad) return;

	// Find which field was clicked
	for (int battery = 0; battery < BATTERY_COUNT; battery++) {
		for (int field = 0; field < FIELD_COUNT_PER_BATTERY; field++) {
			if (modal->field_buttons[battery][field] == button) {
				printf("[I] alerts_modal: Field clicked: battery=%d, field=%d\n", battery, field);

				// If switching to a different field, restore colors first
				if (modal->current_battery != -1 && modal->current_field != -1 &&
					(modal->current_battery != battery || modal->current_field != field)) {
					printf("[I] alerts_modal: Switching fields - restoring colors first\n");
					alerts_modal_restore_colors(modal);
				}

				// Reset current field to prevent crosstalk
				reset_current_field(modal);

				// Highlight current field
				highlight_current_field(modal, battery, field);

				// Set current field
				modal->current_battery = battery;
				modal->current_field = field;

				// Check if this field has been opened before
				bool is_first_open = !modal->field_opened_before[battery][field];
				modal->field_opened_before[battery][field] = true; // Mark as opened

				// Update the field display to show current value
				update_field_display(modal, battery, field);

				// Dim all elements except the target field for focus
				alerts_modal_dim_for_focus(modal, battery, field);

				// Show numberpad with first digit flag (don't set value, let numberpad start fresh)
				numberpad_show_with_first_digit_flag(modal->numberpad, button, is_first_open);
				return;
			}
		}
	}
}

// Create battery section
static void create_battery_section(alerts_modal_t* modal, battery_type_t battery, lv_obj_t* parent, int y_offset)
{
	if (!modal || battery >= BATTERY_COUNT) return;

	// Create battery section container - much taller and wider to eliminate scroll bars
	modal->battery_sections[battery] = lv_obj_create(parent);
	lv_obj_set_size(modal->battery_sections[battery], LV_PCT(100), 200);  // Increased from 160 to 200
	lv_obj_set_pos(modal->battery_sections[battery], 0, y_offset);
	lv_obj_set_style_bg_opa(modal->battery_sections[battery], LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(modal->battery_sections[battery], 1, 0);
	lv_obj_set_style_border_color(modal->battery_sections[battery], lv_color_hex(0xffffff), 0);
	lv_obj_set_style_pad_all(modal->battery_sections[battery], 1, 0);  // Minimal padding for maximum space

	// Battery title - positioned inline with border like detail_screen
	// Create as child of root modal container so it's not clipped by battery_sections
	lv_obj_t* title = lv_label_create(modal->content_container);
	lv_label_set_text(title, battery_names[battery]);
	lv_obj_set_style_text_color(title, lv_color_hex(0xffffff), 0);
	lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
	lv_obj_set_style_bg_color(title, lv_color_hex(0x000000), 0); // Black background to obscure border
	lv_obj_set_style_bg_opa(title, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_left(title, 8, 0);
	lv_obj_set_style_pad_right(title, 8, 0);
	lv_obj_set_style_pad_top(title, 2, 0);
	lv_obj_set_style_pad_bottom(title, 2, 0);
	lv_obj_align_to(title, modal->battery_sections[battery], LV_ALIGN_OUT_TOP_LEFT, 10, 10);

	// Create ALERTS group - 2 units wide using flexbox, balanced margins
	modal->alert_groups[battery] = lv_obj_create(modal->battery_sections[battery]);
	lv_obj_set_size(modal->alert_groups[battery], LV_PCT(38), 140);  // Reduced width to make room for GAUGE
	lv_obj_set_pos(modal->alert_groups[battery], 8, 32);  // 8px left margin (A), reduced y by 3px
	lv_obj_set_style_bg_opa(modal->alert_groups[battery], LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(modal->alert_groups[battery], 1, 0);
	lv_obj_set_style_border_color(modal->alert_groups[battery], lv_color_hex(0xffffff), 0);
	lv_obj_set_style_radius(modal->alert_groups[battery], 5, 0);
	lv_obj_set_style_pad_all(modal->alert_groups[battery], 0, 0);  // No internal padding for maximum space
	// Setup flexbox layout for 2-item alerts
	lv_obj_set_layout(modal->alert_groups[battery], LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(modal->alert_groups[battery], LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(modal->alert_groups[battery], LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// ALERTS group title - positioned inline with border like detail_screen
	lv_obj_t* alerts_title = lv_label_create(modal->battery_sections[battery]);
	lv_label_set_text(alerts_title, "ALERTS");
	lv_obj_set_style_text_color(alerts_title, lv_color_hex(0xffffff), 0);
	lv_obj_set_style_text_font(alerts_title, &lv_font_montserrat_12, 0);
	lv_obj_set_style_bg_color(alerts_title, lv_color_hex(0x000000), 0); // Black background to obscure border
	lv_obj_set_style_bg_opa(alerts_title, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_left(alerts_title, 8, 0);
	lv_obj_set_style_pad_right(alerts_title, 8, 0);
	lv_obj_set_style_pad_top(alerts_title, 2, 0);
	lv_obj_set_style_pad_bottom(alerts_title, 2, 0);
	lv_obj_align_to(alerts_title, modal->alert_groups[battery], LV_ALIGN_OUT_TOP_RIGHT, -10, 10);

	// Create GAUGE group - positioned absolutely to use all available space
	modal->gauge_groups[battery] = lv_obj_create(modal->battery_sections[battery]);
	// Position from right edge with 8px margin, width calculated to fill remaining space
	lv_obj_set_size(modal->gauge_groups[battery], LV_PCT(56), 140);  // Even wider to reduce gap
	lv_obj_align(modal->gauge_groups[battery], LV_ALIGN_TOP_RIGHT, -8, 32);  // 8px from right edge, reduced y by 3px
	lv_obj_set_style_bg_opa(modal->gauge_groups[battery], LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(modal->gauge_groups[battery], 1, 0);
	lv_obj_set_style_border_color(modal->gauge_groups[battery], lv_color_hex(0xffffff), 0);
	lv_obj_set_style_radius(modal->gauge_groups[battery], 5, 0);
	lv_obj_set_style_pad_all(modal->gauge_groups[battery], 0, 0);  // No internal padding for maximum space
	lv_obj_clear_flag(modal->gauge_groups[battery], LV_OBJ_FLAG_SCROLLABLE);  // Ensure no scroll bars
	// Setup flexbox layout for 3-item gauge (maintains alignment when items missing)
	lv_obj_set_layout(modal->gauge_groups[battery], LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(modal->gauge_groups[battery], LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(modal->gauge_groups[battery], LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// GAUGE group title - positioned inline with border like detail_screen
	lv_obj_t* gauge_title = lv_label_create(modal->battery_sections[battery]);
	lv_label_set_text(gauge_title, "GAUGE");
	lv_obj_set_style_text_color(gauge_title, lv_color_hex(0xffffff), 0);
	lv_obj_set_style_text_font(gauge_title, &lv_font_montserrat_12, 0);
	lv_obj_set_style_bg_color(gauge_title, lv_color_hex(0x000000), 0); // Black background to obscure border
	lv_obj_set_style_bg_opa(gauge_title, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_left(gauge_title, 8, 0);
	lv_obj_set_style_pad_right(gauge_title, 8, 0);
	lv_obj_set_style_pad_top(gauge_title, 2, 0);
	lv_obj_set_style_pad_bottom(gauge_title, 2, 0);
	lv_obj_align_to(gauge_title, modal->gauge_groups[battery], LV_ALIGN_OUT_TOP_RIGHT, -10, 10);

	// Create field buttons for ALERTS group (LOW, HIGH) - flexbox layout
	for (int i = 0; i < 2; i++) {
		// Create container for button + label - reduced padding for tighter spacing
		lv_obj_t* field_container = lv_obj_create(modal->alert_groups[battery]);
		lv_obj_set_size(field_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_set_style_bg_opa(field_container, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(field_container, 0, 0);
		lv_obj_set_style_pad_all(field_container, 1, 0);  // Reduced from 2 to 1
		lv_obj_clear_flag(field_container, LV_OBJ_FLAG_SCROLLABLE);  // Clear scrollable flag

		modal->field_buttons[battery][i] = lv_button_create(field_container);
		lv_obj_set_size(modal->field_buttons[battery][i], 60, 60);  // Square buttons
		lv_obj_set_style_bg_color(modal->field_buttons[battery][i], lv_color_hex(0x000000), 0);
		lv_obj_set_style_border_color(modal->field_buttons[battery][i], lv_color_hex(0xffffff), 0);
		lv_obj_set_style_border_width(modal->field_buttons[battery][i], 1, 0);
		lv_obj_set_style_radius(modal->field_buttons[battery][i], 5, 0);

		lv_obj_t* label = lv_label_create(modal->field_buttons[battery][i]);
		lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
		lv_obj_set_style_text_font(label, &lv_font_noplato_24, 0);  // Monospace font
		lv_obj_center(label);

		lv_obj_add_event_cb(modal->field_buttons[battery][i], field_button_cb, LV_EVENT_CLICKED, modal);

		// Update layout to ensure field button positions are calculated
		lv_obj_update_layout(field_container);
		lv_obj_update_layout(modal->field_buttons[battery][i]);

		// Field name label - aligned to field button (which has the border)
		lv_obj_t* name_label = lv_label_create(field_container);
		lv_label_set_text(name_label, field_names[i]);
		lv_obj_set_style_text_color(name_label, lv_color_hex(0xffffff), 0);  // White text
		lv_obj_set_style_text_font(name_label, &lv_font_montserrat_12, 0);
		lv_obj_set_style_bg_color(name_label, lv_color_hex(0x000000), 0); // Black background to obscure border
		lv_obj_set_style_bg_opa(name_label, LV_OPA_COVER, 0);
		lv_obj_set_style_pad_left(name_label, 8, 0);
		lv_obj_set_style_pad_right(name_label, 8, 0);
		lv_obj_set_style_pad_top(name_label, 2, 0);
		lv_obj_set_style_pad_bottom(name_label, 2, 0);
		lv_obj_align_to(name_label, modal->field_buttons[battery][i], LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
	}

	// Create field buttons for GAUGE group (LOW, BASE-LINE, HIGH) - always create 3 containers for alignment
	// For SOLAR: create 3 containers with blank field in middle position
	for (int i = 0; i < 3; i++) {
		int field_index = i + 2; // Start from FIELD_GAUGE_LOW
		bool should_create_field = true;

		// For SOLAR: skip the middle field (BASE-LINE), create blank instead
		if (battery == BATTERY_SOLAR && i == 1) {
			should_create_field = false; // Create blank field for solar
		}

		// For SOLAR: map position 2 to HIGH field (field_index 4)
		if (battery == BATTERY_SOLAR && i == 2) {
			field_index = 4; // HIGH field for solar
		}

		// Create container for button + label - reduced padding for tighter spacing
		lv_obj_t* field_container = lv_obj_create(modal->gauge_groups[battery]);
		lv_obj_set_size(field_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_set_style_bg_opa(field_container, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(field_container, 0, 0);
		lv_obj_set_style_pad_all(field_container, 1, 0);  // Reduced from 2 to 1
		lv_obj_clear_flag(field_container, LV_OBJ_FLAG_SCROLLABLE);  // Clear scrollable flag

		if (should_create_field) {
			// Create active field button
			modal->field_buttons[battery][field_index] = lv_button_create(field_container);
			lv_obj_set_size(modal->field_buttons[battery][field_index], 60, 60);  // Square buttons
			lv_obj_set_style_bg_color(modal->field_buttons[battery][field_index], lv_color_hex(0x000000), 0);
			lv_obj_set_style_border_color(modal->field_buttons[battery][field_index], lv_color_hex(0xffffff), 0);
			lv_obj_set_style_border_width(modal->field_buttons[battery][field_index], 1, 0);
			lv_obj_set_style_radius(modal->field_buttons[battery][field_index], 5, 0);

			lv_obj_t* label = lv_label_create(modal->field_buttons[battery][field_index]);
			lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
			lv_obj_set_style_text_font(label, &lv_font_noplato_24, 0);  // Monospace font
			lv_obj_center(label);

			lv_obj_add_event_cb(modal->field_buttons[battery][field_index], field_button_cb, LV_EVENT_CLICKED, modal);

			// Update layout to ensure field button positions are calculated
			lv_obj_update_layout(field_container);
			lv_obj_update_layout(modal->field_buttons[battery][field_index]);

			// Field name label - aligned to field button (which has the border)
			lv_obj_t* name_label = lv_label_create(field_container);
			lv_label_set_text(name_label, field_names[field_index]);
			lv_obj_set_style_text_color(name_label, lv_color_hex(0xffffff), 0);  // White text
			lv_obj_set_style_text_font(name_label, &lv_font_montserrat_12, 0);
			lv_obj_set_style_bg_color(name_label, lv_color_hex(0x000000), 0); // Black background to obscure border
			lv_obj_set_style_bg_opa(name_label, LV_OPA_COVER, 0);
			lv_obj_set_style_pad_left(name_label, 8, 0);
			lv_obj_set_style_pad_right(name_label, 8, 0);
			lv_obj_set_style_pad_top(name_label, 2, 0);
			lv_obj_set_style_pad_bottom(name_label, 2, 0);
			lv_obj_align_to(name_label, modal->field_buttons[battery][field_index], LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
		} else {
			// Create blank field for alignment (empty container)
			lv_obj_set_size(field_container, 60, 60);  // Same size as field buttons
			// No button or label created - just empty space for alignment
		}
	}
}

// Update field display
static void update_field_display(alerts_modal_t* modal, battery_type_t battery, field_type_t field)
{
	if (!modal || battery >= BATTERY_COUNT || field >= FIELD_COUNT_PER_BATTERY) return;

	lv_obj_t* field_button = modal->field_buttons[battery][field];
	if (!field_button) return;

	lv_obj_t* label = lv_obj_get_child(field_button, 0);
	if (label && lv_obj_check_type(label, &lv_label_class)) {
		char value_str[16];
		snprintf(value_str, sizeof(value_str), "%.1f", modal->field_values[battery][field]);
		lv_label_set_text(label, value_str);
	}
}

// Load values from device state
static void load_values_from_device_state(alerts_modal_t* modal)
{
	if (!modal) return;

	// Load alert thresholds from device state
	modal->field_values[BATTERY_STARTER][FIELD_ALERT_LOW] = (float)device_state_get_starter_alert_low_voltage_v();
	modal->field_values[BATTERY_STARTER][FIELD_ALERT_HIGH] = (float)device_state_get_starter_alert_high_voltage_v();
	modal->field_values[BATTERY_HOUSE][FIELD_ALERT_LOW] = (float)device_state_get_house_alert_low_voltage_v();
	modal->field_values[BATTERY_HOUSE][FIELD_ALERT_HIGH] = (float)device_state_get_house_alert_high_voltage_v();
	modal->field_values[BATTERY_SOLAR][FIELD_ALERT_LOW] = (float)device_state_get_solar_alert_low_voltage_v();
	modal->field_values[BATTERY_SOLAR][FIELD_ALERT_HIGH] = (float)device_state_get_solar_alert_high_voltage_v();

	// Load gauge ranges from device state (convert from tenths to volts)
	modal->field_values[BATTERY_STARTER][FIELD_GAUGE_LOW] = device_state_get_starter_min_voltage_v();
	modal->field_values[BATTERY_STARTER][FIELD_GAUGE_BASELINE] = device_state_get_starter_baseline_voltage_v();
	modal->field_values[BATTERY_STARTER][FIELD_GAUGE_HIGH] = device_state_get_starter_max_voltage_v();
	modal->field_values[BATTERY_HOUSE][FIELD_GAUGE_LOW] = device_state_get_house_min_voltage_v();
	modal->field_values[BATTERY_HOUSE][FIELD_GAUGE_BASELINE] = device_state_get_house_baseline_voltage_v();
	modal->field_values[BATTERY_HOUSE][FIELD_GAUGE_HIGH] = device_state_get_house_max_voltage_v();
	modal->field_values[BATTERY_SOLAR][FIELD_GAUGE_LOW] = device_state_get_solar_min_voltage_v();
	modal->field_values[BATTERY_SOLAR][FIELD_GAUGE_BASELINE] = 0.0f; // Solar has no baseline
	modal->field_values[BATTERY_SOLAR][FIELD_GAUGE_HIGH] = device_state_get_solar_max_voltage_v();

	// Update all field displays
	for (int battery = 0; battery < BATTERY_COUNT; battery++) {
		for (int field = 0; field < FIELD_COUNT_PER_BATTERY; field++) {
			if (modal->field_buttons[battery][field]) {
				update_field_display(modal, battery, field);
			}
		}
	}

	printf("[I] alerts_modal: Loaded threshold values from device state\n");
}

// Save values to device state
static void save_values_to_device_state(alerts_modal_t* modal)
{
	if (!modal) return;

	printf("[I] alerts_modal: Saving threshold values to device state\n");

	// Save alert thresholds to device state
	device_state_set_starter_alert_low_voltage_v((int)modal->field_values[BATTERY_STARTER][FIELD_ALERT_LOW]);
	device_state_set_starter_alert_high_voltage_v((int)modal->field_values[BATTERY_STARTER][FIELD_ALERT_HIGH]);
	device_state_set_house_alert_low_voltage_v((int)modal->field_values[BATTERY_HOUSE][FIELD_ALERT_LOW]);
	device_state_set_house_alert_high_voltage_v((int)modal->field_values[BATTERY_HOUSE][FIELD_ALERT_HIGH]);
	device_state_set_solar_alert_low_voltage_v((int)modal->field_values[BATTERY_SOLAR][FIELD_ALERT_LOW]);
	device_state_set_solar_alert_high_voltage_v((int)modal->field_values[BATTERY_SOLAR][FIELD_ALERT_HIGH]);

	// Save gauge ranges to device state (convert to tenths for precision)
	device_state_set_starter_min_voltage_v(modal->field_values[BATTERY_STARTER][FIELD_GAUGE_LOW]);
	device_state_set_starter_baseline_voltage_v(modal->field_values[BATTERY_STARTER][FIELD_GAUGE_BASELINE]);
	device_state_set_starter_max_voltage_v(modal->field_values[BATTERY_STARTER][FIELD_GAUGE_HIGH]);
	device_state_set_house_min_voltage_v(modal->field_values[BATTERY_HOUSE][FIELD_GAUGE_LOW]);
	device_state_set_house_baseline_voltage_v(modal->field_values[BATTERY_HOUSE][FIELD_GAUGE_BASELINE]);
	device_state_set_house_max_voltage_v(modal->field_values[BATTERY_HOUSE][FIELD_GAUGE_HIGH]);
	device_state_set_solar_min_voltage_v(modal->field_values[BATTERY_SOLAR][FIELD_GAUGE_LOW]);
	// Solar baseline is not saved (always 0.0)
	device_state_set_solar_max_voltage_v(modal->field_values[BATTERY_SOLAR][FIELD_GAUGE_HIGH]);

	// Trigger device state save
	device_state_save();

	// Log the saved values
	for (int battery = 0; battery < BATTERY_COUNT; battery++) {
		printf("[I] alerts_modal: %s values:\n", battery_names[battery]);
		for (int field = 0; field < FIELD_COUNT_PER_BATTERY; field++) {
			printf("[I] alerts_modal:   %s: %.1f\n", field_names[field], modal->field_values[battery][field]);
		}
	}

	printf("[I] alerts_modal: Threshold values saved to device state\n");
}

// Highlight current field
static void highlight_current_field(alerts_modal_t* modal, battery_type_t battery, field_type_t field)
{
	if (!modal || battery >= BATTERY_COUNT || field >= FIELD_COUNT_PER_BATTERY) return;

	lv_obj_t* button = modal->field_buttons[battery][field];
	if (!button) return;

	// Set cyan border for the current field
	lv_obj_set_style_border_color(button, lv_color_hex(0x00ffff), 0);
	lv_obj_set_style_border_width(button, 2, 0);
}

// Clear field highlights
static void clear_field_highlights(alerts_modal_t* modal)
{
	if (!modal) return;

	// Reset all field buttons to white border
	for (int battery = 0; battery < BATTERY_COUNT; battery++) {
		for (int field = 0; field < FIELD_COUNT_PER_BATTERY; field++) {
			if (modal->field_buttons[battery][field]) {
				lv_obj_set_style_border_color(modal->field_buttons[battery][field], lv_color_hex(0xffffff), 0);
				lv_obj_set_style_border_width(modal->field_buttons[battery][field], 1, 0);
			}
		}
	}
}

// Numberpad callbacks
static void numberpad_value_changed(const char* value, void* user_data)
{
	alerts_modal_t* modal = (alerts_modal_t*)user_data;
	if (!modal || modal->current_battery < 0 || modal->current_field < 0) return;

	// Parse the value and validate with visual feedback
	float new_value = atof(value);
	validate_and_apply_value_with_feedback(modal, modal->current_battery, modal->current_field, new_value);
}

static void numberpad_clear(void* user_data)
{
	alerts_modal_t* modal = (alerts_modal_t*)user_data;
	if (!modal || modal->current_battery < 0 || modal->current_field < 0) return;

	// Set value to 0
	modal->field_values[modal->current_battery][modal->current_field] = 0.0f;

	// Update display
	update_field_display(modal, modal->current_battery, modal->current_field);
}

// No numberpad_enter callback needed - save on close

// Public API functions
alerts_modal_t* alerts_modal_create(void (*on_close_callback)(void))
{
	printf("[I] alerts_modal: Creating enhanced alerts modal\n");

	alerts_modal_t* modal = malloc(sizeof(alerts_modal_t));
	if (!modal) {
		printf("[E] alerts_modal: Failed to allocate memory for alerts modal\n");
		return NULL;
	}

	memset(modal, 0, sizeof(alerts_modal_t));
	modal->on_close = on_close_callback;
	modal->current_battery = -1;
	modal->current_field = -1;

	// Create modal background - truly full screen, no padding
	modal->background = lv_obj_create(lv_screen_active());
	lv_obj_set_size(modal->background, LV_PCT(100), LV_PCT(100));
	lv_obj_set_pos(modal->background, 0, 0);
	lv_obj_set_style_bg_color(modal->background, lv_color_hex(0x000000), 0);
	lv_obj_set_style_bg_opa(modal->background, LV_OPA_80, 0);
	lv_obj_set_style_border_width(modal->background, 0, 0);
	lv_obj_set_style_pad_all(modal->background, 0, 0);

	// Create content container - truly full width, no padding from background
	modal->content_container = lv_obj_create(modal->background);
	lv_obj_set_size(modal->content_container, LV_PCT(100), LV_PCT(100));
	lv_obj_align(modal->content_container, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_bg_color(modal->content_container, lv_color_hex(0x000000), 0);
	lv_obj_set_style_border_color(modal->content_container, lv_color_hex(0xffffff), 0);
	lv_obj_set_style_border_width(modal->content_container, 1, 0);
	lv_obj_set_style_radius(modal->content_container, 10, 0);

	// Create main container without scrolling - fit all content with proper spacing
	modal->scroll_container = lv_obj_create(modal->content_container);
	lv_obj_set_size(modal->scroll_container, LV_PCT(100), 740);  // Fixed height for 3 sections: 3*240 + margin
	lv_obj_align(modal->scroll_container, LV_ALIGN_TOP_MID, 0, 25);
	lv_obj_set_style_bg_opa(modal->scroll_container, LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(modal->scroll_container, 0, 0);
	lv_obj_set_style_pad_all(modal->scroll_container, 0, 0);  // No padding for maximum space
	lv_obj_clear_flag(modal->scroll_container, LV_OBJ_FLAG_SCROLLABLE);

	// Create title
	modal->title_label = lv_label_create(modal->content_container);
	lv_label_set_text(modal->title_label, "THRESHOLDS");
	lv_obj_set_style_text_color(modal->title_label, lv_color_hex(0xffffff), 0);
	lv_obj_set_style_text_font(modal->title_label, &lv_font_montserrat_20, 0);
	lv_obj_align(modal->title_label, LV_ALIGN_TOP_RIGHT, -10, -6);

	// Create battery sections with proper spacing to prevent overlap
	for (int i = 0; i < BATTERY_COUNT; i++) {
		create_battery_section(modal, i, modal->scroll_container, i * 240);  // Increased for better visual separation
	}

	// Create close button
	modal->close_button = lv_button_create(modal->content_container);
	lv_obj_set_size(modal->close_button, 100, 40);
	lv_obj_align(modal->close_button, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
	lv_obj_set_style_bg_color(modal->close_button, lv_color_hex(0x555555), 0);

	lv_obj_t *close_label = lv_label_create(modal->close_button);
	lv_label_set_text(close_label, "Close");
	lv_obj_set_style_text_color(close_label, lv_color_hex(0xffffff), 0);
	lv_obj_center(close_label);

	// Add click event to close button
	lv_obj_add_event_cb(modal->close_button, close_button_cb, LV_EVENT_CLICKED, modal);

	// Add tap-outside-to-save event handlers
	lv_obj_add_event_cb(modal->background, tap_outside_numberpad_cb, LV_EVENT_CLICKED, modal);
	lv_obj_add_event_cb(modal->content_container, tap_outside_numberpad_cb, LV_EVENT_CLICKED, modal);
	lv_obj_add_event_cb(modal->scroll_container, tap_outside_numberpad_cb, LV_EVENT_CLICKED, modal);

	// Create shared numberpad component
	numberpad_config_t numpad_config = NUMBERPAD_DEFAULT_CONFIG;
	numpad_config.max_digits = 4;
	numpad_config.decimal_places = 1;
	numpad_config.auto_decimal = true;
	modal->numberpad = numberpad_create(&numpad_config, modal->background);

	if (modal->numberpad) {
		numberpad_set_callbacks(modal->numberpad,
							   numberpad_value_changed,
							   numberpad_clear,
							   NULL, // No enter callback - save on close
							   modal);
	}

	// Load values and update displays
	load_values_from_device_state(modal);

	// Initially hidden
	lv_obj_add_flag(modal->background, LV_OBJ_FLAG_HIDDEN);
	modal->is_visible = false;

	printf("[I] alerts_modal: Enhanced alerts modal created\n");
	return modal;
}

void alerts_modal_show(alerts_modal_t* modal)
{
	if (!modal) {
		printf("[W] alerts_modal: Cannot show NULL modal\n");
		return;
	}

	if (!modal->is_visible) {
		printf("[I] alerts_modal: Showing enhanced alerts modal\n");
		lv_obj_remove_flag(modal->background, LV_OBJ_FLAG_HIDDEN);
		modal->is_visible = true;
	}
}

void alerts_modal_hide(alerts_modal_t* modal)
{
	if (!modal) {
		printf("[W] alerts_modal: Cannot hide NULL modal\n");
		return;
	}

	if (modal->is_visible) {
		printf("[I] alerts_modal: Hiding enhanced alerts modal\n");
		lv_obj_add_flag(modal->background, LV_OBJ_FLAG_HIDDEN);
		modal->is_visible = false;

		// Also hide numberpad if visible
		if (modal->numberpad && numberpad_is_visible(modal->numberpad)) {
			numberpad_hide(modal->numberpad);
			alerts_modal_restore_colors(modal);
			reset_current_field(modal);
		}
	}
}

void alerts_modal_destroy(alerts_modal_t* modal)
{
	if (!modal) {
		printf("[W] alerts_modal: Cannot destroy NULL modal\n");
		return;
	}

	printf("[I] alerts_modal: Destroying enhanced alerts modal\n");

	if (modal->numberpad) {
		numberpad_destroy(modal->numberpad);
	}

	if (modal->background) {
		lv_obj_del_async(modal->background);
	}

	free(modal);
}

bool alerts_modal_is_visible(alerts_modal_t* modal)
{
	if (!modal) {
		return false;
	}
	return modal->is_visible;
}

void alerts_modal_show_numberpad(alerts_modal_t* modal, battery_type_t battery, field_type_t field)
{
	if (!modal || battery >= BATTERY_COUNT || field >= FIELD_COUNT_PER_BATTERY) {
		return;
	}

	modal->current_battery = battery;
	modal->current_field = field;

	// Clear any existing highlights and highlight current field
	clear_field_highlights(modal);
	highlight_current_field(modal, battery, field);

	// Set initial value in numberpad
	char value_str[16];
	snprintf(value_str, sizeof(value_str), "%.1f", modal->field_values[battery][field]);
	numberpad_set_value(modal->numberpad, value_str);

	// Show numberpad
	numberpad_show(modal->numberpad, modal->field_buttons[battery][field]);

		printf("[I] alerts_modal: Showing numberpad for %s %s\n", battery_names[battery], field_names[field]);
}

void alerts_modal_hide_numberpad(alerts_modal_t* modal)
{
	if (!modal) return;

	if (modal->numberpad) {
		numberpad_hide(modal->numberpad);
	}

	// Restore colors and reset current field
	alerts_modal_restore_colors(modal);
	reset_current_field(modal);
}

float alerts_modal_get_field_value(alerts_modal_t* modal, battery_type_t battery, field_type_t field)
{
	if (!modal || battery >= BATTERY_COUNT || field >= FIELD_COUNT_PER_BATTERY) {
		return 0.0f;
	}
	return modal->field_values[battery][field];
}

void alerts_modal_set_field_value(alerts_modal_t* modal, battery_type_t battery, field_type_t field, float value)
{
	if (!modal || battery >= BATTERY_COUNT || field >= FIELD_COUNT_PER_BATTERY) {
		return;
	}

	modal->field_values[battery][field] = value;
	update_field_display(modal, battery, field);
}

void alerts_modal_dim_for_focus(alerts_modal_t* modal, battery_type_t battery, field_type_t field)
{
	if (!modal) return;

	// Dim the title
	lv_obj_set_style_text_color(modal->title_label, lv_color_hex(0x404040), 0);

	// Dim all battery sections
	for (int b = 0; b < BATTERY_COUNT; b++) {
		// Dim battery section borders (except the one containing the target field)
		if (b != battery) {
			lv_obj_set_style_border_color(modal->battery_sections[b], lv_color_hex(0x404040), 0);
		}

		// Dim battery section titles
		lv_obj_t* battery_title = lv_obj_get_child(modal->battery_sections[b], 0);
		if (battery_title) {
			lv_obj_set_style_text_color(battery_title, lv_color_hex(0x404040), 0);
		}

		// Dim alert and gauge group borders (except the ones in the target battery section)
		if (b != battery) {
			lv_obj_set_style_border_color(modal->alert_groups[b], lv_color_hex(0x404040), 0);
			lv_obj_set_style_border_color(modal->gauge_groups[b], lv_color_hex(0x404040), 0);
		} else {
			// In the target battery section, dim the group that doesn't contain the target field
			// Determine which group contains the target field
			bool target_in_alerts = (field == FIELD_ALERT_LOW || field == FIELD_ALERT_HIGH);
			bool target_in_gauge = (field == FIELD_GAUGE_LOW || field == FIELD_GAUGE_BASELINE || field == FIELD_GAUGE_HIGH);

			if (target_in_alerts) {
				// Target is in alerts group, dim gauge group
				lv_obj_set_style_border_color(modal->gauge_groups[b], lv_color_hex(0x404040), 0);
			} else if (target_in_gauge) {
				// Target is in gauge group, dim alerts group
				lv_obj_set_style_border_color(modal->alert_groups[b], lv_color_hex(0x404040), 0);
			}
		}

		// Dim alert and gauge group titles
		if (b != battery) {
			// Dim titles in other battery sections
			lv_obj_t* alerts_title = lv_obj_get_child(modal->alert_groups[b], 0);
			if (alerts_title) {
				lv_obj_set_style_text_color(alerts_title, lv_color_hex(0x404040), 0);
			}

			lv_obj_t* gauge_title = lv_obj_get_child(modal->gauge_groups[b], 0);
			if (gauge_title) {
				lv_obj_set_style_text_color(gauge_title, lv_color_hex(0x404040), 0);
			}
		} else {
			// In the target battery section, dim the title of the group that doesn't contain the target field
			bool target_in_alerts = (field == FIELD_ALERT_LOW || field == FIELD_ALERT_HIGH);
			bool target_in_gauge = (field == FIELD_GAUGE_LOW || field == FIELD_GAUGE_BASELINE || field == FIELD_GAUGE_HIGH);

			if (target_in_alerts) {
				// Target is in alerts group, dim gauge group title
				lv_obj_t* gauge_title = lv_obj_get_child(modal->gauge_groups[b], 0);
				if (gauge_title) {
					lv_obj_set_style_text_color(gauge_title, lv_color_hex(0x404040), 0);
				}
			} else if (target_in_gauge) {
				// Target is in gauge group, dim alerts group title
				lv_obj_t* alerts_title = lv_obj_get_child(modal->alert_groups[b], 0);
				if (alerts_title) {
					lv_obj_set_style_text_color(alerts_title, lv_color_hex(0x404040), 0);
				}
			}
		}

		// Dim all field buttons except the target field
		for (int f = 0; f < FIELD_COUNT_PER_BATTERY; f++) {
			if (b == battery && f == field) {
				// Keep target field bright - don't dim it
				continue;
			}

			lv_obj_t* field_button = modal->field_buttons[b][f];
			if (field_button) {
				// Dim button border
				lv_obj_set_style_border_color(field_button, lv_color_hex(0x404040), 0);

				// Dim button text
				lv_obj_t* label = lv_obj_get_child(field_button, 0);
				if (label) {
					lv_obj_set_style_text_color(label, lv_color_hex(0x404040), 0);
				}
			}

			// Dim field labels (LOW, HIGH, etc.)
			lv_obj_t* field_label = lv_obj_get_child(modal->battery_sections[b], 10 + f); // Assuming labels are after buttons
			if (field_label) {
				lv_obj_set_style_text_color(field_label, lv_color_hex(0x404040), 0);
			}
		}
	}

		printf("[I] alerts_modal: Modal dimmed for focus on battery=%d, field=%d\n", battery, field);
}

void alerts_modal_restore_colors(alerts_modal_t* modal)
{
	if (!modal) return;

	// Restore title
	lv_obj_set_style_text_color(modal->title_label, lv_color_hex(0xffffff), 0);

	// Restore all battery sections
	for (int b = 0; b < BATTERY_COUNT; b++) {
		// Restore battery section borders
		lv_obj_set_style_border_color(modal->battery_sections[b], lv_color_hex(0xffffff), 0);

		// Restore battery section titles
		lv_obj_t* battery_title = lv_obj_get_child(modal->battery_sections[b], 0);
		if (battery_title) {
			lv_obj_set_style_text_color(battery_title, lv_color_hex(0xffffff), 0);
		}

		// Restore alert and gauge group borders
		lv_obj_set_style_border_color(modal->alert_groups[b], lv_color_hex(0xffffff), 0);
		lv_obj_set_style_border_color(modal->gauge_groups[b], lv_color_hex(0xffffff), 0);

		// Restore alert and gauge group titles
		lv_obj_t* alerts_title = lv_obj_get_child(modal->alert_groups[b], 0);
		if (alerts_title) {
			lv_obj_set_style_text_color(alerts_title, lv_color_hex(0xffffff), 0);
		}

		lv_obj_t* gauge_title = lv_obj_get_child(modal->gauge_groups[b], 0);
		if (gauge_title) {
			lv_obj_set_style_text_color(gauge_title, lv_color_hex(0xffffff), 0);
		}

		// Restore all field buttons
		for (int f = 0; f < FIELD_COUNT_PER_BATTERY; f++) {
			lv_obj_t* field_button = modal->field_buttons[b][f];
			if (field_button) {
				// Restore button border
				lv_obj_set_style_border_color(field_button, lv_color_hex(0xffffff), 0);

				// Restore button text
				lv_obj_t* label = lv_obj_get_child(field_button, 0);
				if (label) {
					lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
				}
			}

			// Restore field labels (LOW, HIGH, etc.)
			lv_obj_t* field_label = lv_obj_get_child(modal->battery_sections[b], 10 + f); // Assuming labels are after buttons
			if (field_label) {
				lv_obj_set_style_text_color(field_label, lv_color_hex(0xffffff), 0);
			}
		}
	}

	printf("[I] alerts_modal: Modal colors restored\n");
}

// Task data structure for border revert
typedef struct {
	alerts_modal_t* modal;
	int battery;
	int field;
} border_revert_data_t;

static void revert_border_color_timer(lv_timer_t* timer)
{
	border_revert_data_t* data = (border_revert_data_t*)lv_timer_get_user_data(timer);
	if (!data || !data->modal) {
		lv_timer_delete(timer);
		return;
	}

	lv_obj_t* field_button = data->modal->field_buttons[data->battery][data->field];
	if (field_button) {
		lv_obj_set_style_border_color(field_button, lv_color_hex(0xffffff), 0);
		lv_obj_set_style_border_width(field_button, 1, 0);
	}

	free(data);
	lv_timer_delete(timer);
}

static void validate_and_apply_value_with_feedback(alerts_modal_t* modal, battery_type_t battery, field_type_t field, float value)
{
	if (!modal || battery >= BATTERY_COUNT || field >= FIELD_COUNT_PER_BATTERY) return;

	lv_obj_t* field_button = modal->field_buttons[battery][field];
	if (!field_button) return;

	// Get validation range for this field
	float min_val = field_ranges[field][0];
	float max_val = field_ranges[field][1];
	float default_val = field_ranges[field][2];

	float final_value = value;
	bool is_out_of_range = false;

	// Validate the value
	if (value < min_val) {
		final_value = min_val;
		is_out_of_range = true;
		printf("[I] alerts_modal: Value %.1f below minimum %.1f, clamping to %.1f\n", value, min_val, min_val);
	} else if (value > max_val) {
		final_value = max_val;
		is_out_of_range = true;
		printf("[I] alerts_modal: Value %.1f above maximum %.1f, clamping to %.1f\n", value, max_val, max_val);
	}

	// Apply the final value
	modal->field_values[battery][field] = final_value;
	update_field_display(modal, battery, field);

	// If out of range, show red border feedback
	if (is_out_of_range) {
		// Set red border
		lv_obj_set_style_border_color(field_button, lv_color_hex(0xff0000), 0);
		lv_obj_set_style_border_width(field_button, 2, 0);

		// Create task data
		border_revert_data_t* data = malloc(sizeof(border_revert_data_t));
		if (data) {
			data->modal = modal;
			data->battery = battery;
			data->field = field;

			// Schedule a timer to revert to white border after 1 second
			lv_timer_t* revert_timer = lv_timer_create(revert_border_color_timer, 1000, data);
			lv_timer_set_repeat_count(revert_timer, 1); // Run only once
		}
	}
}