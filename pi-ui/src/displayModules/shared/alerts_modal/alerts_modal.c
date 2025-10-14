#include <stdio.h>
#include "alerts_modal.h"

#include "../../../fonts/lv_font_noplato_24.h"
#include "../numberpad/numberpad.h"
#include "../palette.h"
#include "../../../state/device_state.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static const char *TAG = "alerts_modal";

// #### Default State Colors ####

#define DEFAULT_WARNING_BORDER_COLOR PALETTE_YELLOW
#define DEFAULT_WARNING_TEXT_COLOR PALETTE_YELLOW

// Updated Warning Colors (for baseline auto-update)
#define UPDATED_WARNING_BORDER_COLOR PALETTE_YELLOW
#define UPDATED_WARNING_TITLE_BORDER_COLOR PALETTE_BLACK
#define UPDATED_WARNING_TITLE_BACKGROUND_COLOR PALETTE_YELLOW

// Changed Value Colors (for modified values)
#define CHANGED_VALUE_BORDER_COLOR PALETTE_GREEN
#define CHANGED_VALUE_TITLE_BORDER_COLOR PALETTE_BLACK
#define CHANGED_VALUE_TITLE_BACKGROUND_COLOR PALETTE_GREEN

// Gauge Container
#define DEFAULT_GAUGE_CONTAINER_BORDER_COLOR PALETTE_GRAY
#define DEFAULT_GAUGE_TITLE_BACKGROUND_COLOR PALETTE_BLUE
#define DEFAULT_GAUGE_TITLE_TEXT_COLOR PALETTE_WHITE

// Group
#define DEFAULT_GROUP_CONTAINER_BORDER_COLOR PALETTE_GRAY
#define DEFAULT_FIELD_ALERT_GROUP_TITLE_BACKGROUND_COLOR PALETTE_YELLOW
#define DEFAULT_FIELD_ALERT_GROUP_TITLE_TEXT_COLOR PALETTE_BLACK
#define DEFAULT_FIELD_GAUGE_GROUP_TITLE_BACKGROUND_COLOR PALETTE_BROWN
#define DEFAULT_FIELD_GAUGE_GROUP_TITLE_TEXT_COLOR PALETTE_WHITE

// Field Value
#define DEFAULT_FIELD_VALUE_TEXT_COLOR PALETTE_WHITE
#define DEFAULT_FIELD_VALUE_TITLE_COLOR PALETTE_WHITE
#define DEFAULT_FIELD_VALUE_TITLE_BACKGROUND_COLOR PALETTE_BLACK

#define DEFAULT_FIELD_VALUE_CONTAINER_BACKGROUND_COLOR PALETTE_BLACK
#define DEFAULT_FIELD_VALUE_CONTAINER_BORDER_COLOR PALETTE_WHITE
#define FIELD_VALUE_TITLE_BACKGROUND_COLOR PALETTE_BLACK



// #### Edit State - Current Value ( Highlighted ) ####

#define IS_EDITING_VALUE_BORDER_COLOR PALETTE_CYAN
#define IS_EDITING_VALUE_TEXT_COLOR PALETTE_WHITE
#define IS_EDITING_VALUE_TITLE_BACKGROUND_COLOR PALETTE_BLACK

// #### Edit State - All other Containers, Borders, and Text Colors ( Dimmed ) ####

// Field Value
#define DIM_FIELD_VALUE_COLOR PALETTE_DARK_GRAY

#define DIM_FIELD_VALUE_CONTAINER_BACKGROUND_COLOR PALETTE_BLACK
#define DIM_FIELD_VALUE_CONTAINER_BORDER_COLOR PALETTE_DARK_GRAY

#define DIM_FIELD_VALUE_TITLE_COLOR PALETTE_BLACK
#define DIM_FIELD_VALUE_TITLE_BACKGROUND_COLOR PALETTE_DARK_GRAY


// Group Container
#define DIM_GROUP_BORDER_COLOR PALETTE_DARK_GRAY
#define DIM_GAUGE_BORDER_COLOR PALETTE_DARK_GRAY

// Gauge Container
#define DIM_FIELD_GAUGE_GROUP_TITLE_TEXT_COLOR PALETTE_BLACK
#define DIM_FIELD_GAUGE_GROUP_TITLE_BACKGROUND_COLOR PALETTE_DARK_GRAY
#define DIM_FIELD_ALERT_GROUP_TITLE_TEXT_COLOR PALETTE_BLACK
#define DIM_FIELD_ALERT_GROUP_TITLE_BACKGROUND_COLOR PALETTE_DARK_GRAY

#define DIM_FIELD_GAUGE_TITLE_BACKGROUND_COLOR PALETTE_DARK_GRAY
#define DIM_FIELD_GAUGE_TITLE_TEXT_COLOR PALETTE_BLACK

// Edit State - Out of Range
#define IS_OUT_OF_RANGE_BORDER_COLOR PALETTE_RED
#define IS_OUT_OF_RANGE_TEXT_COLOR PALETTE_WHITE
#define IS_OUT_OF_RANGE_BACKGROUND_COLOR PALETTE_BLACK

// Changed Value State
#define HAS_CHANGED_BORDER_COLOR PALETTE_GREEN
#define HAS_CHANGED_TEXT_COLOR PALETTE_WHITE
#define HAS_CHANGED_BACKGROUND_COLOR PALETTE_BLACK


// Field validation ranges are now provided by configuration

// Warning system for out-of-range values
typedef struct {
	lv_obj_t* text_label;  // Label for "OVER"/"UNDER"/"MAX"/"MIN" text
	lv_obj_t* value_label; // Label for the numeric value (for max/min warnings)
	lv_obj_t* container;   // Container for max/min warnings (matches value field style)
	lv_timer_t* timer;
	int field_id;  // Field ID instead of field object
	float original_value;  // Store the original out-of-range value for display
	float clamped_value;   // Store the clamped value to revert to when warning expires
	alerts_modal_t* modal;  // Reference to modal for updating display
	int highlighted_field_id;  // Field ID to highlight (for baseline warnings)
	bool is_baseline_warning;  // Whether this is a baseline warning
} warning_data_t;

// Warning data is now allocated per modal instance
// Gauge names, field names, and group names are now provided by configuration

// Global warning data array for all fields
static warning_data_t g_warning_data[15]; // 3 gauges * 5 fields = 15 total fields

// Forward declarations
static void close_button_cb(lv_event_t *e);
static void cancel_button_cb(lv_event_t *e);
static void field_click_handler(lv_event_t *e);
static void initialize_field_data(field_data_t* field_data, int gauge, int field_type, const alerts_modal_config_t* config);
static void update_field_display(alerts_modal_t* modal, int field_id);
static void update_all_field_borders(alerts_modal_t* modal);
static void update_current_field_border(alerts_modal_t* modal);
static int find_field_by_button(alerts_modal_t* modal, lv_obj_t* button);
static void close_current_field(alerts_modal_t* modal);
static void highlight_field_for_warning(alerts_modal_t* modal, int field_id);
static void create_gauge_section(alerts_modal_t* modal, int gauge, lv_obj_t* parent, int y_offset);
static void numberpad_value_changed(const char* value, void* user_data);
static void numberpad_clear(void* user_data);
static void numberpad_enter(const char* value, void* user_data);
static void numberpad_cancel(void* user_data);
static void show_out_of_range_warning(alerts_modal_t* modal, int field_id, float out_of_range_value);
static void hide_out_of_range_warning(alerts_modal_t* modal, int field_id);
static void warning_timer_callback(lv_timer_t* timer);
static void check_and_update_baseline(alerts_modal_t* modal, int gauge_index);
static float clamp_field_value(alerts_modal_t* modal, int field_id, float value);

// Initialize field data (complete state management)
static void initialize_field_data(field_data_t* field_data, int gauge, int field_type, const alerts_modal_config_t* config)
{
	if (!field_data || !config) return;

	// Field identification
	field_data->gauge_index = gauge;
	field_data->field_index = field_type;
	field_data->group_type = (field_type < 2) ? GROUP_ALERTS : GROUP_GAUGE;

	// Set value ranges based on configuration
	const alerts_modal_field_config_t* field_config = &config->gauges[gauge].fields[field_type];
	const alerts_modal_gauge_config_t* gauge_config = &config->gauges[gauge];

	// Use RAW_MIN and RAW_MAX as absolute bounds for all fields
	field_data->min_value = gauge_config->raw_min_value;
	field_data->max_value = gauge_config->raw_max_value;
	field_data->default_value = field_config->default_value;
	field_data->current_value = field_data->default_value;
	field_data->original_value = field_data->default_value;

	// Initialize state flags
	field_data->is_being_edited = false;
	field_data->has_changed = false;
	field_data->is_out_of_range = false;
	field_data->is_warning_highlighted = false;
	field_data->is_updated_warning = false;

	// Initialize UI state
	field_data->border_color = PALETTE_WHITE;  // White border
	field_data->border_width = 2;                       // Default border width
	field_data->text_color = PALETTE_WHITE;    // White text
	field_data->text_background_color = PALETTE_BLACK;  // Black text background
}

// Update field display value using field ID
static void update_field_display(alerts_modal_t* modal, int field_id)
{
	if (!modal || field_id < 0 || field_id >= modal->total_field_count) return;

	field_ui_t* ui = &modal->field_ui[field_id];
	field_data_t* data = &modal->field_data[field_id];

	if (!ui->label) return;

	char value_str[16];

	// If there's an active warning for this field, show the original out-of-range value
	if (modal->field_data[field_id].is_out_of_range) {
		snprintf(value_str, sizeof(value_str), "%.1f", data->current_value);
	} else {
		snprintf(value_str, sizeof(value_str), "%.1f", data->current_value);
	}

	lv_label_set_text(ui->label, value_str);
}


// Check if field value equals original value
static bool field_value_equals_original(field_data_t* data)
{
	if (!data) return false;
	return (fabs(data->current_value - data->original_value) < 0.01f);
}

