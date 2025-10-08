#include <stdio.h>
#include "alerts_modal.h"

#include "../../../state/device_state.h"
#include "../../../fonts/lv_font_noplato_24.h"
#include "../numberpad/numberpad.h"
#include "../gauges/bar_graph_gauge.h"
#include "../../../screens/detail_screen/detail_screen.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static const char *TAG = "alerts_modal";

// Color Palette
#define PALETTE_WHITE lv_color_hex(0xFFFFFF)
#define PALETTE_BLACK lv_color_hex(0x000000)
#define PALETTE_GRAY lv_color_hex(0x8c8c8c)
#define PALETTE_YELLOW lv_color_hex(0xFFD700)
#define PALETTE_RED lv_color_hex(0xff0000)
#define PALETTE_GREEN lv_color_hex(0x00ff00)
#define PALETTE_BLUE lv_color_hex(0x004557)
#define PALETTE_CYAN lv_color_hex(0x00FFFF)
#define PALETTE_DARK_GRAY lv_color_hex(0x3F3F3F)
#define PALETTE_BROWN lv_color_hex(0x793100)

// Shared Colors
#define BORDER_COLOR PALETTE_WHITE
#define TEXT_COLOR_LIGHT PALETTE_WHITE
#define TEXT_COLOR_DARK PALETTE_DARK_GRAY
#define BACKGROUND_COLOR PALETTE_BLACK

// #### Default State Colors ####

#define DEFAULT_WARNING_BORDER_COLOR PALETTE_YELLOW
#define DEFAULT_WARNING_TEXT_COLOR PALETTE_YELLOW

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


// Field validation ranges (min, max, default)
static const float field_ranges[FIELD_COUNT_PER_GAUGE][3] = {
	{0.0f, 20.0f, 10.0f},  // FIELD_ALERT_LOW: 0-20V, default 10V
	{0.0f, 20.0f, 15.0f},  // FIELD_ALERT_HIGH: 0-20V, default 15V
	{0.0f, 20.0f, 10.0f},  // FIELD_GAUGE_LOW: 0-20V, default 10V
	{0.0f, 20.0f, 13.0f},  // FIELD_GAUGE_BASELINE: 0-20V, default 13V
	{0.0f, 20.0f, 15.0f}   // FIELD_GAUGE_HIGH: 0-20V, default 15V
};

// Warning system for out-of-range values
typedef struct {
	lv_obj_t* text_label;  // Label for "OVER"/"UNDER"/"MAX"/"MIN" text
	lv_obj_t* value_label; // Label for the numeric value (for max/min warnings)
	lv_obj_t* container;   // Container for max/min warnings (matches value field style)
	lv_timer_t* timer;
	int field_id;  // Field ID instead of field object
	float original_value;  // Store the original out-of-range value for display
	alerts_modal_t* modal;  // Reference to modal for updating display
	int highlighted_field_id;  // Field ID to highlight (for baseline warnings)
	bool is_baseline_warning;  // Whether this is a baseline warning
} warning_data_t;

static warning_data_t g_warning_data[GAUGE_COUNT * FIELD_COUNT_PER_GAUGE] = {0};

// gauge names and field names for UI
static const char* gauge_names[] = {"STARTER (V)", "HOUSE (V)", "SOLAR (W)"};
static const char* field_names[] = {"LOW", "HIGH", "LOW", "BASE", "HIGH"};
static const char* group_names[] = {"ALERTS", "GAUGE"};

// Forward declarations
static void close_button_cb(lv_event_t *e);
static void cancel_button_cb(lv_event_t *e);
static void field_click_handler(lv_event_t *e);
static void initialize_field_data(field_data_t* field_data, int gauge, int field_type);
static void update_field_display(alerts_modal_t* modal, int field_id);
static void update_field_border(alerts_modal_t* modal, int field_id);
static void update_all_field_borders(alerts_modal_t* modal);
static void update_current_field_border(alerts_modal_t* modal);
static int find_field_by_button(alerts_modal_t* modal, lv_obj_t* button);
static void close_current_field(alerts_modal_t* modal);
static void highlight_field_for_warning(alerts_modal_t* modal, int field_id);
static void unhighlight_field_for_warning(alerts_modal_t* modal, int field_id);
static void create_gauge_section(alerts_modal_t* modal, gauge_type_t gauge, lv_obj_t* parent, int y_offset);
static void numberpad_value_changed(const char* value, void* user_data);
static void numberpad_clear(void* user_data);
static void numberpad_enter(const char* value, void* user_data);
static void numberpad_cancel(void* user_data);
static void show_out_of_range_warning(alerts_modal_t* modal, int field_id, float out_of_range_value);
static void hide_out_of_range_warning(int field_id);
static void warning_timer_callback(lv_timer_t* timer);