// Clamp field value based on field type and current field values
static float clamp_field_value(alerts_modal_t* modal, int field_id, float value)
{
	if (!modal || field_id < 0 || field_id >= modal->total_field_count) return value;

	field_data_t* data = &modal->field_data[field_id];
	int gauge_index = data->gauge_index;
	int field_index = data->field_index;

	// Bounds checking to prevent crashes
	if (gauge_index < 0 || gauge_index >= modal->config.gauge_count) {
		printf("[E] alerts_modal: Invalid gauge_index %d in clamp function\n", gauge_index);
		return value;
	}

	// For baseline fields, clamp to midpoint between current LOW and HIGH values
	if (field_index == FIELD_GAUGE_BASELINE) {
		int low_field_id = gauge_index * 5 + FIELD_GAUGE_LOW;
		int high_field_id = gauge_index * 5 + FIELD_GAUGE_HIGH;

		if (low_field_id >= 0 && low_field_id < modal->total_field_count &&
			high_field_id >= 0 && high_field_id < modal->total_field_count) {
			float current_low_value = modal->field_data[low_field_id].current_value;
			float current_high_value = modal->field_data[high_field_id].current_value;

			if (value < current_low_value || value > current_high_value) {
				return (current_low_value + current_high_value) / 2.0f;
			}
		}
		return value;
	}

	// For LOW/HIGH fields, clamp to actual current field values
	if (field_index == FIELD_GAUGE_LOW) {
		// GAUGE LOW: clamp to GAUGE MIN or current GAUGE HIGH value
		int high_field_id = gauge_index * 5 + FIELD_GAUGE_HIGH;
		if (high_field_id >= 0 && high_field_id < modal->total_field_count) {
			float current_gauge_high = modal->field_data[high_field_id].current_value;
			if (value < data->min_value) {
				return data->min_value;
			} else if (value > current_gauge_high) {
				return current_gauge_high;
			}
		}
	} else if (field_index == FIELD_GAUGE_HIGH) {
		// GAUGE HIGH: clamp to current GAUGE LOW value or GAUGE MAX
		int low_field_id = gauge_index * 5 + FIELD_GAUGE_LOW;
		if (low_field_id >= 0 && low_field_id < modal->total_field_count) {
			float current_gauge_low = modal->field_data[low_field_id].current_value;
			if (value < current_gauge_low) {
				return current_gauge_low;
			} else if (value > data->max_value) {
				return data->max_value;
			}
		}
	} else if (field_index == FIELD_ALERT_LOW) {
		// ALERT LOW: clamp to GAUGE MIN or current ALERT HIGH value
		int high_field_id = gauge_index * 5 + FIELD_ALERT_HIGH;
		if (high_field_id >= 0 && high_field_id < modal->total_field_count) {
			float current_alert_high = modal->field_data[high_field_id].current_value;
			if (value < data->min_value) {
				return data->min_value;
			} else if (value > current_alert_high) {
				return current_alert_high;
			}
		}
	} else if (field_index == FIELD_ALERT_HIGH) {
		// ALERT HIGH: clamp to current ALERT LOW value or GAUGE MAX
		int low_field_id = gauge_index * 5 + FIELD_ALERT_LOW;
		if (low_field_id >= 0 && low_field_id < modal->total_field_count) {
			float current_alert_low = modal->field_data[low_field_id].current_value;
			if (value < current_alert_low) {
				return current_alert_low;
			} else if (value > data->max_value) {
				return data->max_value;
			}
		}
	}

	return value;
}

// Get field information for debugging/state inspection
static void get_field_info(field_data_t* data, char* info_buffer, size_t buffer_size)
{
	if (!data || !info_buffer) return;

	snprintf(info_buffer, buffer_size,
		"Field[%d,%d] Group:%d Value:%.1f/%.1f Changed:%d OutOfRange:%d Editing:%d BorderWidth:%d",
		data->gauge_index, data->field_index, data->group_type,
		data->current_value, data->original_value,
		data->has_changed, data->is_out_of_range, data->is_being_edited,
		data->border_width);
}

// Get device state value for a specific gauge and field type using callback
static float get_device_state_value(alerts_modal_t* modal, int gauge, int field_type)
{
	if (!modal || !modal->config.get_value_cb) return 0.0f;
	return modal->config.get_value_cb(gauge, field_type);
}

// Set device state value for a specific gauge and field type using callback
static void set_device_state_value(alerts_modal_t* modal, int gauge, int field_type, float value)
{
	if (!modal || !modal->config.set_value_cb) return;
	modal->config.set_value_cb(gauge, field_type, value);
}

// Update all field borders - clear all highlighting first, then apply active highlighting
static void update_all_field_borders(alerts_modal_t* modal)
{
	if (!modal) return;


	printf("[I] alerts_modal: update_all_field_borders called (current_field_id=%d)\n", modal->current_field_id);

	// Cache references to the gauge containers and group type containers
	typedef struct {
		lv_obj_t* container;
		lv_obj_t* title_label;  // Cached reference to the title label
		bool has_active_field;
		int gauge_index;
		int group_type; // -1 for gauge section, 0 for alerts, 1 for gauge
	} container_info_t;

	container_info_t container_cache[modal->config.gauge_count * 3]; // 3 containers per gauge (section, alerts, gauge)
	int cache_index = 0;

	// Populate cache with all containers
	for (int gauge = 0; gauge < modal->config.gauge_count; gauge++) {

		// Gauge section container
		container_cache[cache_index].container = modal->gauge_sections[gauge];
		container_cache[cache_index].title_label = modal->gauge_titles[gauge];
		container_cache[cache_index].has_active_field = false;
		container_cache[cache_index].gauge_index = gauge;
		container_cache[cache_index].group_type = -1; // Special type for gauge section
		cache_index++;

		// Alert group container
		container_cache[cache_index].container = modal->alert_groups[gauge];
		container_cache[cache_index].title_label = modal->alert_titles[gauge];
		container_cache[cache_index].has_active_field = false;
		container_cache[cache_index].gauge_index = gauge;
		container_cache[cache_index].group_type = GROUP_ALERTS;
		cache_index++;

		// Gauge group container
		container_cache[cache_index].container = modal->gauge_groups[gauge];
		container_cache[cache_index].title_label = modal->gauge_group_title[gauge];
		container_cache[cache_index].has_active_field = false;
		container_cache[cache_index].gauge_index = gauge;
		container_cache[cache_index].group_type = GROUP_GAUGE;
		cache_index++;
	}

	bool does_modal_have_active_field = modal->current_field_id >= 0 &&
		modal->current_field_id < modal->total_field_count;

	printf("[I] alerts_modal: does_modal_have_active_field: %d\n", does_modal_have_active_field);

	// Step 1: Clear all highlighting on Fields and their containers
	for (int field_id = 0; field_id < modal->total_field_count; field_id++) {

		field_ui_t* ui = &modal->field_ui[field_id];
		field_data_t* data = &modal->field_data[field_id];

		if (!ui->button || !ui->label) continue;

		// Reset field to default state
		data->border_color = DEFAULT_FIELD_VALUE_CONTAINER_BORDER_COLOR;
		data->button_background_color = DEFAULT_FIELD_VALUE_CONTAINER_BACKGROUND_COLOR;
		data->border_width = 1;
		data->text_color = DEFAULT_FIELD_VALUE_TEXT_COLOR;
		data->text_background_color = FIELD_VALUE_TITLE_BACKGROUND_COLOR;
		data->title_color = DEFAULT_FIELD_VALUE_TITLE_COLOR;
		data->title_background_color = DEFAULT_FIELD_VALUE_TITLE_BACKGROUND_COLOR;

		bool value_should_dim = does_modal_have_active_field && !data->is_being_edited;

		if( data->is_being_edited ){ // Active field being edited

			data->border_color = IS_EDITING_VALUE_BORDER_COLOR;
			data->border_width = 2;
			data->button_background_color = DIM_FIELD_VALUE_CONTAINER_BACKGROUND_COLOR;

			data->title_color = DEFAULT_FIELD_VALUE_TITLE_COLOR;
			data->title_background_color = DEFAULT_FIELD_VALUE_TITLE_BACKGROUND_COLOR;
		} else if( does_modal_have_active_field ){  // field should be dimmed gray since there is an active one somewhere

			data->text_color = DIM_FIELD_VALUE_COLOR;
			data->text_background_color = DIM_FIELD_VALUE_TITLE_BACKGROUND_COLOR;

			data->border_color = DIM_FIELD_VALUE_CONTAINER_BORDER_COLOR;
			data->button_background_color = DIM_FIELD_VALUE_CONTAINER_BACKGROUND_COLOR;

			data->title_color = DIM_FIELD_VALUE_TITLE_COLOR;
			data->title_background_color = DIM_FIELD_VALUE_TITLE_BACKGROUND_COLOR;
		} else if( data->is_updated_warning ){ // Updated warning (yellow)

			data->border_color = UPDATED_WARNING_BORDER_COLOR;
			data->title_background_color = UPDATED_WARNING_TITLE_BACKGROUND_COLOR;
			data->title_color = UPDATED_WARNING_TITLE_BORDER_COLOR;
			data->border_width = 3;
		} else if( data->is_out_of_range ){ // Out of range

			data->border_color = IS_OUT_OF_RANGE_BORDER_COLOR;
			data->border_width = 2;
		} else if( data->has_changed ){ // Green border for changed values

			data->border_color = CHANGED_VALUE_BORDER_COLOR;
			data->title_background_color = CHANGED_VALUE_TITLE_BACKGROUND_COLOR;
			data->title_color = CHANGED_VALUE_TITLE_BORDER_COLOR;
			data->border_width = 2;
		}

		// Apply the highlighting to field UI
		lv_obj_set_style_text_color( ui->label, data->text_color, 0 );
		lv_obj_set_style_border_color( ui->button, data->border_color, 0 );
		lv_obj_set_style_bg_color( ui->button, data->button_background_color, 0 );
		lv_obj_set_style_border_width(ui->button, data->border_width, 0);

		// Apply dimming to title label as well
		if (ui->title) {
			lv_obj_set_style_text_color( ui->title, data->title_color, 0 );
			lv_obj_set_style_bg_color( ui->title, data->title_background_color, 0);
		}

		// Track which containers have active fields
		int gauge_index = data->gauge_index;

		if (data->is_being_edited) {

			// Mark gauge section as having active field
			for (int i = 0; i < modal->config.gauge_count * 3; i++) {

				if (container_cache[i].gauge_index == gauge_index && container_cache[i].group_type == -1) {

					container_cache[i].has_active_field = true;
				}
			}

			// Mark appropriate group as having active field
			for (int i = 0; i < modal->config.gauge_count * 3; i++) {

				if (container_cache[i].gauge_index == gauge_index && container_cache[i].group_type == data->group_type) {

					container_cache[i].has_active_field = true;
				}
			}
		}

	}

	// Step 3: Style the cached containers based on whether they contain active fields
	for (int i = 0; i < modal->config.gauge_count * 3; i++) {

		lv_obj_t* container = container_cache[ i ].container;
		lv_obj_t* title_label = container_cache[ i ].title_label;
		bool has_active_field = container_cache[ i ].has_active_field;

		// Determine if this container should be dimmed
		// Dim only if there's an active field somewhere else (not in this container)
		bool should_dim = does_modal_have_active_field && !has_active_field;

		if (has_active_field) {

			// White border for containers with active fields
			lv_obj_set_style_border_color(container, BORDER_COLOR, 0);
			// lv_obj_set_style_border_width(container, 2, 0);
		} else {

			// Dimmed border for containers without active fields
			lv_color_t container_border_color = should_dim ? DIM_GROUP_BORDER_COLOR : DEFAULT_GROUP_CONTAINER_BORDER_COLOR;
			lv_obj_set_style_border_color(container, container_border_color, 0);
			// lv_obj_set_style_border_width(container, should_dim ? 1 : 2, 0);
		}

		// Update text color for the cached title label
		if (title_label) {

			lv_color_t title_background_color;
			lv_color_t title_text_color;

			if( should_dim ){

				title_background_color = DIM_FIELD_VALUE_TITLE_BACKGROUND_COLOR;

				// Use group-specific dim text colors
				if (container_cache[i].group_type == GROUP_ALERTS) {

					title_text_color = DIM_FIELD_ALERT_GROUP_TITLE_TEXT_COLOR;
					title_background_color = DIM_FIELD_ALERT_GROUP_TITLE_BACKGROUND_COLOR;

				} else if (container_cache[i].group_type == GROUP_GAUGE) {

					title_text_color = DIM_FIELD_GAUGE_GROUP_TITLE_TEXT_COLOR;
					title_background_color = DIM_FIELD_ALERT_GROUP_TITLE_BACKGROUND_COLOR;
				} else if (container_cache[i].group_type == -1) {

					// Gauge section title
					title_text_color = DIM_FIELD_GAUGE_TITLE_TEXT_COLOR;
					title_background_color = DIM_FIELD_GAUGE_TITLE_BACKGROUND_COLOR;
				} else {

					title_text_color = DIM_FIELD_VALUE_TITLE_COLOR;
					title_background_color = DIM_FIELD_VALUE_TITLE_BACKGROUND_COLOR;
				}
			}else{

				// todo: this is not DRY
				// Use group-specific colors when field is being edited, default when not
				if (has_active_field) {
					// Field is being edited in this group - use group-specific colors
					if (container_cache[i].group_type == GROUP_ALERTS) {

						title_background_color = DEFAULT_FIELD_ALERT_GROUP_TITLE_BACKGROUND_COLOR;
						title_text_color = DEFAULT_FIELD_ALERT_GROUP_TITLE_TEXT_COLOR;
					} else if (container_cache[i].group_type == GROUP_GAUGE) {

						title_background_color = DEFAULT_FIELD_GAUGE_GROUP_TITLE_BACKGROUND_COLOR;
						title_text_color = DEFAULT_FIELD_GAUGE_GROUP_TITLE_TEXT_COLOR;
					} else if (container_cache[i].group_type == -1) {

						// Gauge section title - use default gauge title colors
						title_background_color = DEFAULT_GAUGE_TITLE_BACKGROUND_COLOR;
						title_text_color = DEFAULT_GAUGE_TITLE_TEXT_COLOR;
					} else {

						title_background_color = FIELD_VALUE_TITLE_BACKGROUND_COLOR;
						title_text_color = TEXT_COLOR_LIGHT;
					}
				} else {
					// No field being edited - use group-specific default colors
					if (container_cache[i].group_type == GROUP_ALERTS) {

						title_background_color = DEFAULT_FIELD_ALERT_GROUP_TITLE_BACKGROUND_COLOR;
						title_text_color = DEFAULT_FIELD_ALERT_GROUP_TITLE_TEXT_COLOR;
					} else if (container_cache[i].group_type == GROUP_GAUGE) {

						title_background_color = DEFAULT_FIELD_GAUGE_GROUP_TITLE_BACKGROUND_COLOR;
						title_text_color = DEFAULT_FIELD_GAUGE_GROUP_TITLE_TEXT_COLOR;
					} else if (container_cache[i].group_type == -1) {

						// Gauge section title - use default gauge title colors
						title_background_color = DEFAULT_GAUGE_TITLE_BACKGROUND_COLOR;
						title_text_color = DEFAULT_GAUGE_TITLE_TEXT_COLOR;
					} else {

						title_background_color = FIELD_VALUE_TITLE_BACKGROUND_COLOR;
						title_text_color = TEXT_COLOR_LIGHT;
					}
				}
			}

			lv_obj_set_style_text_color( title_label, title_text_color, 0 );
			lv_obj_set_style_bg_color(title_label, title_background_color, 0);
		}
	}
}