// Initialize field data (complete state management)
static void initialize_field_data(field_data_t* field_data, int gauge, int field_type)
{
	if (!field_data) return;

	// Field identification
	field_data->gauge_index = gauge;
	field_data->field_index = field_type;
	field_data->group_type = (field_type < 2) ? GROUP_ALERTS : GROUP_GAUGE;

	// Set value ranges based on field type
	field_data->min_value = field_ranges[field_type][0];
	field_data->max_value = field_ranges[field_type][1];
	field_data->default_value = field_ranges[field_type][2];
	field_data->current_value = field_data->default_value;
	field_data->original_value = field_data->default_value;

	// Initialize state flags
	field_data->is_being_edited = false;
	field_data->has_changed = false;
	field_data->is_out_of_range = false;

	// Initialize UI state
	field_data->border_color = PALETTE_WHITE;  // White border
	field_data->border_width = 2;                       // Default border width
	field_data->text_color = PALETTE_WHITE;    // White text
	field_data->text_background_color = PALETTE_BLACK;  // Black text background
}

// Update field display value using field ID
static void update_field_display(alerts_modal_t* modal, int field_id)
{
	if (!modal || field_id < 0 || field_id >= GAUGE_COUNT * FIELD_COUNT_PER_GAUGE) return;

	field_ui_t* ui = &modal->field_ui[field_id];
	field_data_t* data = &modal->field_data[field_id];

	if (!ui->label) return;

	char value_str[16];

	// If there's an active warning for this field, show the original out-of-range value
	if (g_warning_data[field_id].text_label != NULL) {
		snprintf(value_str, sizeof(value_str), "%.1f", g_warning_data[field_id].original_value);
	} else {
		snprintf(value_str, sizeof(value_str), "%.1f", data->current_value);
	}

	lv_label_set_text(ui->label, value_str);
}

// Update field border based on state using field ID
static void update_field_border(alerts_modal_t* modal, int field_id)
{
	if (!modal || field_id < 0 || field_id >= GAUGE_COUNT * FIELD_COUNT_PER_GAUGE) return;

	field_data_t* data = &modal->field_data[field_id];

	// Update state in field_data only - UI updates are handled by update_all_field_borders
	if (data->is_out_of_range) {
		// Red border for out of range
		data->border_color = PALETTE_RED;
		data->border_width = 2;
	} else if (data->has_changed) {
		// Green border for changed values
		data->border_color = PALETTE_GREEN;
		data->border_width = 2;
	} else {
		// White border for default
		data->border_color = PALETTE_WHITE;
		data->border_width = 1;
	}

	// Always reset text color to white
	data->text_color = PALETTE_WHITE;
}