// Update only the current field's border (more efficient during editing)
static void update_current_field_border(alerts_modal_t* modal)
{
	if (!modal || modal->current_field_id < 0) return;

	field_ui_t* ui = &modal->field_ui[modal->current_field_id];
	field_data_t* data = &modal->field_data[modal->current_field_id];

	if (!ui->button || !ui->label) return;

	// Update state based on current conditions
	if (data->is_out_of_range) {
		// Red border for out of range
		data->border_color = IS_OUT_OF_RANGE_BORDER_COLOR;
		data->border_width = 2;
	} else if (data->has_changed && !data->is_being_edited) {
		// Green border for changed values
		data->border_color = CHANGED_VALUE_BORDER_COLOR;
		data->title_background_color = CHANGED_VALUE_TITLE_BACKGROUND_COLOR;
		data->title_color = CHANGED_VALUE_TITLE_BORDER_COLOR;
		data->border_width = 2;
	} else {
		// Cyan border for editing state
		data->border_color = IS_EDITING_VALUE_BORDER_COLOR;
		data->border_width = 2;
	}

	// Always reset text color to white for active field
	data->text_color = IS_EDITING_VALUE_TEXT_COLOR;

	// Apply the state to UI
	lv_obj_set_style_text_color(ui->label, data->text_color, 0);
	lv_obj_set_style_border_color(ui->button, data->border_color, 0);
	lv_obj_set_style_border_width(ui->button, data->border_width, 0);
}

static void highlight_field_for_warning(alerts_modal_t* modal, int field_id)
{
	if (!modal || field_id < 0 || field_id >= modal->total_field_count) return;

	field_data_t* data = &modal->field_data[field_id];

	// Set warning highlighted flag
	data->is_warning_highlighted = true;

	// Update all field borders to apply styling
	update_all_field_borders(modal);

	printf("[I] alerts_modal: Highlighted field %d for warning (border and text)\n", field_id);
}


// Find field by button
static int find_field_by_button(alerts_modal_t* modal, lv_obj_t* button)
{
	if (!modal || !button) return -1;

		for (int i = 0; i < modal->total_field_count; i++) {
		if (modal->field_ui[i].button == button) {
			return i;
		}
	}
	return -1;
}

// Close current field
static void close_current_field(alerts_modal_t* modal)
{
	if (!modal || modal->current_field_id < 0) return;

	field_data_t* data = &modal->field_data[modal->current_field_id];

	// Clamp the value using proper field-specific clamping logic
	data->current_value = clamp_field_value(modal, modal->current_field_id, data->current_value);

	data->is_being_edited = false;
	data->is_out_of_range = false;
	data->has_changed = !field_value_equals_original(data);

	// Save this field's value to device state
	set_device_state_value(modal, data->gauge_index, data->field_index, data->current_value);
	device_state_save();

	printf("[I] alerts_modal: Saved field[%d,%d] value: %.1f\n",
		data->gauge_index, data->field_index, data->current_value);

	// Check if this is a GAUGE LOW or HIGH field change that might affect baseline
	if (data->field_index == FIELD_GAUGE_LOW || data->field_index == FIELD_GAUGE_HIGH) {
		check_and_update_baseline(modal, data->gauge_index);
	}

	// Hide any warning for this field
		hide_out_of_range_warning(modal, modal->current_field_id);

	modal->current_field_id = -1;

	update_all_field_borders(modal); // Apply UI changes

	if (modal->numberpad) {
		numberpad_hide(modal->numberpad);
	}
}

// Field click handler
static void field_click_handler(lv_event_t *e)
{
	alerts_modal_t* modal = (alerts_modal_t*)lv_event_get_user_data(e);
	lv_obj_t* target = lv_event_get_target(e);

	printf("[I] alerts_modal: field_click_handler called, target: %p\n", target);

	if (!modal || !target) return;

	// Find which field was clicked first
	int field_id = find_field_by_button(modal, target);

	// If a field is currently being edited, check if this click should close it
	if (modal->current_field_id >= 0) {
		printf("[I] alerts_modal: Field is being edited, checking if click is on numberpad\n");
		printf("[I] alerts_modal: Numberpad exists: %d, visible: %d, target: %p, background: %p\n",
			modal->numberpad != NULL,
			modal->numberpad ? modal->numberpad->is_visible : 0,
			target,
			modal->numberpad ? modal->numberpad->background : NULL);

		// Check if the target is the numberpad background container
		if (modal->numberpad && modal->numberpad->is_visible && target == modal->numberpad->background) {

			printf("[I] alerts_modal: Click is on numberpad background, letting numberpad handle it\n");
			return; // This is a numberpad container click, let numberpad handle it
		}

		// If we get here, it's not a numberpad click, so close the current field
		printf("[I] alerts_modal: Click is NOT on numberpad, closing current field\n");
		close_current_field(modal);

		// If the click was on another field, continue to open it
		if( field_id < 0 ){

			printf("[I] alerts_modal: Click not on a field button, ignoring\n");
			return;
		}
	} else if( field_id < 0 ){

		// No field is being edited, check if this is a field click
		return;
	}

	field_data_t* data = &modal->field_data[field_id];

	// Log complete field state
	char field_info[256];
	get_field_info(data, field_info, sizeof(field_info));
	printf("[I] alerts_modal: Field clicked: %s\n", field_info);

	// Open this field
	modal->current_field_id = field_id;
	data->is_being_edited = true;

	// Show numberpad
	if( !modal->numberpad ){

		numberpad_config_t numpad_config = NUMBERPAD_DEFAULT_CONFIG;
		numpad_config.max_digits = 4;
		numpad_config.decimal_places = 1;
		numpad_config.auto_decimal = true;
		modal->numberpad = numberpad_create(&numpad_config, modal->background);

		// todo: probably dont need the conditional
		if( modal->numberpad ){

			numberpad_set_callbacks(
				modal->numberpad,
				numberpad_value_changed,
				numberpad_clear,
				numberpad_enter,
				numberpad_cancel,
				modal
			);
			}
	}

	if (modal->numberpad) {

		// Set current value in numberpad
		char value_str[16];
		snprintf(value_str, sizeof(value_str), "%.1f", data->current_value);
		numberpad_set_value(modal->numberpad, value_str);

		// Show numberpad aligned to field but positioned outside gauge container
		lv_obj_t* gauge_container = modal->gauge_sections[data->gauge_index];
		numberpad_show_outside_container(modal->numberpad, target, gauge_container);
		update_all_field_borders(modal);
	}
}


// Close button callback
static void close_button_cb(lv_event_t *e)
{
	alerts_modal_t* modal = (alerts_modal_t*)lv_event_get_user_data(e);

	if (!modal) return;

	close_current_field(modal);

	// Refresh gauges and alerts after saving changes
	alerts_modal_refresh_gauges_and_alerts(modal);

	if (modal->on_close) {

		modal->on_close();
	}
}

// Cancel button callback - reverts all changes and closes modal
static void cancel_button_cb(lv_event_t *e)
{
	alerts_modal_t* modal = (alerts_modal_t*)lv_event_get_user_data(e);

	if (!modal) return;

	// Close any currently editing field first
	close_current_field(modal);

	// Revert all field values to their original values
		for (int field_id = 0; field_id < modal->total_field_count; field_id++) {
		field_data_t* data = &modal->field_data[field_id];

		// Restore original value
		data->current_value = data->original_value;
		data->has_changed = false;
		data->is_out_of_range = false;

		// Clear any warning UI elements
		hide_out_of_range_warning(modal, field_id);

		// Update display and border
		update_field_display(modal, field_id);
		update_all_field_borders(modal);
	}

	// Update all field borders to reflect the reverted state
	update_all_field_borders(modal);

	// Save the reverted values to device state
	for (int field_id = 0; field_id < modal->total_field_count; field_id++) {
		field_data_t* data = &modal->field_data[field_id];
		set_device_state_value(modal, data->gauge_index, data->field_index, data->current_value);
	}
	device_state_save();

	printf("[I] alerts_modal: Cancel button pressed - reverted all changes\n");

	// Refresh gauges and alerts after reverting changes
	alerts_modal_refresh_gauges_and_alerts(modal);

	// Close the modal
	if (modal->on_close) {
		modal->on_close();
	}
}

// Numberpad callbacks
static void check_and_update_baseline(alerts_modal_t* modal, int gauge_index)
{
	// Get current LOW and HIGH values
	float current_low = modal->field_data[gauge_index * 5 + FIELD_GAUGE_LOW].current_value;
	float current_high = modal->field_data[gauge_index * 5 + FIELD_GAUGE_HIGH].current_value;

	// Get current baseline value
	int baseline_field_id = gauge_index * 5 + FIELD_GAUGE_BASELINE;
	field_data_t* baseline_data = &modal->field_data[baseline_field_id];
	float current_baseline = baseline_data->current_value;

	// Calculate new midpoint
	float new_midpoint = (current_low + current_high) / 2.0f;

	// Check if baseline is out of range
	bool baseline_out_of_range = (current_baseline < current_low) || (current_baseline > current_high);

	if (baseline_out_of_range) {
		// Update baseline to new midpoint
		baseline_data->current_value = new_midpoint;
		baseline_data->has_changed = true;

		// Mark baseline as updated warning to trigger yellow highlighting
		baseline_data->is_updated_warning = true;
		baseline_data->is_out_of_range = true;

		// Update the baseline display first
		update_field_display(modal, baseline_field_id);
		update_all_field_borders(modal);

		// Show "UPDATED" warning for baseline
		show_out_of_range_warning(modal, baseline_field_id, new_midpoint);

		printf("[I] alerts_modal: Baseline updated to %.1f (was %.1f, new range: %.1f-%.1f)\n",
			new_midpoint, current_baseline, current_low, current_high);
	}
}

static void numberpad_value_changed(const char* value, void* user_data)
{
	alerts_modal_t* modal = (alerts_modal_t*)user_data;
	if (!modal || modal->current_field_id < 0) return;

	field_data_t* data = &modal->field_data[modal->current_field_id];
	float new_value = atof(value);

	// Always store the actual input value for display
	data->current_value = new_value;

	// For baseline fields, check against current LOW and HIGH values instead of min/max
	if (data->field_index == FIELD_GAUGE_BASELINE) {
		int gauge_index = data->gauge_index;
		float current_low_value = modal->field_data[gauge_index * 5 + FIELD_GAUGE_LOW].current_value;
		float current_high_value = modal->field_data[gauge_index * 5 + FIELD_GAUGE_HIGH].current_value;

		// Clear any existing "UPDATED" warning when user starts editing baseline
		if (data->is_updated_warning) {
			data->is_updated_warning = false;
			hide_out_of_range_warning(modal, modal->current_field_id);
		}

		// Check if value is out of range (below low or above high)
		bool is_out_of_range = (new_value < current_low_value) || (new_value > current_high_value);

		if (is_out_of_range) {
			printf("[I] alerts_modal: Baseline value %.1f is out of range (LOW: %.1f, HIGH: %.1f), showing warning\n",
				new_value, current_low_value, current_high_value);

			// Show warning for out-of-range value
			show_out_of_range_warning(modal, modal->current_field_id, new_value);
			data->is_out_of_range = true;
		} else {
			printf("[I] alerts_modal: Baseline value %.1f is within range (LOW: %.1f, HIGH: %.1f), hiding warning\n",
				new_value, current_low_value, current_high_value);

			// Hide any existing warning
			hide_out_of_range_warning(modal, modal->current_field_id);
			data->is_out_of_range = false;
		}
	} else {
		// For LOW/HIGH fields, use proper field-specific range checking
		int gauge_index = data->gauge_index;
		int field_index = data->field_index;
		bool is_out_of_range = false;

		// Bounds checking to prevent crashes
		if (gauge_index < 0 || gauge_index >= modal->config.gauge_count) {
			printf("[E] alerts_modal: Invalid gauge_index %d in range checking\n", gauge_index);
			return;
		}

		if (field_index == FIELD_GAUGE_LOW) {
			// GAUGE LOW: check against GAUGE MIN or current GAUGE HIGH value
			int high_field_id = gauge_index * 5 + FIELD_GAUGE_HIGH;
			if (high_field_id >= 0 && high_field_id < modal->total_field_count) {
				float current_gauge_high = modal->field_data[high_field_id].current_value;
				is_out_of_range = (new_value < data->min_value || new_value > current_gauge_high);
			}
		} else if (field_index == FIELD_GAUGE_HIGH) {
			// GAUGE HIGH: check against current GAUGE LOW value or GAUGE MAX
			int low_field_id = gauge_index * 5 + FIELD_GAUGE_LOW;
			if (low_field_id >= 0 && low_field_id < modal->total_field_count) {
				float current_gauge_low = modal->field_data[low_field_id].current_value;
				is_out_of_range = (new_value < current_gauge_low || new_value > data->max_value);
			}
		} else if (field_index == FIELD_ALERT_LOW) {
			// ALERT LOW: check against GAUGE MIN or current ALERT HIGH value
			int high_field_id = gauge_index * 5 + FIELD_ALERT_HIGH;
			if (high_field_id >= 0 && high_field_id < modal->total_field_count) {
				float current_alert_high = modal->field_data[high_field_id].current_value;
				is_out_of_range = (new_value < data->min_value || new_value > current_alert_high);
			}
		} else if (field_index == FIELD_ALERT_HIGH) {
			// ALERT HIGH: check against current ALERT LOW value or GAUGE MAX
			int low_field_id = gauge_index * 5 + FIELD_ALERT_LOW;
			if (low_field_id >= 0 && low_field_id < modal->total_field_count) {
				float current_alert_low = modal->field_data[low_field_id].current_value;
				is_out_of_range = (new_value < current_alert_low || new_value > data->max_value);
			}
		}

		// Update warning state based on current range status
		if (is_out_of_range && !data->is_out_of_range) {
			// Show warning when transitioning from in-range to out-of-range
			show_out_of_range_warning(modal, modal->current_field_id, new_value);
			data->is_out_of_range = true;
		} else if (!is_out_of_range && data->is_out_of_range) {
			// Hide any existing warning when value comes back into range
			hide_out_of_range_warning(modal, modal->current_field_id);
			data->is_out_of_range = false;
		} else if (is_out_of_range && data->is_out_of_range) {
			// Already out-of-range, but check if we need to update warning type (MIN->MAX transition)
			// Always update warning to handle MIN->MAX transitions
			show_out_of_range_warning(modal, modal->current_field_id, new_value);
		}
	}

	update_field_display(modal, modal->current_field_id);
	update_current_field_border(modal); // More efficient - only updates current field
}

static void numberpad_clear(void* user_data)
{
	alerts_modal_t* modal = (alerts_modal_t*)user_data;
	if (!modal || modal->current_field_id < 0) return;

	field_data_t* data = &modal->field_data[modal->current_field_id];
	data->current_value = 0.0f;
	data->is_out_of_range = false;

	update_field_display(modal, modal->current_field_id);
	update_current_field_border(modal); // More efficient - only updates current field
}

static void numberpad_enter(const char* value, void* user_data)
{
	// Save and close on enter
	alerts_modal_t* modal = (alerts_modal_t*)user_data;
	if (modal) {
		close_current_field(modal);
	}
}

static void numberpad_cancel(void* user_data)
{
	// Cancel without saving - restore original value and close
	alerts_modal_t* modal = (alerts_modal_t*)user_data;

	// Bail early if we don't have a valid modal
	if( !modal ){ return; }

	// Restore original value if we have it
	bool found_original_value = (
		modal->current_field_id >= 0 &&
		modal->current_field_id < modal->total_field_count
	);

	if( found_original_value ){

		field_data_t* data = &modal->field_data[modal->current_field_id];

		// Bail early if we don't have a valid field data
		if( data->original_value < 0  ){ return; }

		// Restore original value to the field data
		data->current_value = data->original_value;

		// Update the field display to show the original value
		update_field_display(modal, modal->current_field_id);

		// Clear any out-of-range state
		data->is_out_of_range = false;

		// Hide any existing warning for this field
		hide_out_of_range_warning(modal, modal->current_field_id);

	}

	close_current_field(modal);

}