// Check if field value equals original value
static bool field_value_equals_original(field_data_t* data)
{
	if (!data) return false;
	return (fabs(data->current_value - data->original_value) < 0.01f);
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

// Get device state value for a specific gauge and field type
static float get_device_state_value(int gauge, int field_type)
{
	// Load alert thresholds
	if (field_type == FIELD_ALERT_LOW) {
		if (gauge == GAUGE_STARTER) return (float)device_state_get_starter_alert_low_voltage_v();
		else if (gauge == GAUGE_HOUSE) return (float)device_state_get_house_alert_low_voltage_v();
		else if (gauge == GAUGE_SOLAR) return (float)device_state_get_solar_alert_low_voltage_v();
	} else if (field_type == FIELD_ALERT_HIGH) {
		if (gauge == GAUGE_STARTER) return (float)device_state_get_starter_alert_high_voltage_v();
		else if (gauge == GAUGE_HOUSE) return (float)device_state_get_house_alert_high_voltage_v();
		else if (gauge == GAUGE_SOLAR) return (float)device_state_get_solar_alert_high_voltage_v();
	}
	// Load gauge ranges
	else if (field_type == FIELD_GAUGE_LOW) {
		if (gauge == GAUGE_STARTER) return device_state_get_starter_min_voltage_v();
		else if (gauge == GAUGE_HOUSE) return device_state_get_house_min_voltage_v();
		else if (gauge == GAUGE_SOLAR) return device_state_get_solar_min_voltage_v();
	} else if (field_type == FIELD_GAUGE_BASELINE) {
		if (gauge == GAUGE_STARTER) return device_state_get_starter_baseline_voltage_v();
		else if (gauge == GAUGE_HOUSE) return device_state_get_house_baseline_voltage_v();
		else if (gauge == GAUGE_SOLAR) return 0.0f; // Solar has no baseline
	} else if (field_type == FIELD_GAUGE_HIGH) {
		if (gauge == GAUGE_STARTER) return device_state_get_starter_max_voltage_v();
		else if (gauge == GAUGE_HOUSE) return device_state_get_house_max_voltage_v();
		else if (gauge == GAUGE_SOLAR) return device_state_get_solar_max_voltage_v();
	}

	return 0.0f; // Default fallback
}

// Set device state value for a specific gauge and field type
static void set_device_state_value(int gauge, int field_type, float value)
{
	// Save alert thresholds
	if (field_type == FIELD_ALERT_LOW) {
		if (gauge == GAUGE_STARTER) device_state_set_starter_alert_low_voltage_v((int)value);
		else if (gauge == GAUGE_HOUSE) device_state_set_house_alert_low_voltage_v((int)value);
		else if (gauge == GAUGE_SOLAR) device_state_set_solar_alert_low_voltage_v((int)value);
	} else if (field_type == FIELD_ALERT_HIGH) {
		if (gauge == GAUGE_STARTER) device_state_set_starter_alert_high_voltage_v((int)value);
		else if (gauge == GAUGE_HOUSE) device_state_set_house_alert_high_voltage_v((int)value);
		else if (gauge == GAUGE_SOLAR) device_state_set_solar_alert_high_voltage_v((int)value);
	}
	// Save gauge ranges
	else if (field_type == FIELD_GAUGE_LOW) {
		if (gauge == GAUGE_STARTER) device_state_set_starter_min_voltage_v(value);
		else if (gauge == GAUGE_HOUSE) device_state_set_house_min_voltage_v(value);
		else if (gauge == GAUGE_SOLAR) device_state_set_solar_min_voltage_v(value);
	} else if (field_type == FIELD_GAUGE_BASELINE) {
		if (gauge == GAUGE_STARTER) device_state_set_starter_baseline_voltage_v(value);
		else if (gauge == GAUGE_HOUSE) device_state_set_house_baseline_voltage_v(value);
		// Solar baseline is not saved (always 0.0)
	} else if (field_type == FIELD_GAUGE_HIGH) {
		if (gauge == GAUGE_STARTER) device_state_set_starter_max_voltage_v(value);
		else if (gauge == GAUGE_HOUSE) device_state_set_house_max_voltage_v(value);
		else if (gauge == GAUGE_SOLAR) device_state_set_solar_max_voltage_v(value);
	}
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

	container_info_t container_cache[GAUGE_COUNT * 3]; // 3 containers per gauge (section, alerts, gauge)
	int cache_index = 0;

	// Populate cache with all containers
	for (int gauge = 0; gauge < GAUGE_COUNT; gauge++) {

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
		modal->current_field_id < GAUGE_COUNT * FIELD_COUNT_PER_GAUGE;

	printf("[I] alerts_modal: does_modal_have_active_field: %d\n", does_modal_have_active_field);

	// Step 1: Clear all highlighting on Fields and their containers
	for (int field_id = 0; field_id < GAUGE_COUNT * FIELD_COUNT_PER_GAUGE; field_id++) {

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
		} else if( data->is_out_of_range ){ // Out of range

			data->border_color = IS_OUT_OF_RANGE_BORDER_COLOR;
			data->border_width = 2;
		} else if( data->has_changed ){ // Green border for changed values

			data->border_color = HAS_CHANGED_BORDER_COLOR;
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
			for (int i = 0; i < GAUGE_COUNT * 3; i++) {

				if (container_cache[i].gauge_index == gauge_index && container_cache[i].group_type == -1) {

					container_cache[i].has_active_field = true;
				}
			}

			// Mark appropriate group as having active field
			for (int i = 0; i < GAUGE_COUNT * 3; i++) {

				if (container_cache[i].gauge_index == gauge_index && container_cache[i].group_type == data->group_type) {

					container_cache[i].has_active_field = true;
				}
			}
		}

	}

	// Step 3: Style the cached containers based on whether they contain active fields
	for (int i = 0; i < GAUGE_COUNT * 3; i++) {

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
	} else if (data->has_changed) {
		// Green border for changed values
		data->border_color = HAS_CHANGED_BORDER_COLOR;
		data->border_width = 2;
	} else {
		// Green border for editing state
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
	if (!modal || field_id < 0 || field_id >= GAUGE_COUNT * FIELD_COUNT_PER_GAUGE) return;

	field_ui_t* ui = &modal->field_ui[field_id];
	if (!ui->button || !ui->label) return;

	// Set warning color border and text (warm yellow)
	lv_obj_set_style_border_color(ui->button, DEFAULT_WARNING_BORDER_COLOR, 0);
	lv_obj_set_style_border_width(ui->button, 3, 0);
	lv_obj_set_style_text_color(ui->label, DEFAULT_WARNING_TEXT_COLOR, 0);

	printf("[I] alerts_modal: Highlighted field %d for warning (border and text)\n", field_id);
}

static void unhighlight_field_for_warning(alerts_modal_t* modal, int field_id)
{
	if (!modal || field_id < 0 || field_id >= GAUGE_COUNT * FIELD_COUNT_PER_GAUGE) return;

	// Restore normal field border
	update_field_border(modal, field_id);

	printf("[I] alerts_modal: Unhighlighted field %d from warning\n", field_id);
}

// Find field by button
static int find_field_by_button(alerts_modal_t* modal, lv_obj_t* button)
{
	if (!modal || !button) return -1;

	for (int i = 0; i < GAUGE_COUNT * FIELD_COUNT_PER_GAUGE; i++) {
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

	// Clamp the value to valid range before saving
	if (data->current_value < data->min_value) {
		data->current_value = data->min_value;
	} else if (data->current_value > data->max_value) {
		data->current_value = data->max_value;
	}

	data->is_being_edited = false;
	data->is_out_of_range = false;
	data->has_changed = !field_value_equals_original(data);

	// Save this field's value to device state
	set_device_state_value(data->gauge_index, data->field_index, data->current_value);
	device_state_save();

	printf("[I] alerts_modal: Saved field[%d,%d] value: %.1f\n",
		data->gauge_index, data->field_index, data->current_value);

	// Hide any warning for this field
	hide_out_of_range_warning(modal->current_field_id);

	update_field_border(modal, modal->current_field_id);

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
	alerts_modal_refresh_gauges_and_alerts();

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
	for (int field_id = 0; field_id < GAUGE_COUNT * FIELD_COUNT_PER_GAUGE; field_id++) {
		field_data_t* data = &modal->field_data[field_id];

		// Restore original value
		data->current_value = data->original_value;
		data->has_changed = false;
		data->is_out_of_range = false;

		// Update display and border
		update_field_display(modal, field_id);
		update_field_border(modal, field_id);
	}

	// Update all field borders to reflect the reverted state
	update_all_field_borders(modal);

	// Save the reverted values to device state
	for (int field_id = 0; field_id < GAUGE_COUNT * FIELD_COUNT_PER_GAUGE; field_id++) {
		field_data_t* data = &modal->field_data[field_id];
		set_device_state_value(data->gauge_index, data->field_index, data->current_value);
	}
	device_state_save();

	printf("[I] alerts_modal: Cancel button pressed - reverted all changes\n");

	// Refresh gauges and alerts after reverting changes
	alerts_modal_refresh_gauges_and_alerts();

	// Close the modal
	if (modal->on_close) {
		modal->on_close();
	}
}

// Numberpad callbacks
static void numberpad_value_changed(const char* value, void* user_data)
{
	alerts_modal_t* modal = (alerts_modal_t*)user_data;
	if (!modal || modal->current_field_id < 0) return;

	field_data_t* data = &modal->field_data[modal->current_field_id];
	float new_value = atof(value);

	// Always store the actual input value for display
	data->current_value = new_value;

	// Check if value is out of range
	bool is_out_of_range = (new_value < data->min_value || new_value > data->max_value);

	if (is_out_of_range) {

		// Show warning for out-of-range value
		show_out_of_range_warning(modal, modal->current_field_id, new_value);
	} else {

		// Hide any existing warning
		hide_out_of_range_warning(modal->current_field_id);
	}

	data->is_out_of_range = is_out_of_range;
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
		modal->current_field_id < GAUGE_COUNT * FIELD_COUNT_PER_GAUGE
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
		hide_out_of_range_warning(modal->current_field_id);

	}

	close_current_field(modal);

}

// Warning system implementation
static void show_out_of_range_warning(alerts_modal_t* modal, int field_id, float out_of_range_value)
{
	// Bail early if we don't have a valid modal or field ID
	if (!modal || field_id < 0 || field_id >= GAUGE_COUNT * FIELD_COUNT_PER_GAUGE) return;

	field_ui_t* ui = &modal->field_ui[field_id];

	// Bail early if we don't have a valid UI button
	if (!ui->button) return;

	// Check if warning already exists for this field
	if (g_warning_data[field_id].text_label != NULL) {
		printf("[I] alerts_modal: Warning already exists for field %d, skipping\n", field_id);
		return;
	}

	// Hide any existing warning for this field
	hide_out_of_range_warning(field_id);

	// Get field data and clamp the value immediately
	field_data_t* data = &modal->field_data[field_id];
	float clamped_value;


	if (out_of_range_value > data->max_value) {

		clamped_value = data->max_value;
	} else if (out_of_range_value < data->min_value) {

		clamped_value = data->min_value;
	} else {

		clamped_value = out_of_range_value; // Shouldn't happen
	}

	// Store the original out-of-range value for display purposes
	g_warning_data[field_id].original_value = out_of_range_value;
	g_warning_data[field_id].modal = modal;

	// Clamp the field value immediately
	data->current_value = clamped_value;

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
	lv_obj_set_style_radius(g_warning_data[field_id].text_label, 4, 0);
	// Hide the label initially until it has proper text
	lv_obj_add_flag(g_warning_data[field_id].text_label, LV_OBJ_FLAG_HIDDEN);

	// Create warning value label and container (for numeric values in max/min warnings)
	g_warning_data[field_id].value_label = NULL; // Will be created only for max/min warnings
	g_warning_data[field_id].container = NULL; // Will be created only for max/min warnings

	// Determine if value is above max or below min
	bool is_above_max = out_of_range_value > data->max_value;
	bool is_below_min = out_of_range_value < data->min_value;

	// Get field type to determine if it's a baseline warning
	bool is_baseline_warning = false;
	int highlighted_field_id = -1;
	if (field_id >= 0 && field_id < GAUGE_COUNT * FIELD_COUNT_PER_GAUGE) {
		int gauge_index = field_id / FIELD_COUNT_PER_GAUGE;
		int field_index = field_id % FIELD_COUNT_PER_GAUGE;
		is_baseline_warning = (field_index == FIELD_GAUGE_BASELINE);

		// For baseline warnings, determine which field to highlight
		if (is_baseline_warning) {
			if (is_above_max) {
				// Baseline is above max, highlight the max field
				highlighted_field_id = gauge_index * FIELD_COUNT_PER_GAUGE + FIELD_GAUGE_HIGH;
			} else if (is_below_min) {
				// Baseline is below min, highlight the min field
				highlighted_field_id = gauge_index * FIELD_COUNT_PER_GAUGE + FIELD_GAUGE_LOW;
			}
		}
	}

	// Set warning text based on warning type
	if (is_baseline_warning) {
		// Baseline warnings: show "OVER" or "UNDER" only
		if (is_above_max) {

			lv_label_set_text(g_warning_data[field_id].text_label, "OVER");
		} else if (is_below_min) {

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
			snprintf(value_text, sizeof(value_text), "%.1f", data->max_value);
			lv_label_set_text(g_warning_data[field_id].value_label, value_text);
		} else if (is_below_min) {

			// Create container that encompasses both text and value
			g_warning_data[field_id].container = lv_obj_create(modal->background);
			lv_obj_set_size(g_warning_data[field_id].container, 63, 60); // Larger to fit both text and value, 3px wider for negative values
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
			lv_obj_align(g_warning_data[field_id].text_label, LV_ALIGN_TOP_MID, 0, 10);

			// Create value label inside container
			g_warning_data[field_id].value_label = lv_label_create(g_warning_data[field_id].container);
			lv_obj_set_style_text_color(g_warning_data[field_id].value_label, PALETTE_YELLOW, 0);
			lv_obj_set_style_text_font(g_warning_data[field_id].value_label, &lv_font_noplato_24, 0);
			lv_obj_set_style_text_align(g_warning_data[field_id].value_label, LV_TEXT_ALIGN_CENTER, 0);
			lv_obj_align(g_warning_data[field_id].value_label, LV_ALIGN_BOTTOM_MID, 0, -10);

			char value_text[16];
			snprintf(value_text, sizeof(value_text), "%.1f", data->min_value);
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

	// Reset negative state in numberpad since value triggered a clamp
	// This ensures subsequent inputs start with positive values
	if (modal->numberpad && modal->numberpad->is_visible) {
		// Reset negative state silently to avoid triggering value changed callback
		modal->numberpad->is_negative = false;

		// Update numberpad to show the clamped value so subsequent inputs start fresh
		char clamped_value_str[16];
		snprintf(clamped_value_str, sizeof(clamped_value_str), "%.1f", clamped_value);
		numberpad_set_value_for_fresh_input(modal->numberpad, clamped_value_str);
	}

	// Update field display to show the out-of-range value
	update_field_display(modal, field_id);

	printf("[I] alerts_modal: Showing warning for out-of-range value: %.1f (timer created)\n", out_of_range_value);
}

static void hide_out_of_range_warning(int field_id)
{
	if (field_id < 0 || field_id >= GAUGE_COUNT * FIELD_COUNT_PER_GAUGE) return;

	if (g_warning_data[field_id].timer) {
		lv_timer_del(g_warning_data[field_id].timer);
		g_warning_data[field_id].timer = NULL;
	}

	if (g_warning_data[field_id].container) {
		lv_obj_del_async(g_warning_data[field_id].container);
		g_warning_data[field_id].container = NULL;
		g_warning_data[field_id].text_label = NULL; // Text label is child of container for max/min
		g_warning_data[field_id].value_label = NULL; // Value label is child of container
	} else if (g_warning_data[field_id].text_label) {
		// For baseline warnings, text label is standalone
		lv_obj_del_async(g_warning_data[field_id].text_label);
		g_warning_data[field_id].text_label = NULL;
	}

	// Update field display with the clamped value and clear out-of-range state
	if (g_warning_data[field_id].modal) {
		// Unhighlight field if it was a baseline warning
		if (g_warning_data[field_id].is_baseline_warning &&
			g_warning_data[field_id].highlighted_field_id >= 0) {
			unhighlight_field_for_warning(g_warning_data[field_id].modal, g_warning_data[field_id].highlighted_field_id);
		}

		// Clear the out-of-range state since the value is now clamped
		field_data_t* data = &g_warning_data[field_id].modal->field_data[field_id];
		data->is_out_of_range = false;

		// The current_value should already be clamped from when the warning was created
		// But let's ensure it's properly clamped
		if (data->current_value > data->max_value) {
			data->current_value = data->max_value;
		} else if (data->current_value < data->min_value) {
			data->current_value = data->min_value;
		}

		update_field_display(g_warning_data[field_id].modal, field_id);
		update_field_border(g_warning_data[field_id].modal, field_id);
		update_all_field_borders(g_warning_data[field_id].modal);
	}

	// Clear the warning data
	g_warning_data[field_id].field_id = -1;
	g_warning_data[field_id].modal = NULL;
	g_warning_data[field_id].highlighted_field_id = -1;
	g_warning_data[field_id].is_baseline_warning = false;
}

static void warning_timer_callback(lv_timer_t* timer)
{
	warning_data_t* data = (warning_data_t*)lv_timer_get_user_data(timer);
	if (!data) return;

	printf("[I] alerts_modal: Warning timer callback called for field %d\n", data->field_id);

	// The value is already clamped, just hide the warning
	hide_out_of_range_warning(data->field_id);

	printf("[I] alerts_modal: Warning timer expired, hiding warning\n");
}

// Create gauge section (maintains original visual design)
static void create_gauge_section(alerts_modal_t* modal, gauge_type_t gauge, lv_obj_t* parent, int y_offset)
{
	if (!modal || gauge >= GAUGE_COUNT) return;

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
	lv_label_set_text(modal->gauge_titles[gauge], gauge_names[gauge]);
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
alerts_modal_t* alerts_modal_create(void (*on_close_callback)(void))
{
	printf("[I] alerts_modal: Creating properly refactored alerts modal\n");

	alerts_modal_t* modal = malloc(sizeof(alerts_modal_t));
	if (!modal) {
		printf("[E] alerts_modal: Failed to allocate memory for alerts modal\n");
		return NULL;
	}

	memset(modal, 0, sizeof(alerts_modal_t));

	// Initialize warning data array to ensure all text_label fields are NULL
	memset(g_warning_data, 0, sizeof(g_warning_data));
	modal->on_close = on_close_callback;
	modal->current_field_id = -1;

	// Create modal background - truly full screen, no padding
	modal->background = lv_obj_create(lv_screen_active());
	lv_obj_set_size(modal->background, LV_PCT(100), LV_PCT(100));
	lv_obj_set_pos(modal->background, 0, 0);
	lv_obj_set_style_bg_color(modal->background, PALETTE_BLACK, 0);
	lv_obj_set_style_bg_opa(modal->background, LV_OPA_80, 0);
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
	for (int i = 0; i < GAUGE_COUNT; i++) {
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
	for (int gauge = 0; gauge < GAUGE_COUNT; gauge++) {

		for (int field_type = 0; field_type < FIELD_COUNT_PER_GAUGE; field_type++) {

			int field_id = gauge * FIELD_COUNT_PER_GAUGE + field_type;

			initialize_field_data(&modal->field_data[field_id], gauge, field_type);
		}
	}

	// Create field containers and populate fields
	for (int field_id = 0; field_id < GAUGE_COUNT * FIELD_COUNT_PER_GAUGE; field_id++) {

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

		lv_label_set_text(title_label, field_names[data->field_index]);
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
		float loaded_value = get_device_state_value(data->gauge_index, data->field_index);
		data->current_value = loaded_value;
		data->original_value = loaded_value;

		// Update display and border
		update_field_display(modal, field_id);
		update_field_border(modal, field_id);
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

	if (modal->numberpad) {
		numberpad_hide(modal->numberpad);
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

void alerts_modal_refresh_gauges_and_alerts(void)
{
	printf("[I] alerts_modal: Refreshing gauges and alerts after modal changes\n");

	// Update power grid view gauge configuration
	extern void power_monitor_power_grid_view_update_configuration(void);
	power_monitor_power_grid_view_update_configuration();

	// Update detail screen gauge ranges
	extern void power_monitor_update_detail_gauge_ranges(void);
	power_monitor_update_detail_gauge_ranges();

	// Update all power monitor data and gauges (this includes detail screen gauges)
	extern void power_monitor_update_data_only(void);
	power_monitor_update_data_only();

	// Update alert flashing
	extern void power_monitor_power_grid_view_apply_alert_flashing(const power_monitor_data_t* data, int starter_lo, int starter_hi, int house_lo, int house_hi, int solar_lo, int solar_hi, bool blink_on);
	extern power_monitor_data_t* power_monitor_get_data(void);

	power_monitor_data_t* data = power_monitor_get_data();
	if (data) {
		// Get current alert thresholds from device state
		int starter_lo = device_state_get_starter_alert_low_voltage_v();
		int starter_hi = device_state_get_starter_alert_high_voltage_v();
		int house_lo = device_state_get_house_alert_low_voltage_v();
		int house_hi = device_state_get_house_alert_high_voltage_v();
		int solar_lo = device_state_get_solar_alert_low_voltage_v();
		int solar_hi = device_state_get_solar_alert_high_voltage_v();

		// Apply alert flashing with current thresholds
		power_monitor_power_grid_view_apply_alert_flashing(data, starter_lo, starter_hi, house_lo, house_hi, solar_lo, solar_hi, false);
	}

	printf("[I] alerts_modal: Gauge and alert refresh complete\n");
}