// Warning system implementation
static void show_out_of_range_warning(alerts_modal_t* modal, int field_id, float out_of_range_value)
{
	// Bail early if we don't have a valid modal or field ID
	if (!modal || field_id < 0 || field_id >= modal->total_field_count) return;

	field_ui_t* ui = &modal->field_ui[field_id];

	// Bail early if we don't have a valid UI button
	if (!ui->button) return;

	// Hide any existing warning for this field first
	hide_out_of_range_warning(modal, field_id);

	// Get field data and calculate clamped value
	field_data_t* data = &modal->field_data[field_id];
	float clamped_value;

	int gauge_index = data->gauge_index;

	// Get the corresponding LOW and HIGH field values based on field type
	float current_low_value, current_high_value;

	// Bounds checking to prevent crashes
	if (gauge_index < 0 || gauge_index >= modal->config.gauge_count) {
		printf("[E] alerts_modal: Invalid gauge_index %d in warning system\n", gauge_index);
		return;
	}

	if (data->field_index == FIELD_ALERT_LOW || data->field_index == FIELD_ALERT_HIGH) {
		// For ALERT fields, use ALERT field values
		int low_field_id = gauge_index * 5 + FIELD_ALERT_LOW;
		int high_field_id = gauge_index * 5 + FIELD_ALERT_HIGH;

		if (low_field_id >= 0 && low_field_id < modal->total_field_count &&
			high_field_id >= 0 && high_field_id < modal->total_field_count) {
			current_low_value = modal->field_data[low_field_id].current_value;
			current_high_value = modal->field_data[high_field_id].current_value;
		} else {
			printf("[E] alerts_modal: Invalid field IDs in warning system (low=%d, high=%d)\n", low_field_id, high_field_id);
			return;
		}
	} else {
		// For GAUGE fields, use GAUGE field values
		int low_field_id = gauge_index * 5 + FIELD_GAUGE_LOW;
		int high_field_id = gauge_index * 5 + FIELD_GAUGE_HIGH;

		if (low_field_id >= 0 && low_field_id < modal->total_field_count &&
			high_field_id >= 0 && high_field_id < modal->total_field_count) {
			current_low_value = modal->field_data[low_field_id].current_value;
			current_high_value = modal->field_data[high_field_id].current_value;
		} else {
			printf("[E] alerts_modal: Invalid field IDs in warning system (low=%d, high=%d)\n", low_field_id, high_field_id);
			return;
		}
	}

	// Initialize clamped_value
	clamped_value = out_of_range_value;

	bool is_baseline = (data->field_index == FIELD_GAUGE_BASELINE);

	// Store the out-of-range value for display during warning
	// The field will show this value while the warning is active
	modal->field_data[field_id].current_value = out_of_range_value;

	// Create warning text label (for "OVER"/"UNDER"/"MAX"/"MIN")
	g_warning_data[field_id].text_label = lv_label_create(modal->background);
	lv_label_set_text(g_warning_data[field_id].text_label, ""); // Initialize with empty text
	lv_obj_set_style_text_color(g_warning_data[field_id].text_label, PALETTE_YELLOW, 0); // Warm yellow
	lv_obj_set_style_text_font(g_warning_data[field_id].text_label, &lv_font_montserrat_20, 0); // Modal font, bigger
	lv_obj_set_style_text_align(g_warning_data[field_id].text_label, LV_TEXT_ALIGN_CENTER, 0); // Center align
	lv_obj_set_style_bg_color(g_warning_data[field_id].text_label, PALETTE_BLACK, 0);
	lv_obj_set_style_bg_opa(g_warning_data[field_id].text_label, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_all(g_warning_data[field_id].text_label, 4, 0);
	lv_obj_clear_flag(g_warning_data[field_id].text_label, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_set_style_radius(g_warning_data[field_id].text_label, 8, 0); // Match field container radius
	lv_obj_set_style_border_color(g_warning_data[field_id].text_label, DEFAULT_WARNING_BORDER_COLOR, 0); // Yellow border
	lv_obj_set_style_border_width(g_warning_data[field_id].text_label, 3, 0); // Match highlighted field border width
	// Hide the label initially until it has proper text
	lv_obj_add_flag(g_warning_data[field_id].text_label, LV_OBJ_FLAG_HIDDEN);

	// Create warning value label and container (for numeric values in max/min warnings)
	g_warning_data[field_id].value_label = NULL; // Will be created only for max/min warnings
	g_warning_data[field_id].container = NULL; // Will be created only for max/min warnings

	// Determine if value is above max or below min using field-specific constraints
	bool is_above_max = false;
	bool is_below_min = false;

	if (data->field_index == FIELD_GAUGE_LOW) {
		// GAUGE LOW: check against GAUGE MIN or current GAUGE HIGH value
		int gauge_index = data->gauge_index;
		int high_field_id = gauge_index * 5 + FIELD_GAUGE_HIGH;
		if (high_field_id >= 0 && high_field_id < modal->total_field_count) {
			float current_gauge_high = modal->field_data[high_field_id].current_value;
			is_above_max = (out_of_range_value > current_gauge_high);
			is_below_min = (out_of_range_value < data->min_value);
		}
	} else if (data->field_index == FIELD_GAUGE_HIGH) {
		// GAUGE HIGH: check against current GAUGE LOW value or GAUGE MAX
		int gauge_index = data->gauge_index;
		int low_field_id = gauge_index * 5 + FIELD_GAUGE_LOW;
		if (low_field_id >= 0 && low_field_id < modal->total_field_count) {
			float current_gauge_low = modal->field_data[low_field_id].current_value;
			is_above_max = (out_of_range_value > data->max_value); // Use gauge RAW MAX
			is_below_min = (out_of_range_value < current_gauge_low);
		}
	} else if (data->field_index == FIELD_ALERT_LOW) {
		// ALERT LOW: check against GAUGE MIN or current ALERT HIGH value
		int gauge_index = data->gauge_index;
		int high_field_id = gauge_index * 5 + FIELD_ALERT_HIGH;
		if (high_field_id >= 0 && high_field_id < modal->total_field_count) {
			float current_alert_high = modal->field_data[high_field_id].current_value;
			is_above_max = (out_of_range_value > current_alert_high);
			is_below_min = (out_of_range_value < data->min_value);
		}
	} else if (data->field_index == FIELD_ALERT_HIGH) {
		// ALERT HIGH: check against current ALERT LOW value or GAUGE MAX
		int gauge_index = data->gauge_index;
		int low_field_id = gauge_index * 5 + FIELD_ALERT_LOW;
		if (low_field_id >= 0 && low_field_id < modal->total_field_count) {
			float current_alert_low = modal->field_data[low_field_id].current_value;
			is_above_max = (out_of_range_value > data->max_value); // Use gauge RAW MAX
			is_below_min = (out_of_range_value < current_alert_low);
		}
	} else {
		// For other fields, use simple config values
		is_above_max = (out_of_range_value > data->max_value);
		is_below_min = (out_of_range_value < data->min_value);
	}

	// Get field type to determine if it's a baseline warning
	bool is_baseline_warning = false;
	int highlighted_field_id = -1;
		if (field_id >= 0 && field_id < modal->total_field_count) {
		int gauge_index = field_id / 5; // 5 fields per gauge
		int field_index = field_id % 5;
		is_baseline_warning = (field_index == FIELD_GAUGE_BASELINE);

		// For baseline warnings, determine which field to highlight
		if (is_baseline_warning) {
			if (is_above_max) {
				// Baseline is above max, highlight the max field
				highlighted_field_id = gauge_index * 5 + 4; // FIELD_GAUGE_HIGH
			} else if (is_below_min) {
				// Baseline is below min, highlight the min field
				highlighted_field_id = gauge_index * 5 + 2; // FIELD_GAUGE_LOW
			}
		}
	}

	// Update clamped value for LOW/HIGH fields
	if (!is_baseline_warning) {
		// For LOW/HIGH fields, clamp to actual current field values
		int gauge_index = data->gauge_index;
		int field_index = data->field_index;

		if (field_index == FIELD_GAUGE_LOW) {
			// GAUGE LOW: clamp to GAUGE MIN or current GAUGE HIGH value
			float current_gauge_high = modal->field_data[gauge_index * 5 + FIELD_GAUGE_HIGH].current_value;
			if (out_of_range_value < data->min_value) {
				clamped_value = data->min_value;
			} else if (out_of_range_value > current_gauge_high) {
				clamped_value = current_gauge_high;
			}
		} else if (field_index == FIELD_GAUGE_HIGH) {
			// GAUGE HIGH: clamp to current GAUGE LOW value or GAUGE MAX
			float current_gauge_low = modal->field_data[gauge_index * 5 + FIELD_GAUGE_LOW].current_value;
			if (out_of_range_value < current_gauge_low) {
				clamped_value = current_gauge_low;
			} else if (out_of_range_value > data->max_value) {
				clamped_value = data->max_value;
			}
		} else if (field_index == FIELD_ALERT_LOW) {
			// ALERT LOW: clamp to GAUGE MIN or current ALERT HIGH value
			float current_alert_high = modal->field_data[gauge_index * 5 + FIELD_ALERT_HIGH].current_value;
			if (out_of_range_value < data->min_value) {
				clamped_value = data->min_value;
			} else if (out_of_range_value > current_alert_high) {
				clamped_value = current_alert_high;
			}
		} else if (field_index == FIELD_ALERT_HIGH) {
			// ALERT HIGH: clamp to current ALERT LOW value or GAUGE MAX
			float current_alert_low = modal->field_data[gauge_index * 5 + FIELD_ALERT_LOW].current_value;
			if (out_of_range_value < current_alert_low) {
				clamped_value = current_alert_low;
			} else if (out_of_range_value > data->max_value) {
				clamped_value = data->max_value;
			}
		}
	}

	// Update clamped value for baseline fields
	if (is_baseline_warning) {
		// For baseline values that are out of range, clamp to midpoint between current LOW and HIGH values
		if (out_of_range_value > current_high_value || out_of_range_value < current_low_value) {
			clamped_value = (current_low_value + current_high_value) / 2.0f; // Midpoint of current LOW and HIGH values
			printf("[I] alerts_modal: Clamping baseline value %.1f to midpoint %.1f (current LOW: %.1f, HIGH: %.1f)\n",
				out_of_range_value, clamped_value, current_low_value, current_high_value);
		}
	}

	// Store the clamped value to revert to when warning expires
	g_warning_data[field_id].clamped_value = clamped_value;


	// Set warning text based on warning type
	if (is_baseline_warning) {
		// Check if this is an automatic baseline update (value equals midpoint)
		float expected_midpoint = (current_low_value + current_high_value) / 2.0f;
		bool is_automatic_update = (fabs(out_of_range_value - expected_midpoint) < 0.01f);

		if (is_automatic_update) {
			// This is an automatic baseline update
			lv_label_set_text(g_warning_data[field_id].text_label, "UPDATED");
		} else if (is_above_max) {
			// User input above range
			lv_label_set_text(g_warning_data[field_id].text_label, "OVER");
		} else if (is_below_min) {
			// User input below range
			lv_label_set_text(g_warning_data[field_id].text_label, "UNDER");
		} else {
			// Fallback - shouldn't happen
			lv_label_set_text(g_warning_data[field_id].text_label, "RANGE");
		}

		// Show the label now that it has text
		lv_obj_clear_flag(g_warning_data[field_id].text_label, LV_OBJ_FLAG_HIDDEN);
	} else {

		// Regular max/min warnings: show "MAX" or "MIN" text + value in single container
		if (is_above_max)
		{
			// Create container that encompasses both text and value
			g_warning_data[field_id].container = lv_obj_create(modal->background);
			lv_obj_set_size(g_warning_data[field_id].container, 63, 80); // Larger to fit both text and value, 3px wider for negative values
			lv_obj_set_style_bg_color(g_warning_data[field_id].container, PALETTE_BLACK, 0); // Black background
			lv_obj_set_style_bg_opa(g_warning_data[field_id].container, LV_OPA_COVER, 0);
			lv_obj_set_style_border_color(g_warning_data[field_id].container, PALETTE_YELLOW, 0); // Yellow border
			lv_obj_set_style_border_width(g_warning_data[field_id].container, 2, 0);
			lv_obj_set_style_radius(g_warning_data[field_id].container, 8, 0);
			lv_obj_clear_flag(g_warning_data[field_id].container, LV_OBJ_FLAG_SCROLLABLE);

			// Create text label inside container
			g_warning_data[field_id].text_label = lv_label_create(g_warning_data[field_id].container);
			lv_obj_set_style_text_color(g_warning_data[field_id].text_label, PALETTE_YELLOW, 0);
			lv_obj_set_style_text_font(g_warning_data[field_id].text_label, &lv_font_montserrat_20, 0);
			lv_obj_set_style_text_align(g_warning_data[field_id].text_label, LV_TEXT_ALIGN_CENTER, 0);
			lv_label_set_text(g_warning_data[field_id].text_label, "MAX");
			lv_obj_align(g_warning_data[field_id].text_label, LV_ALIGN_TOP_MID, 0, 22);

			// Create value label inside container
			g_warning_data[field_id].value_label = lv_label_create(g_warning_data[field_id].container);
			lv_obj_set_style_text_color(g_warning_data[field_id].value_label, PALETTE_YELLOW, 0);
			lv_obj_set_style_text_font(g_warning_data[field_id].value_label, &lv_font_noplato_24, 0);
			lv_obj_set_style_text_align(g_warning_data[field_id].value_label, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_align(g_warning_data[field_id].value_label, LV_ALIGN_BOTTOM_MID, 0, -22);

			char value_text[16];
			// For MAX warning, show the actual constraint value being used
			if (data->field_index == FIELD_GAUGE_LOW) {
				// For GAUGE LOW fields, MAX constraint is current GAUGE HIGH value
				int gauge_index = data->gauge_index;
				float actual_max_value = modal->field_data[gauge_index * 5 + FIELD_GAUGE_HIGH].current_value;
				snprintf(value_text, sizeof(value_text), "%.1f", actual_max_value);
			} else if (data->field_index == FIELD_ALERT_LOW) {
				// For ALERT LOW fields, MAX constraint is current ALERT HIGH value
				int gauge_index = data->gauge_index;
				float actual_max_value = modal->field_data[gauge_index * 5 + FIELD_ALERT_HIGH].current_value;
				snprintf(value_text, sizeof(value_text), "%.1f", actual_max_value);
			} else {
				// For other fields, use config max
				snprintf(value_text, sizeof(value_text), "%.1f", data->max_value);
			}
			lv_label_set_text(g_warning_data[field_id].value_label, value_text);
		} else if (is_below_min) {
			// Create container that encompasses both text and value
			g_warning_data[field_id].container = lv_obj_create(modal->background);
			lv_obj_set_size(g_warning_data[field_id].container, 63, 80); // Larger to fit both text and value, 3px wider for negative values
			lv_obj_set_style_bg_color(g_warning_data[field_id].container, PALETTE_BLACK, 0); // Black background
			lv_obj_set_style_bg_opa(g_warning_data[field_id].container, LV_OPA_COVER, 0);
			lv_obj_set_style_border_color(g_warning_data[field_id].container, PALETTE_YELLOW, 0); // Yellow border
			lv_obj_set_style_border_width(g_warning_data[field_id].container, 2, 0);
			lv_obj_set_style_radius(g_warning_data[field_id].container, 8, 0);
			lv_obj_clear_flag(g_warning_data[field_id].container, LV_OBJ_FLAG_SCROLLABLE);

			// Create text label inside container
			g_warning_data[field_id].text_label = lv_label_create(g_warning_data[field_id].container);
			lv_obj_set_style_text_color(g_warning_data[field_id].text_label, PALETTE_YELLOW, 0);
			lv_obj_set_style_text_font(g_warning_data[field_id].text_label, &lv_font_montserrat_20, 0);
			lv_obj_set_style_text_align(g_warning_data[field_id].text_label, LV_TEXT_ALIGN_CENTER, 0);
			lv_label_set_text(g_warning_data[field_id].text_label, "MIN");
			lv_obj_align(g_warning_data[field_id].text_label, LV_ALIGN_TOP_MID, 0, 22);

			// Create value label inside container
			g_warning_data[field_id].value_label = lv_label_create(g_warning_data[field_id].container);
			lv_obj_set_style_text_color(g_warning_data[field_id].value_label, PALETTE_YELLOW, 0);
			lv_obj_set_style_text_font(g_warning_data[field_id].value_label, &lv_font_noplato_24, 0);
			lv_obj_set_style_text_align(g_warning_data[field_id].value_label, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_align(g_warning_data[field_id].value_label, LV_ALIGN_BOTTOM_MID, 0, -22);

			char value_text[16];
			// For MIN warning, show the actual constraint value being used
			if (data->field_index == FIELD_GAUGE_HIGH) {
				// For GAUGE HIGH fields, MIN constraint is current GAUGE LOW value
				int gauge_index = data->gauge_index;
				float actual_min_value = modal->field_data[gauge_index * 5 + FIELD_GAUGE_LOW].current_value;
				snprintf(value_text, sizeof(value_text), "%.1f", actual_min_value);
			} else if (data->field_index == FIELD_ALERT_HIGH) {
				// For ALERT HIGH fields, MIN constraint is current ALERT LOW value
				int gauge_index = data->gauge_index;
				float actual_min_value = modal->field_data[gauge_index * 5 + FIELD_ALERT_LOW].current_value;
				snprintf(value_text, sizeof(value_text), "%.1f", actual_min_value);
			} else {
				// For other fields, use config min
				snprintf(value_text, sizeof(value_text), "%.1f", data->min_value);
			}
			lv_label_set_text(g_warning_data[field_id].value_label, value_text);
		} else {

			// Fallback - shouldn't happen
			lv_label_set_text(g_warning_data[field_id].text_label, "ERROR");
		}
	}

	// Position warning based on type:
	// - Max warnings: Always above (out of range)
	// - Min warnings: Always below (below minimum)
	// - Baseline warnings: Always above (out of range)
	int offset_distance = 25;
	if (is_above_max || is_baseline_warning) {
		if (is_baseline_warning) {
			// Baseline warnings: show text label above field
			lv_obj_align_to(
				g_warning_data[field_id].text_label, ui->label, LV_ALIGN_OUT_TOP_MID,
				0, -offset_distance
			);
		} else {
			// Max warnings: show container above field
			lv_obj_align_to(
				g_warning_data[field_id].container, ui->label, LV_ALIGN_OUT_TOP_MID,
				0, -offset_distance
			);
		}
		printf("[I] alerts_modal: Positioned %s warning above field\n",
			is_baseline_warning ? "baseline" : "max");
	} else {
		// Min warnings: show container below field
		lv_obj_align_to(
			g_warning_data[field_id].container, ui->label, LV_ALIGN_OUT_BOTTOM_MID,
			0, offset_distance
		);
		printf("[I] alerts_modal: Positioned min warning below field\n");
	}

	// Store warning data
	g_warning_data[field_id].modal = modal;
	g_warning_data[field_id].highlighted_field_id = highlighted_field_id;
	g_warning_data[field_id].is_baseline_warning = is_baseline_warning;

	// Highlight the corresponding field for baseline warnings
	if (is_baseline_warning && highlighted_field_id >= 0) {
		highlight_field_for_warning(modal, highlighted_field_id);
	}

	// Create timer to hide warning after 2 seconds
	g_warning_data[field_id].timer = lv_timer_create(warning_timer_callback, 5000, &g_warning_data[field_id]);
	lv_timer_set_repeat_count(g_warning_data[field_id].timer, 1); // Run only once
	g_warning_data[field_id].field_id = field_id;

	// Don't reset the numberpad input to allow continuous typing for all fields
	// Only reset when the warning timer expires

	// Update field display to show the out-of-range value
	update_field_display(modal, field_id);

	printf("[I] alerts_modal: Showing warning for out-of-range value: %.1f (timer created)\n", out_of_range_value);
}

static void hide_out_of_range_warning(alerts_modal_t* modal, int field_id)
{
	if (!modal || field_id < 0 || field_id >= modal->total_field_count) return;

	printf("[I] alerts_modal: Hiding warning for field %d\n", field_id);

	// Mark field as no longer out of range
	modal->field_data[field_id].is_out_of_range = false;

	// Hide warning UI elements
	if (g_warning_data[field_id].text_label) {
		lv_obj_add_flag(g_warning_data[field_id].text_label, LV_OBJ_FLAG_HIDDEN);
	}
	if (g_warning_data[field_id].container) {
		lv_obj_add_flag(g_warning_data[field_id].container, LV_OBJ_FLAG_HIDDEN);
	}
	if (g_warning_data[field_id].value_label) {
		lv_obj_add_flag(g_warning_data[field_id].value_label, LV_OBJ_FLAG_HIDDEN);
	}

	// Destroy timer if it exists
	if (g_warning_data[field_id].timer) {
		lv_timer_del(g_warning_data[field_id].timer);
		g_warning_data[field_id].timer = NULL;
	}

	// Store the highlighted field ID before clearing warning data
	int highlighted_field_id = g_warning_data[field_id].highlighted_field_id;

	// Unhighlight any highlighted field for baseline warnings
	if (highlighted_field_id >= 0) {
		modal->field_data[highlighted_field_id].is_warning_highlighted = false;
		update_all_field_borders(modal);
	}

	// Clear warning data
	g_warning_data[field_id].text_label = NULL;
	g_warning_data[field_id].container = NULL;
	g_warning_data[field_id].value_label = NULL;
	g_warning_data[field_id].highlighted_field_id = -1;
	g_warning_data[field_id].is_baseline_warning = false;
	g_warning_data[field_id].modal = NULL;

	// Update field display for the baseline field
	update_field_display(modal, field_id);
	// Note: update_all_field_borders is called by the caller to avoid recursive calls

	printf("[I] alerts_modal: Hiding warning for field %d\n", field_id);
}

static void warning_timer_callback(lv_timer_t* timer)
{
	warning_data_t* data = (warning_data_t*)lv_timer_get_user_data(timer);
	if (!data || !data->modal) return;

	printf("[I] alerts_modal: Warning timer callback called for field %d\n", data->field_id);

	// Revert the field value to the clamped value (midpoint between min and max)
	field_data_t* field_data = &data->modal->field_data[data->field_id];
	printf("[I] alerts_modal: Timer callback - field %d, stored clamped value: %.1f\n", data->field_id, data->clamped_value);
	field_data->current_value = data->clamped_value;
	field_data->is_out_of_range = false;

	// For baseline "UPDATED" warnings, keep the yellow highlighting
	// Only clear it for user input warnings
	if (field_data->field_index != FIELD_GAUGE_BASELINE) {
		field_data->is_warning_highlighted = false;
	}

	// Reset the numberpad input to show the clamped value (for both baseline and non-baseline fields)
	if (data->modal->numberpad && data->modal->numberpad->is_visible) {
		// Reset negative state silently to avoid triggering value changed callback
		data->modal->numberpad->is_negative = false;

		// Update numberpad to show the clamped value so subsequent inputs start fresh
		char clamped_value_str[16];
		snprintf(clamped_value_str, sizeof(clamped_value_str), "%.1f", data->clamped_value);
		numberpad_set_value_for_fresh_input(data->modal->numberpad, clamped_value_str);
	}

	// Hide the warning UI elements
	hide_out_of_range_warning(data->modal, data->field_id);

	// Update the field display to show the clamped value
	update_field_display(data->modal, data->field_id);
	update_all_field_borders(data->modal);

	printf("[I] alerts_modal: Warning timer expired, reverted to clamped value %.1f\n", data->clamped_value);
}

// Create gauge section (maintains original visual design)
static void create_gauge_section(alerts_modal_t* modal, int gauge, lv_obj_t* parent, int y_offset)
{
	if (!modal || gauge >= modal->config.gauge_count) return;

	// Gauge section container
	modal->gauge_sections[gauge] = lv_obj_create(parent);
	lv_obj_set_size(modal->gauge_sections[gauge], LV_PCT(100), 200);  // Increased from 160 to 200
	lv_obj_set_pos(modal->gauge_sections[gauge], 0, y_offset);
	lv_obj_set_style_bg_opa(modal->gauge_sections[gauge], LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(modal->gauge_sections[gauge], 2, 0);
	lv_obj_set_style_border_color(modal->gauge_sections[gauge], PALETTE_WHITE, 0);
	lv_obj_set_style_pad_all(modal->gauge_sections[gauge], 1, 0);  // Minimal padding for maximum space
	lv_obj_clear_flag(modal->gauge_sections[gauge], LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(modal->gauge_sections[gauge], LV_OBJ_FLAG_EVENT_BUBBLE);

	// Gauge Title - positioned inline with border like detail_screen
	// Create as child of root modal container so it's not clipped by gauge_sections
	modal->gauge_titles[gauge] = lv_label_create(modal->content_container);
	lv_label_set_text(modal->gauge_titles[gauge], modal->config.gauges[gauge].name);
	lv_obj_set_style_text_color(modal->gauge_titles[gauge], DEFAULT_GAUGE_TITLE_TEXT_COLOR, 0);
	lv_obj_set_style_text_font(modal->gauge_titles[gauge], &lv_font_montserrat_16, 0);
	lv_obj_set_style_bg_color(modal->gauge_titles[gauge], DEFAULT_GAUGE_TITLE_BACKGROUND_COLOR, 0); // Blue background to obscure border
	lv_obj_set_style_bg_opa(modal->gauge_titles[gauge], LV_OPA_COVER, 0);
	lv_obj_set_style_pad_left(modal->gauge_titles[gauge], 8, 0);
	lv_obj_set_style_pad_right(modal->gauge_titles[gauge], 8, 0);
	lv_obj_set_style_pad_top(modal->gauge_titles[gauge], 2, 0);
	lv_obj_set_style_pad_bottom(modal->gauge_titles[gauge], 2, 0);
	lv_obj_set_style_radius(modal->gauge_titles[gauge], 5, 0);
	lv_obj_align_to(modal->gauge_titles[gauge], modal->gauge_sections[gauge], LV_ALIGN_OUT_TOP_RIGHT, -10, 10);

	// Create ALERTS group - 2 units wide using flexbox, balanced margins
	modal->alert_groups[gauge] = lv_obj_create(modal->gauge_sections[gauge]);

	// Layout
	lv_obj_set_size(modal->alert_groups[gauge], LV_PCT(38), 140);  // Reduced width to make room for GAUGE
	lv_obj_set_pos(modal->alert_groups[gauge], 8, 32);  // 8px left margin (A), reduced y by 3px

	lv_obj_set_layout(modal->alert_groups[gauge], LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(modal->alert_groups[gauge], LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(modal->alert_groups[gauge], LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// Style
	lv_obj_set_style_bg_opa(modal->alert_groups[gauge], LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(modal->alert_groups[gauge], 2, 0);
	lv_obj_set_style_border_color(modal->alert_groups[gauge], PALETTE_WHITE, 0);
	lv_obj_set_style_radius(modal->alert_groups[gauge], 5, 0);
	lv_obj_set_style_pad_all(modal->alert_groups[gauge], 0, 0);  // No internal padding for maximum space

	// Properties
	lv_obj_clear_flag(modal->alert_groups[gauge], LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(modal->alert_groups[gauge], LV_OBJ_FLAG_EVENT_BUBBLE);


	// ALERTS group title - positioned inline with border like detail_screen // do not style here
	modal->alert_titles[gauge] = lv_label_create(modal->gauge_sections[gauge]);
	lv_label_set_text(modal->alert_titles[gauge], "ALERTS");
	lv_obj_set_style_text_color(modal->alert_titles[gauge], DEFAULT_FIELD_ALERT_GROUP_TITLE_TEXT_COLOR, 0);
	lv_obj_set_style_text_font(modal->alert_titles[gauge], &lv_font_montserrat_12, 0);
	lv_obj_set_style_bg_color(modal->alert_titles[gauge], PALETTE_RED, 0); // Black background to obscure border
	lv_obj_set_style_bg_opa(modal->alert_titles[gauge], LV_OPA_COVER, 0);
	lv_obj_set_style_pad_left(modal->alert_titles[gauge], 8, 0);
	lv_obj_set_style_pad_right(modal->alert_titles[gauge], 8, 0);
	lv_obj_set_style_pad_top(modal->alert_titles[gauge], 2, 0);
	lv_obj_set_style_pad_bottom(modal->alert_titles[gauge], 2, 0);
	// lv_obj_set_style_border_width(modal->alert_titles[gauge], 0, 0);
	lv_obj_set_style_radius(modal->alert_titles[gauge], 3, 0);


	lv_obj_align_to(modal->alert_titles[gauge], modal->alert_groups[gauge], LV_ALIGN_OUT_TOP_LEFT, 10, 10);

	// Create GAUGE group - 3 units wide using flexbox, positioned to the right of ALERTS
	modal->gauge_groups[gauge] = lv_obj_create(modal->gauge_sections[gauge]);

	// Layout
	lv_obj_set_size(modal->gauge_groups[gauge], LV_PCT(57), 140);  // Increased width to fill remaining space
	lv_obj_align_to(modal->gauge_groups[gauge], modal->alert_groups[gauge], LV_ALIGN_OUT_RIGHT_MID, 8, 0); // Position to the right of ALERTS group with 8px gap
	lv_obj_set_layout(modal->gauge_groups[gauge], LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(modal->gauge_groups[gauge], LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(modal->gauge_groups[gauge], LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	// Style
	lv_obj_set_style_bg_opa(modal->gauge_groups[gauge], LV_OPA_TRANSP, 0);
	lv_obj_set_style_border_width(modal->gauge_groups[gauge], 2, 0);
	lv_obj_set_style_border_color(modal->gauge_groups[gauge], PALETTE_WHITE, 0);
	lv_obj_set_style_radius(modal->gauge_groups[gauge], 5, 0);
	lv_obj_set_style_pad_all(modal->gauge_groups[gauge], 0, 0);  // No internal padding for maximum space

	// Properties
	lv_obj_clear_flag(modal->gauge_groups[gauge], LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(modal->gauge_groups[gauge], LV_OBJ_FLAG_EVENT_BUBBLE);


	// GAUGE group title - positioned inline with border like detail_screen
	modal->gauge_group_title[gauge] = lv_label_create(modal->gauge_sections[gauge]);
	lv_label_set_text(modal->gauge_group_title[gauge], "GAUGE");
	lv_obj_set_style_text_color(modal->gauge_group_title[gauge], DEFAULT_FIELD_GAUGE_GROUP_TITLE_TEXT_COLOR, 0);
	lv_obj_set_style_text_font(modal->gauge_group_title[gauge], &lv_font_montserrat_12, 0);
	lv_obj_set_style_bg_color(modal->gauge_group_title[gauge], lv_color_hex(0x8F4700), 0); // Black background to obscure border
	lv_obj_set_style_bg_opa(modal->gauge_group_title[gauge], LV_OPA_COVER, 0);
	lv_obj_set_style_pad_left(modal->gauge_group_title[gauge], 8, 0);
	lv_obj_set_style_pad_right(modal->gauge_group_title[gauge], 8, 0);
	lv_obj_set_style_pad_top(modal->gauge_group_title[gauge], 2, 0);
	lv_obj_set_style_pad_bottom(modal->gauge_group_title[gauge], 2, 0);
	lv_obj_set_style_radius(modal->gauge_group_title[gauge], 3, 0);
	lv_obj_align_to(modal->gauge_group_title[gauge], modal->gauge_groups[gauge], LV_ALIGN_OUT_TOP_LEFT, 10, 10);
}



// Public API functions
alerts_modal_t* alerts_modal_create(const alerts_modal_config_t* config, void (*on_close_callback)(void))
{
	printf("[I] alerts_modal: Creating generic alerts modal\n");

	if (!config) {
		printf("[E] alerts_modal: Configuration is required\n");
		return NULL;
	}

	if (config->gauge_count <= 0 || !config->gauges) {
		printf("[E] alerts_modal: Invalid gauge configuration\n");
		return NULL;
	}

	alerts_modal_t* modal = malloc(sizeof(alerts_modal_t));
	if (!modal) {
		printf("[E] alerts_modal: Failed to allocate memory for alerts modal\n");
		return NULL;
	}

	memset(modal, 0, sizeof(alerts_modal_t));

	// Store configuration
	modal->config = *config;
	modal->total_field_count = config->gauge_count * 5; // 5 fields per gauge

	// Allocate dynamic arrays
	modal->gauge_sections = calloc(config->gauge_count, sizeof(lv_obj_t*));
	modal->alert_groups = calloc(config->gauge_count, sizeof(lv_obj_t*));
	modal->gauge_groups = calloc(config->gauge_count, sizeof(lv_obj_t*));
	modal->gauge_titles = calloc(config->gauge_count, sizeof(lv_obj_t*));
	modal->alert_titles = calloc(config->gauge_count, sizeof(lv_obj_t*));
	modal->gauge_group_title = calloc(config->gauge_count, sizeof(lv_obj_t*));
	modal->field_ui = calloc(modal->total_field_count, sizeof(field_ui_t));
	modal->field_data = calloc(modal->total_field_count, sizeof(field_data_t));

	if (!modal->gauge_sections || !modal->alert_groups || !modal->gauge_groups ||
		!modal->gauge_titles || !modal->alert_titles || !modal->gauge_group_title ||
		!modal->field_ui || !modal->field_data) {
		printf("[E] alerts_modal: Failed to allocate memory for dynamic arrays\n");
		alerts_modal_destroy(modal);
		return NULL;
	}

	modal->on_close = on_close_callback;
	modal->current_field_id = -1;

	// Create modal background - truly full screen, no padding
	modal->background = lv_obj_create(lv_screen_active());
	lv_obj_set_size(modal->background, LV_PCT(100), LV_PCT(100));
	lv_obj_set_pos(modal->background, 0, 0);
	lv_obj_set_style_bg_color(modal->background, PALETTE_BLACK, 0);
	lv_obj_set_style_bg_opa(modal->background, LV_OPA_COVER, 0); // Use full opacity for better performance
	lv_obj_set_style_border_width(modal->background, 0, 0);
	lv_obj_set_style_pad_all(modal->background, 0, 0);

	// Create content container - truly full width, no padding from background
	modal->content_container = lv_obj_create(modal->background);
	lv_obj_set_size(modal->content_container, LV_PCT(100), LV_PCT(100));
	lv_obj_align(modal->content_container, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_bg_color(modal->content_container, PALETTE_BLACK, 0);
	lv_obj_set_style_border_color(modal->content_container, PALETTE_BLACK, 0);
	lv_obj_set_style_border_width(modal->content_container, 0, 0);
	lv_obj_set_style_pad_all(modal->content_container, 0, 10);

	// Create gauge sections
		for (int i = 0; i < config->gauge_count; i++) {
		create_gauge_section(modal, i, modal->content_container, i * 240);  // Increased for better visual separation
	}

	// Create close button - made taller to fit to bottom of last gauge
	modal->close_button = lv_button_create(modal->content_container);
	lv_obj_set_size(modal->close_button, 100, 60);  // Increased height from 40 to 60
	lv_obj_align(modal->close_button, LV_ALIGN_BOTTOM_RIGHT, 0, -10);
	lv_obj_set_style_bg_color(modal->close_button, lv_color_hex(0x555555), 0);

	lv_obj_t *close_label = lv_label_create(modal->close_button);
	lv_label_set_text(close_label, "Close");
	lv_obj_set_style_text_color(close_label, PALETTE_WHITE, 0);
	lv_obj_center(close_label);

	// Add click event to close button
	lv_obj_add_event_cb(modal->close_button, close_button_cb, LV_EVENT_CLICKED, modal);

	// Create cancel button - same height as close button
	modal->cancel_button = lv_button_create(modal->content_container);
	lv_obj_set_size(modal->cancel_button, 100, 60);  // Same height as close button
	lv_obj_align(modal->cancel_button, LV_ALIGN_BOTTOM_RIGHT, -110, -10);  // Positioned to the left of close button
	lv_obj_set_style_bg_color(modal->cancel_button, lv_color_hex(0x666666), 0);

	lv_obj_t *cancel_label = lv_label_create(modal->cancel_button);
	lv_label_set_text(cancel_label, "Cancel");
	lv_obj_set_style_text_color(cancel_label, PALETTE_WHITE, 0);
	lv_obj_center(cancel_label);

	// Add click event to cancel button
	lv_obj_add_event_cb(modal->cancel_button, cancel_button_cb, LV_EVENT_CLICKED, modal);

	// Add field click handler to modal containers to handle all clicks
	lv_obj_add_event_cb(modal->background, field_click_handler, LV_EVENT_CLICKED, modal);
	lv_obj_add_event_cb(modal->content_container, field_click_handler, LV_EVENT_CLICKED, modal);


	// Initialize all field data with proper group types and field types
	for (int gauge = 0; gauge < config->gauge_count; gauge++) {
		for (int field_type = 0; field_type < 5; field_type++) { // 5 fields per gauge
			int field_id = gauge * 5 + field_type;
			initialize_field_data(&modal->field_data[field_id], gauge, field_type, config);
		}
	}

	// Create field containers and populate fields
	for (int field_id = 0; field_id < modal->total_field_count; field_id++) {

		field_data_t* data = &modal->field_data[field_id];

		// Create the field container for this field
		lv_obj_t* field_container = NULL;
		if (data->group_type == GROUP_ALERTS) {
			// For ALERTS group, create field container
			if (data->field_index < 2) { // FIELD_ALERT_LOW or FIELD_ALERT_HIGH

				field_container = lv_obj_create(modal->alert_groups[data->gauge_index]);
			}
		} else if (data->group_type == GROUP_GAUGE) {
			// For GAUGE group, create field container
			if (data->field_index >= 2 && data->field_index < 5) { // FIELD_GAUGE_LOW, FIELD_GAUGE_BASELINE, FIELD_GAUGE_HIGH

				field_container = lv_obj_create(modal->gauge_groups[data->gauge_index]);
			}
		}

		lv_obj_clear_flag(field_container, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_flag(field_container, LV_OBJ_FLAG_EVENT_BUBBLE);

		if (!field_container) continue;

		// Field Container ( Button and Title)
		lv_obj_set_size(field_container, 63, 82); // 3px wider for negative values

		// Layout
		lv_obj_set_layout(field_container, LV_LAYOUT_FLEX);
		lv_obj_set_flex_flow(field_container, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(field_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		// Style
		lv_obj_set_style_bg_opa(field_container, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(field_container, 0, 0);
		lv_obj_set_style_border_color(field_container, PALETTE_WHITE, 0);
		lv_obj_set_style_radius(field_container, 0, 0);
		lv_obj_set_style_pad_all(field_container, 1, 1);  // Add padding to accommodate child border

		// Field Value (Button)
		lv_obj_t* field_value_container = lv_obj_create( field_container );
		lv_obj_set_size(field_value_container, 63, 60); // 3px wider for negative values
		lv_obj_set_style_border_color(field_value_container, PALETTE_WHITE, 0);
		lv_obj_set_style_border_width(field_value_container, 2, 0);
		lv_obj_set_style_border_opa(field_value_container, LV_OPA_COVER, 0);
		lv_obj_set_style_radius(field_value_container, 8, 0);

		lv_obj_clear_flag(field_value_container, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_add_flag(field_value_container, LV_OBJ_FLAG_EVENT_BUBBLE);

		// Numeric Value (Button)
		lv_obj_t* number_label = lv_label_create(field_value_container);
		lv_obj_set_style_text_color(number_label, lv_color_hex(0xfffff), 0);
		lv_obj_set_style_bg_color(number_label, PALETTE_RED, 0);
		lv_obj_set_style_bg_opa(field_value_container, LV_OPA_COVER, 0);
		lv_obj_set_style_bg_color(field_value_container, PALETTE_RED, 0);
		lv_obj_set_style_text_font(number_label, &lv_font_noplato_24, 0);
		lv_obj_set_style_pad_bottom(number_label, 0, 0);
		lv_obj_center(number_label);

		// Title
		lv_obj_t*title_label = lv_label_create( field_container );

		lv_label_set_text(title_label, config->gauges[data->gauge_index].fields[data->field_index].name);
		lv_obj_set_style_text_color(title_label, PALETTE_WHITE, 0);
		lv_obj_set_style_text_font(title_label, &lv_font_montserrat_12, 0);
		lv_obj_set_style_bg_color(title_label, PALETTE_BLACK, 0);
		lv_obj_set_style_bg_opa(title_label, LV_OPA_COVER, 0);
		lv_obj_set_style_pad_left(title_label, 4, 0);
		lv_obj_set_style_pad_right(title_label, 4, 0);
		lv_obj_set_style_pad_top(title_label, 0, 0);
		lv_obj_set_style_pad_bottom(title_label, 2, 0);
		lv_obj_set_style_margin_top(title_label, -8, 0);
		lv_obj_set_style_radius(title_label, 3, 0);

		// map field UI values
		modal->field_ui[field_id].button = field_value_container;
		modal->field_ui[field_id].label = number_label;
		modal->field_ui[field_id].title = title_label;

		// Load value from device state
		float loaded_value = get_device_state_value(modal, data->gauge_index, data->field_index);
		data->current_value = loaded_value;
		data->original_value = loaded_value;

		// Update display and border
		update_field_display(modal, field_id);
		update_all_field_borders(modal);
		// Note: update_all_field_borders will be called at the end of field creation
	}

	// Apply all border styling after field creation
	update_all_field_borders(modal);

	// Initially hidden
	lv_obj_add_flag(modal->background, LV_OBJ_FLAG_HIDDEN);
	modal->is_visible = false;

	printf("[I] alerts_modal: Properly refactored alerts modal created\n");
	return modal;
}

void alerts_modal_show(alerts_modal_t* modal)
{
	if (!modal) {
		printf("[W] alerts_modal: Cannot show NULL modal\n");
		return;
	}

	if (!modal->is_visible) {
		printf("[I] alerts_modal: Showing properly refactored alerts modal\n");
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
		printf("[I] alerts_modal: Hiding properly refactored alerts modal\n");
		close_current_field(modal);
		lv_obj_add_flag(modal->background, LV_OBJ_FLAG_HIDDEN);
		modal->is_visible = false;
	}
}

void alerts_modal_destroy(alerts_modal_t* modal)
{
	if (!modal) {
		printf("[W] alerts_modal: Cannot destroy NULL modal\n");
		return;
	}

	printf("[I] alerts_modal: Destroying properly refactored alerts modal\n");

	// Clear all warnings and destroy timers
	for (int field_id = 0; field_id < modal->total_field_count; field_id++) {
		hide_out_of_range_warning(modal, field_id);
	}

	if (modal->numberpad) {
		numberpad_hide(modal->numberpad);
	}

	if (modal->background) {
		lv_obj_del_async(modal->background);
	}

	// Free dynamically allocated arrays
	free(modal->gauge_sections);
	free(modal->alert_groups);
	free(modal->gauge_groups);
	free(modal->gauge_titles);
	free(modal->alert_titles);
	free(modal->gauge_group_title);
	free(modal->field_ui);
	free(modal->field_data);

	free(modal);
}

bool alerts_modal_is_visible(alerts_modal_t* modal)
{
	if (!modal) {
		return false;
	}
	return modal->is_visible;
}

void alerts_modal_refresh_gauges_and_alerts(alerts_modal_t* modal)
{
	printf("[I] alerts_modal: Refreshing gauges and alerts after modal changes\n");

	if (!modal || !modal->config.refresh_cb) {
		printf("[W] alerts_modal: No refresh callback provided\n");
		return;
	}

	// Call the provided refresh callback
	modal->config.refresh_cb();

	printf("[I] alerts_modal: Gauge and alert refresh complete\n");
}
