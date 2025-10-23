#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lvgl.h>
#include <math.h>

#include "timeline_modal.h"

#include "../../../../state/device_state.h"

// UI Style
#include "../../palette.h"
#include "../../../../fonts/lv_font_noplato_10.h"
#include "../../../../fonts/lv_font_noplato_14.h"
#include "../../../../fonts/lv_font_noplato_18.h"
#include "../../../../fonts/lv_font_noplato_24.h"
#include "../../utils/animation/animation.h"


// #### Default State Colors ####

// Gauge Section
#define DEFAULT_GAUGE_SECTION_BORDER_COLOR PALETTE_GRAY
#define DEFAULT_GAUGE_SECTION_BORDER_WIDTH 1

// Gauge Title
#define DEFAULT_GAUGE_TITLE_BACKGROUND_COLOR PALETTE_BLACK
#define DEFAULT_GAUGE_TITLE_TEXT_COLOR PALETTE_WHITE

// Group Containers
#define DEFAULT_GROUP_BORDER_COLOR PALETTE_GRAY
#define DEFAULT_GROUP_BORDER_WIDTH 1
#define DIM_GROUP_BORDER_COLOR PALETTE_DARK_GRAY
#define DIM_GROUP_BORDER_WIDTH 1

// Group Titles
#define DEFAULT_CURRENT_VIEW_TITLE_BACKGROUND_COLOR PALETTE_YELLOW
#define DEFAULT_CURRENT_VIEW_TITLE_TEXT_COLOR PALETTE_BLACK
#define DEFAULT_DETAIL_VIEW_TITLE_BACKGROUND_COLOR lv_color_hex(0x8F4700)  // Brown
#define DEFAULT_DETAIL_VIEW_TITLE_TEXT_COLOR PALETTE_WHITE

// Value Labels
#define DEFAULT_VALUE_TEXT_COLOR PALETTE_WHITE

// #### Selected State Colors ####

// Selected Gauge Section (inactive gauge container)
#define SELECTED_GAUGE_SECTION_BORDER_COLOR PALETTE_WHITE
#define SELECTED_GAUGE_SECTION_BORDER_WIDTH 1

// Selected Gauge Title
#define SELECTED_GAUGE_TITLE_BACKGROUND_COLOR PALETTE_BLUE
#define SELECTED_GAUGE_TITLE_TEXT_COLOR PALETTE_WHITE

// Active View Container (only the active view gets cyan)
#define ACTIVE_VIEW_CONTAINER_BORDER_COLOR PALETTE_CYAN
#define ACTIVE_VIEW_CONTAINER_BORDER_WIDTH 3

// Active View Title
#define ACTIVE_VIEW_TITLE_BACKGROUND_COLOR PALETTE_CYAN
#define ACTIVE_VIEW_TITLE_TEXT_COLOR PALETTE_BLACK

// Inactive View Container (within selected gauge)
#define INACTIVE_VIEW_CONTAINER_BORDER_COLOR PALETTE_DARK_GRAY
#define INACTIVE_VIEW_CONTAINER_BORDER_WIDTH 1

// Inactive View Title
#define INACTIVE_VIEW_TITLE_BACKGROUND_COLOR PALETTE_DARK_GRAY
#define INACTIVE_VIEW_TITLE_TEXT_COLOR PALETTE_BLACK

// #### Changed State Colors ####

// Changed Gauge Section
#define CHANGED_GAUGE_SECTION_BORDER_COLOR PALETTE_GREEN
#define CHANGED_GAUGE_SECTION_BORDER_WIDTH 2

// Changed Gauge Title
#define CHANGED_GAUGE_TITLE_BACKGROUND_COLOR PALETTE_GREEN
#define CHANGED_GAUGE_TITLE_TEXT_COLOR PALETTE_BLACK

// #### Being Edited State Colors (Highest Priority) ####

// Being Edited Group Container
#define BEING_EDITED_GROUP_BORDER_COLOR PALETTE_CYAN
#define BEING_EDITED_GROUP_BORDER_WIDTH 3

// Being Edited Group Title
#define BEING_EDITED_GROUP_TITLE_BACKGROUND_COLOR PALETTE_CYAN
#define BEING_EDITED_GROUP_TITLE_TEXT_COLOR PALETTE_BLACK

// #### Dimmed State Colors ####

// Dimmed Gauge Section
#define DIM_GAUGE_SECTION_BORDER_COLOR PALETTE_DARK_GRAY
#define DIM_GAUGE_SECTION_BORDER_WIDTH 1

// Dimmed Gauge Title
#define DIM_GAUGE_TITLE_BACKGROUND_COLOR PALETTE_DARK_GRAY
#define DIM_GAUGE_TITLE_TEXT_COLOR PALETTE_BLACK

// Dimmed Group Titles
#define DIM_GROUP_TITLE_BACKGROUND_COLOR PALETTE_DARK_GRAY
#define DIM_GROUP_TITLE_TEXT_COLOR PALETTE_BLACK

// Dimmed Value Labels
#define DIM_VALUE_TEXT_COLOR PALETTE_DARK_GRAY

// Forward declarations
static void timeline_click_handler(lv_event_t* e);
static void close_button_clicked(lv_event_t* e);
static void cancel_button_clicked(lv_event_t* e);
// Helper function to create a view container with H, M, S labels
static void create_view_container(timeline_modal_t* modal, int gauge, bool is_current_view);

static void create_gauge_section(timeline_modal_t* modal, int gauge, lv_obj_t* parent);
static void update_gauge_value(timeline_modal_t* modal, int gauge, float value, bool is_current_view);
static void update_timeline_display(timeline_modal_t* modal, int gauge, bool is_current_view);
static void gauge_animation_callback(int index, float value, void* user_data);
static void update_selected_gauge_highlight(timeline_modal_t* modal);
static void update_gauge_ui(timeline_modal_t* modal);
static int find_gauge_by_section(timeline_modal_t* modal, lv_obj_t* section);
static void time_input_value_changed(int hours, int minutes, int seconds, void* user_data);
static void time_input_enter(int hours, int minutes, int seconds, void* user_data);
static void time_input_cancel(void* user_data);
static void load_current_gauge_timeline_settings(timeline_modal_t* modal);
static void animate_numbers(timeline_modal_t* modal, int gauge, bool is_current_view, float target_duration);

// Create gauge section - simplified structure like alerts modal
static void create_gauge_section(timeline_modal_t* modal, int gauge, lv_obj_t* parent)
{
	if (!modal || gauge >= modal->config.gauge_count) return;

	// Create gauge container parent - holds both gauge section and title
	lv_obj_t* gauge_container = lv_obj_create(parent);
	lv_obj_set_size(gauge_container, LV_PCT(100), 116);
	lv_obj_set_style_bg_color(gauge_container, PALETTE_BLACK, 0);
	lv_obj_set_style_bg_opa(gauge_container, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(gauge_container, 0, 0);
	lv_obj_set_style_pad_all(gauge_container, 0, 0);
	lv_obj_clear_flag(gauge_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(gauge_container, LV_OBJ_FLAG_EVENT_BUBBLE);
	lv_obj_add_flag(gauge_container, LV_OBJ_FLAG_CLICKABLE);

	// Store gauge container reference for direct access
	modal->gauge_ui[gauge].gauge_container = gauge_container;

	// Gauge section container
	modal->gauge_sections[gauge] = lv_obj_create(gauge_container);
	lv_obj_set_size(modal->gauge_sections[gauge], LV_PCT(100), 100);
	lv_obj_align(modal->gauge_sections[gauge], LV_ALIGN_BOTTOM_MID, 0, 0);
	lv_obj_set_style_bg_color(modal->gauge_sections[gauge], PALETTE_BLACK, 0);
	lv_obj_set_style_bg_opa(modal->gauge_sections[gauge], LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(modal->gauge_sections[gauge], DEFAULT_GAUGE_SECTION_BORDER_WIDTH, 0);
	lv_obj_set_style_border_color(modal->gauge_sections[gauge], DEFAULT_GAUGE_SECTION_BORDER_COLOR, 0);
	lv_obj_set_style_pad_all(modal->gauge_sections[gauge], 0, 0);
	lv_obj_clear_flag(modal->gauge_sections[gauge], LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(modal->gauge_sections[gauge], LV_OBJ_FLAG_EVENT_BUBBLE);
	lv_obj_add_flag(modal->gauge_sections[gauge], LV_OBJ_FLAG_CLICKABLE);

	// Gauge Title - positioned inline with border like alerts modal
	modal->gauge_titles[gauge] = lv_label_create(gauge_container);
	char title_text[64];
	snprintf(title_text, sizeof(title_text), "%s", modal->config.gauges[gauge].name);
	lv_label_set_text(modal->gauge_titles[gauge], title_text);
	lv_obj_set_style_text_color(modal->gauge_titles[gauge], DEFAULT_GAUGE_TITLE_TEXT_COLOR, 0);
	lv_obj_set_style_text_font(modal->gauge_titles[gauge], &lv_font_montserrat_16, 0);
	lv_obj_set_style_bg_color(modal->gauge_titles[gauge], DEFAULT_GAUGE_TITLE_BACKGROUND_COLOR, 0);
	lv_obj_set_style_bg_opa(modal->gauge_titles[gauge], LV_OPA_COVER, 0);
	lv_obj_set_style_pad_left(modal->gauge_titles[gauge], 8, 0);
	lv_obj_set_style_pad_right(modal->gauge_titles[gauge], 8, 0);
	lv_obj_set_style_pad_top(modal->gauge_titles[gauge], 2, 0);
	lv_obj_set_style_pad_bottom(modal->gauge_titles[gauge], 2, 0);
	lv_obj_set_style_radius(modal->gauge_titles[gauge], 5, 0);
	lv_obj_align_to(modal->gauge_titles[gauge], modal->gauge_sections[gauge], LV_ALIGN_OUT_TOP_RIGHT, -10, 10);

	// Add click event handlers to gauge containers
	lv_obj_add_event_cb(gauge_container, timeline_click_handler, LV_EVENT_CLICKED, modal);
	lv_obj_add_event_cb(modal->gauge_sections[gauge], timeline_click_handler, LV_EVENT_CLICKED, modal);

	// Initialize individual durations to realtime (0 seconds)
	modal->gauge_ui[gauge].current_view_duration = 0.0f;  // Default to realtime
	modal->gauge_ui[gauge].detail_view_duration = 0.0f;   // Default to realtime

	// Create view containers using helper function
	create_view_container(modal, gauge, true);   // Current view
	create_view_container(modal, gauge, false);  // Detail view

	// Update the display to show the initial values (should be "REAL TIME" for 0 seconds)
	update_timeline_display(modal, gauge, true);  // Current view
	update_timeline_display(modal, gauge, false); // Detail view
}


// Helper function to create a view container with H, M, S labels
static void create_view_container(timeline_modal_t* modal, int gauge, bool is_current_view)
{
	lv_obj_t** group;
	lv_obj_t** title;
	lv_obj_t** hours_label;
	lv_obj_t** hours_letter;
	lv_obj_t** minutes_label;
	lv_obj_t** minutes_letter;
	lv_obj_t** seconds_label;
	lv_obj_t** seconds_letter;

	// Set up pointers based on view type
	if (is_current_view) {
		group = &modal->gauge_ui[gauge].current_view_group;
		title = &modal->gauge_ui[gauge].current_view_title;
		hours_label = &modal->gauge_ui[gauge].current_view_hours_label;
		hours_letter = &modal->gauge_ui[gauge].current_view_hours_letter;
		minutes_label = &modal->gauge_ui[gauge].current_view_minutes_label;
		minutes_letter = &modal->gauge_ui[gauge].current_view_minutes_letter;
		seconds_label = &modal->gauge_ui[gauge].current_view_seconds_label;
		seconds_letter = &modal->gauge_ui[gauge].current_view_seconds_letter;
	} else {
		group = &modal->gauge_ui[gauge].detail_view_group;
		title = &modal->gauge_ui[gauge].detail_view_title;
		hours_label = &modal->gauge_ui[gauge].detail_view_hours_label;
		hours_letter = &modal->gauge_ui[gauge].detail_view_hours_letter;
		minutes_label = &modal->gauge_ui[gauge].detail_view_minutes_label;
		minutes_letter = &modal->gauge_ui[gauge].detail_view_minutes_letter;
		seconds_label = &modal->gauge_ui[gauge].detail_view_seconds_label;
		seconds_letter = &modal->gauge_ui[gauge].detail_view_seconds_letter;
	}

	// Create group container
	*group = lv_obj_create(modal->gauge_sections[gauge]);
	lv_obj_set_size(*group, is_current_view ? LV_PCT(37) : LV_PCT(56), 60);

	if (is_current_view) {
		lv_obj_set_pos(*group, 10, 20);
	} else {
		lv_obj_align_to(*group, modal->gauge_ui[gauge].current_view_group, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
	}

	lv_obj_set_layout(*group, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(*group, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(*group, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_bg_color(*group, PALETTE_BLACK, 0);
	lv_obj_set_style_bg_opa(*group, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(*group, DEFAULT_GROUP_BORDER_WIDTH, 0);
	lv_obj_set_style_border_color(*group, DEFAULT_GROUP_BORDER_COLOR, 0);
	lv_obj_set_style_radius(*group, 5, 0);
	lv_obj_set_style_pad_all(*group, 0, 0);
	lv_obj_clear_flag(*group, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(*group, LV_OBJ_FLAG_EVENT_BUBBLE);
	lv_obj_add_flag(*group, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(*group, timeline_click_handler, LV_EVENT_CLICKED, modal);

	// Create group title
	*title = lv_label_create(modal->gauge_sections[gauge]);
	lv_label_set_text(*title, is_current_view ? "CURRENT VIEW" : "DETAIL VIEW");
	lv_obj_set_style_text_color(*title, is_current_view ? DEFAULT_CURRENT_VIEW_TITLE_TEXT_COLOR : DEFAULT_DETAIL_VIEW_TITLE_TEXT_COLOR, 0);
	lv_obj_set_style_text_font(*title, &lv_font_montserrat_12, 0);
	lv_obj_set_style_bg_color(*title, is_current_view ? DEFAULT_CURRENT_VIEW_TITLE_BACKGROUND_COLOR : DEFAULT_DETAIL_VIEW_TITLE_BACKGROUND_COLOR, 0);
	lv_obj_set_style_bg_opa(*title, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_left(*title, 8, 0);
	lv_obj_set_style_pad_right(*title, 8, 0);
	lv_obj_set_style_pad_top(*title, 2, 0);
	lv_obj_set_style_pad_bottom(*title, 2, 0);
	lv_obj_set_style_radius(*title, 3, 0);
	lv_obj_align_to(*title, *group, LV_ALIGN_OUT_TOP_LEFT, 10, 10);
	lv_obj_add_flag(*title, LV_OBJ_FLAG_EVENT_BUBBLE);

	// Create H, M, S labels
	// Hours
	*hours_label = lv_label_create(*group);
	lv_label_set_text(*hours_label, "0");
	lv_obj_set_style_text_color(*hours_label, DEFAULT_VALUE_TEXT_COLOR, 0);
	lv_obj_set_style_text_font(*hours_label, &lv_font_noplato_24, 0);
	lv_obj_add_flag(*hours_label, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(*hours_label, LV_OBJ_FLAG_EVENT_BUBBLE);

	*hours_letter = lv_label_create(*group);
	lv_label_set_text(*hours_letter, "H");
	lv_obj_set_style_text_color(*hours_letter, DEFAULT_VALUE_TEXT_COLOR, 0);
	lv_obj_set_style_text_font(*hours_letter, &lv_font_montserrat_16, 0);
	lv_obj_set_style_translate_x(*hours_letter, -8, 0);
	lv_obj_set_style_translate_y(*hours_letter, -1, 0);
	lv_obj_add_flag(*hours_letter, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(*hours_letter, LV_OBJ_FLAG_EVENT_BUBBLE);

	// Minutes
	*minutes_label = lv_label_create(*group);
	lv_label_set_text(*minutes_label, "0");
	lv_obj_set_style_text_color(*minutes_label, DEFAULT_VALUE_TEXT_COLOR, 0);
	lv_obj_set_style_text_font(*minutes_label, &lv_font_noplato_24, 0);
	lv_obj_add_flag(*minutes_label, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(*minutes_label, LV_OBJ_FLAG_EVENT_BUBBLE);

	*minutes_letter = lv_label_create(*group);
	lv_label_set_text(*minutes_letter, "M");
	lv_obj_set_style_text_color(*minutes_letter, DEFAULT_VALUE_TEXT_COLOR, 0);
	lv_obj_set_style_text_font(*minutes_letter, &lv_font_montserrat_16, 0);
	lv_obj_set_style_translate_x(*minutes_letter, -8, 0);
	lv_obj_set_style_translate_y(*minutes_letter, -1, 0);
	lv_obj_add_flag(*minutes_letter, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(*minutes_letter, LV_OBJ_FLAG_EVENT_BUBBLE);

	// Seconds
	*seconds_label = lv_label_create(*group);
	lv_label_set_text(*seconds_label, is_current_view ? "30" : "30");
	lv_obj_set_style_text_color(*seconds_label, DEFAULT_VALUE_TEXT_COLOR, 0);
	lv_obj_set_style_text_font(*seconds_label, &lv_font_noplato_24, 0);
	lv_obj_add_flag(*seconds_label, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(*seconds_label, LV_OBJ_FLAG_EVENT_BUBBLE);

	*seconds_letter = lv_label_create(*group);
	lv_label_set_text(*seconds_letter, "S");
	lv_obj_set_style_text_color(*seconds_letter, DEFAULT_VALUE_TEXT_COLOR, 0);
	lv_obj_set_style_text_font(*seconds_letter, &lv_font_montserrat_16, 0);
	lv_obj_set_style_translate_x(*seconds_letter, -8, 0);
	lv_obj_set_style_translate_y(*seconds_letter, -1, 0);
	lv_obj_add_flag(*seconds_letter, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_flag(*seconds_letter, LV_OBJ_FLAG_EVENT_BUBBLE);
}

// Create timeline option button for a specific gauge - flat data structure like alerts_modal

// Time input value changed callback
static void time_input_value_changed(int hours, int minutes, int seconds, void* user_data)
{
	timeline_modal_t* modal = (timeline_modal_t*)user_data;
	if (!modal || modal->selected_gauge < 0) return;

	// Calculate total time in seconds
	float total_seconds = (hours * 3600.0) + (minutes * 60.0) + seconds;

	// Use the tracked view type from the click handler
	bool is_current_view = modal->selected_is_current_view;

	// Update the individual duration for the selected view with smooth animation
	if (is_current_view) {
		modal->gauge_ui[modal->selected_gauge].current_view_duration = total_seconds;
		animate_numbers(modal, modal->selected_gauge, true, total_seconds);
	} else {
		modal->gauge_ui[modal->selected_gauge].detail_view_duration = total_seconds;
		animate_numbers(modal, modal->selected_gauge, false, total_seconds);
	}

	// Call the callback with the updated duration
	if (modal->config.on_timeline_changed) {
		modal->config.on_timeline_changed(modal->selected_gauge, (int)total_seconds, is_current_view);
	}

	// Update changed status based on whether value was reverted to original
	if (modal->selected_gauge >= 0 && modal->selected_gauge < modal->config.gauge_count) {
		if (modal->selected_is_current_view) {
			// Check if current view was reverted to original value
			if (fabs(total_seconds - modal->gauge_ui[modal->selected_gauge].original_current_view_duration) < 0.1f) {
				modal->gauge_ui[modal->selected_gauge].current_view_has_changed = false;
			} else {
				modal->gauge_ui[modal->selected_gauge].current_view_has_changed = true;
			}
		} else {
			// Check if detail view was reverted to original value
			if (fabs(total_seconds - modal->gauge_ui[modal->selected_gauge].original_detail_view_duration) < 0.1f) {
				modal->gauge_ui[modal->selected_gauge].detail_view_has_changed = false;
			} else {
				modal->gauge_ui[modal->selected_gauge].detail_view_has_changed = true;
			}
		}
		update_gauge_ui(modal);
	}
}

// Time input enter callback
static void time_input_enter(int hours, int minutes, int seconds, void* user_data)
{
	timeline_modal_t* modal = (timeline_modal_t*)user_data;
	if (!modal) return;

	// Hide time input
	time_input_hide(modal->time_input);

	// Deselect gauge
	modal->selected_gauge = -1;
	update_gauge_ui(modal);
}

// Time input cancel callback
static void time_input_cancel(void* user_data)
{
	timeline_modal_t* modal = (timeline_modal_t*)user_data;
	if (!modal) return;

	// Hide time input
	time_input_hide(modal->time_input);

	// Deselect gauge
	modal->selected_gauge = -1;
	update_gauge_ui(modal);
}

// Animation callback for gauge value changes
static void gauge_animation_callback(int index, float value, void* user_data)
{
	timeline_modal_t* modal = (timeline_modal_t*)user_data;
	if (!modal || index >= modal->config.gauge_count) return;

	// Update the gauge UI - no longer needed since we have individual durations

	// Update timeline displays for both views
	update_timeline_display(modal, index, true);  // Current view
	update_timeline_display(modal, index, false); // Detail view
}

// Animation data structure for individual H, M, S animation
typedef struct {
	timeline_modal_t* modal;
	int gauge;
	bool is_current_view;
	int component_type; // 0=hours, 1=minutes, 2=seconds
} component_animation_data_t;

// Animation callback for individual component lerping
static void component_animation_callback(void* var, int32_t value)
{
	component_animation_data_t* anim_data = (component_animation_data_t*)var;
	if (!anim_data || !anim_data->modal) return;

	timeline_modal_t* modal = anim_data->modal;
	int gauge = anim_data->gauge;
	bool is_current_view = anim_data->is_current_view;
	int component_type = anim_data->component_type;

	if (gauge >= modal->config.gauge_count) return;

	// Get the appropriate label based on component type
	lv_obj_t* label = NULL;
	switch (component_type) {
		case 0: // hours
			label = is_current_view ?
				modal->gauge_ui[gauge].current_view_hours_label :
				modal->gauge_ui[gauge].detail_view_hours_label;
			break;
		case 1: // minutes
			label = is_current_view ?
				modal->gauge_ui[gauge].current_view_minutes_label :
				modal->gauge_ui[gauge].detail_view_minutes_label;
			break;
		case 2: // seconds
			label = is_current_view ?
				modal->gauge_ui[gauge].current_view_seconds_label :
				modal->gauge_ui[gauge].detail_view_seconds_label;
			break;
	}

	if (!label) return;

	// Update the label with the animated value
	char text[8];
	snprintf(text, sizeof(text), "%d", (int)value);
	lv_label_set_text(label, text);
}

// Animation ready callback
static void component_animation_ready_callback(lv_anim_t* a)
{
	component_animation_data_t* anim_data = (component_animation_data_t*)a->var;
	if (!anim_data) return;

	// Animation complete - update the full display to ensure visibility is correct
	update_timeline_display(anim_data->modal, anim_data->gauge, anim_data->is_current_view);

	// Free the animation data
	free(anim_data);
}

// Animate individual H, M, S components smoothly
static void animate_component(timeline_modal_t* modal, int gauge, bool is_current_view, int component_type, int target_value)
{
	if (!modal || gauge >= modal->config.gauge_count) return;

	// Get current displayed value
	lv_obj_t* label = NULL;
	switch (component_type) {
		case 0: // hours
			label = is_current_view ?
				modal->gauge_ui[gauge].current_view_hours_label :
				modal->gauge_ui[gauge].detail_view_hours_label;
			break;
		case 1: // minutes
			label = is_current_view ?
				modal->gauge_ui[gauge].current_view_minutes_label :
				modal->gauge_ui[gauge].detail_view_minutes_label;
			break;
		case 2: // seconds
			label = is_current_view ?
				modal->gauge_ui[gauge].current_view_seconds_label :
				modal->gauge_ui[gauge].detail_view_seconds_label;
			break;
	}

	if (!label) return;

	// Get current displayed value
	const char* current_text = lv_label_get_text(label);
	int current_value = 0;
	if (current_text) {
		current_value = atoi(current_text);
	}

	// If values are the same, no animation needed
	if (current_value == target_value) {
		return;
	}

	// Allocate animation data
	component_animation_data_t* anim_data = malloc(sizeof(component_animation_data_t));
	if (!anim_data) return;

	anim_data->modal = modal;
	anim_data->gauge = gauge;
	anim_data->is_current_view = is_current_view;
	anim_data->component_type = component_type;

	// Create animation for smooth component transition
	lv_anim_t a;
	lv_anim_init(&a);
	lv_anim_set_var(&a, anim_data);
	lv_anim_set_values(&a, current_value, target_value);
	lv_anim_set_time(&a, 300); // 300ms animation
	lv_anim_set_exec_cb(&a, component_animation_callback);
	lv_anim_set_ready_cb(&a, component_animation_ready_callback);
	lv_anim_start(&a);
}

// Animate all H, M, S components smoothly
static void animate_numbers(timeline_modal_t* modal, int gauge, bool is_current_view, float target_duration)
{
	if (!modal || gauge >= modal->config.gauge_count) return;

	// Calculate target values
	int total_seconds = (int)target_duration;
	int target_hours = total_seconds / 3600;
	int target_minutes = (total_seconds % 3600) / 60;
	int target_seconds = total_seconds % 60;

	// Animate each component individually
	animate_component(modal, gauge, is_current_view, 0, target_hours);   // hours
	animate_component(modal, gauge, is_current_view, 1, target_minutes); // minutes
	animate_component(modal, gauge, is_current_view, 2, target_seconds); // seconds
}

// Update gauge display (helper function for animation)

// Update timeline display for a specific view
static void update_timeline_display(timeline_modal_t* modal, int gauge, bool is_current_view)
{
	if (!modal || gauge >= modal->config.gauge_count) return;

	float duration = is_current_view ?
		modal->gauge_ui[gauge].current_view_duration :
		modal->gauge_ui[gauge].detail_view_duration;

	// Get the individual H, M, S labels
	lv_obj_t* hours_label = is_current_view ?
		modal->gauge_ui[gauge].current_view_hours_label :
		modal->gauge_ui[gauge].detail_view_hours_label;
	lv_obj_t* hours_letter = is_current_view ?
		modal->gauge_ui[gauge].current_view_hours_letter :
		modal->gauge_ui[gauge].detail_view_hours_letter;

	lv_obj_t* minutes_label = is_current_view ?
		modal->gauge_ui[gauge].current_view_minutes_label :
		modal->gauge_ui[gauge].detail_view_minutes_label;
	lv_obj_t* minutes_letter = is_current_view ?
		modal->gauge_ui[gauge].current_view_minutes_letter :
		modal->gauge_ui[gauge].detail_view_minutes_letter;

	lv_obj_t* seconds_label = is_current_view ?
		modal->gauge_ui[gauge].current_view_seconds_label :
		modal->gauge_ui[gauge].detail_view_seconds_label;
	lv_obj_t* seconds_letter = is_current_view ?
		modal->gauge_ui[gauge].current_view_seconds_letter :
		modal->gauge_ui[gauge].detail_view_seconds_letter;

	// Calculate time components
	int total_seconds = (int)duration;
	int hours = total_seconds / 3600;
	int minutes = (total_seconds % 3600) / 60;
	int seconds = total_seconds % 60;

	// Special case for realtime (0 seconds) - show "REALTIME"
	if (total_seconds == 0) {
		// Hide hours and minutes
		lv_obj_add_flag(hours_label, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(hours_letter, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(minutes_label, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(minutes_letter,
			LV_OBJ_FLAG_HIDDEN);

		// Show "REALTIME" for realtime
		lv_label_set_text(seconds_label, "REALTIME");
		lv_obj_set_style_text_font(seconds_label, &lv_font_montserrat_20, 0);
		lv_obj_clear_flag(seconds_label, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(seconds_letter, LV_OBJ_FLAG_HIDDEN);
		return;
	}

	// Update hours display
	if (hours > 0) {
		char hours_text[8];
		snprintf(hours_text, sizeof(hours_text), "%d", hours);
		lv_label_set_text(hours_label, hours_text);
		lv_obj_clear_flag(hours_label, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(hours_letter, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(hours_label, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(hours_letter, LV_OBJ_FLAG_HIDDEN);
	}

	// Update minutes display
	if (minutes > 0 || hours > 0) {
		char minutes_text[8];
		snprintf(minutes_text, sizeof(minutes_text), "%d", minutes);
		lv_label_set_text(minutes_label, minutes_text);
		lv_obj_clear_flag(minutes_label, LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(minutes_letter, LV_OBJ_FLAG_HIDDEN);
	} else {
		lv_obj_add_flag(minutes_label, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(minutes_letter, LV_OBJ_FLAG_HIDDEN);
	}

	// Update seconds display (always show)
	char seconds_text[8];
	snprintf(seconds_text, sizeof(seconds_text), "%d", seconds);
	lv_label_set_text(seconds_label, seconds_text);
	// Restore the correct font for seconds (same as hours and minutes)
	lv_obj_set_style_text_font(seconds_label, &lv_font_noplato_24, 0);
	lv_obj_clear_flag(seconds_label, LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(seconds_letter, LV_OBJ_FLAG_HIDDEN);

	// Labels are positioned by flexbox - no manual positioning needed
}

// Update gauge value and display with smooth animation
static void update_gauge_value(timeline_modal_t* modal, int gauge, float value, bool is_current_view)
{
	if (!modal || gauge >= modal->config.gauge_count) return;

	// Use animation manager to animate to target value
	if (modal->animation_manager) {
		animation_manager_animate_to(modal->animation_manager, gauge, value);
	} else {
		// Fallback: set immediately if no animation manager
		// No longer needed since we have individual durations
		// Update timeline displays for both views
		update_timeline_display(modal, gauge, true);  // Current view
		update_timeline_display(modal, gauge, false); // Detail view
	}

	// Call the callback if set (with gauge index, target value, and view type)
	if (modal->config.on_timeline_changed) {
		modal->config.on_timeline_changed(gauge, (int)value, is_current_view);
	}
}


// Update gauge UI - set default colors and apply state-based highlighting
static void update_gauge_ui(timeline_modal_t* modal)
{
	if (!modal) {
		printf("[D] timeline_modal: update_gauge_ui called with NULL modal\n");
		return;
	}

	printf("[D] timeline_modal: update_gauge_ui called (selected_gauge=%d, gauge_count=%d)\n", modal->selected_gauge, modal->config.gauge_count);

	bool has_selected_gauge = modal->selected_gauge >= 0 && modal->selected_gauge < modal->config.gauge_count;

	// Update all gauge sections
	for( int i = 0; i < modal->config.gauge_count; i++ ){
		bool is_selected = (i == modal->selected_gauge);
		bool current_view_has_changed = modal->gauge_ui[i].current_view_has_changed;
		bool detail_view_has_changed = modal->gauge_ui[i].detail_view_has_changed;
		bool current_view_being_edited = modal->gauge_ui[i].current_view_being_edited;
		bool detail_view_being_edited = modal->gauge_ui[i].detail_view_being_edited;
		bool should_dim = has_selected_gauge && !is_selected;

		// Set gauge section border colors and widths
		if (is_selected) {
			lv_obj_set_style_border_color(modal->gauge_sections[i], SELECTED_GAUGE_SECTION_BORDER_COLOR, 0);
			lv_obj_set_style_border_width(modal->gauge_sections[i], SELECTED_GAUGE_SECTION_BORDER_WIDTH, 0);
		} else {
			lv_color_t border_color = should_dim ? DIM_GAUGE_SECTION_BORDER_COLOR : DEFAULT_GAUGE_SECTION_BORDER_COLOR;
			lv_obj_set_style_border_color(modal->gauge_sections[i], border_color, 0);
			lv_obj_set_style_border_width(modal->gauge_sections[i], should_dim ? DIM_GAUGE_SECTION_BORDER_WIDTH : DEFAULT_GAUGE_SECTION_BORDER_WIDTH, 0);
		}

		// Set gauge title colors
		if (modal->gauge_titles[i]) {
			lv_color_t title_bg_color = DEFAULT_GAUGE_TITLE_BACKGROUND_COLOR;
			lv_color_t title_text_color = DEFAULT_GAUGE_TITLE_TEXT_COLOR;

			if (is_selected) {
				title_bg_color = SELECTED_GAUGE_TITLE_BACKGROUND_COLOR;
				title_text_color = SELECTED_GAUGE_TITLE_TEXT_COLOR;
			} else if (should_dim) {
				title_bg_color = DIM_GAUGE_TITLE_BACKGROUND_COLOR;
				title_text_color = DIM_GAUGE_TITLE_TEXT_COLOR;
			}

			lv_obj_set_style_bg_color(modal->gauge_titles[i], title_bg_color, 0);
			lv_obj_set_style_text_color(modal->gauge_titles[i], title_text_color, 0);
		}

		// Set value text colors based on active state - individual H, M, S components
		if (i == modal->selected_gauge) {
			// This is the selected gauge - dim inactive view value, keep active view value normal
			if (modal->selected_is_current_view) {
				// Current view is active - normal current, dim detail
				// Current view H, M, S labels
				if (modal->gauge_ui[i].current_view_hours_label) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_hours_label, DEFAULT_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].current_view_hours_letter) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_hours_letter, DEFAULT_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].current_view_minutes_label) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_minutes_label, DEFAULT_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].current_view_minutes_letter) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_minutes_letter, DEFAULT_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].current_view_seconds_label) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_seconds_label, DEFAULT_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].current_view_seconds_letter) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_seconds_letter, DEFAULT_VALUE_TEXT_COLOR, 0);
				}
				// Detail view H, M, S labels - dimmed
				if (modal->gauge_ui[i].detail_view_hours_label) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_hours_label, DIM_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].detail_view_hours_letter) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_hours_letter, DIM_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].detail_view_minutes_label) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_minutes_label, DIM_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].detail_view_minutes_letter) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_minutes_letter, DIM_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].detail_view_seconds_label) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_seconds_label, DIM_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].detail_view_seconds_letter) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_seconds_letter, DIM_VALUE_TEXT_COLOR, 0);
				}
			} else {
				// Detail view is active - normal detail, dim current
				// Current view H, M, S labels - dimmed
				if (modal->gauge_ui[i].current_view_hours_label) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_hours_label, DIM_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].current_view_hours_letter) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_hours_letter, DIM_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].current_view_minutes_label) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_minutes_label, DIM_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].current_view_minutes_letter) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_minutes_letter, DIM_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].current_view_seconds_label) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_seconds_label, DIM_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].current_view_seconds_letter) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_seconds_letter, DIM_VALUE_TEXT_COLOR, 0);
				}
				// Detail view H, M, S labels - normal
				if (modal->gauge_ui[i].detail_view_hours_label) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_hours_label, DEFAULT_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].detail_view_hours_letter) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_hours_letter, DEFAULT_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].detail_view_minutes_label) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_minutes_label, DEFAULT_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].detail_view_minutes_letter) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_minutes_letter, DEFAULT_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].detail_view_seconds_label) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_seconds_label, DEFAULT_VALUE_TEXT_COLOR, 0);
				}
				if (modal->gauge_ui[i].detail_view_seconds_letter) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_seconds_letter, DEFAULT_VALUE_TEXT_COLOR, 0);
				}
			}
		} else {
			// This is not the selected gauge - use normal colors or dim if should_dim
			lv_color_t value_text_color = should_dim ? DIM_VALUE_TEXT_COLOR : DEFAULT_VALUE_TEXT_COLOR;

			// Current view H, M, S labels
			if (modal->gauge_ui[i].current_view_hours_label) {
				lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_hours_label, value_text_color, 0);
			}
			if (modal->gauge_ui[i].current_view_hours_letter) {
				lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_hours_letter, value_text_color, 0);
			}
			if (modal->gauge_ui[i].current_view_minutes_label) {
				lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_minutes_label, value_text_color, 0);
			}
			if (modal->gauge_ui[i].current_view_minutes_letter) {
				lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_minutes_letter, value_text_color, 0);
			}
			if (modal->gauge_ui[i].current_view_seconds_label) {
				lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_seconds_label, value_text_color, 0);
			}
			if (modal->gauge_ui[i].current_view_seconds_letter) {
				lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_seconds_letter, value_text_color, 0);
			}
			// Detail view H, M, S labels
			if (modal->gauge_ui[i].detail_view_hours_label) {
				lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_hours_label, value_text_color, 0);
			}
			if (modal->gauge_ui[i].detail_view_hours_letter) {
				lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_hours_letter, value_text_color, 0);
			}
			if (modal->gauge_ui[i].detail_view_minutes_label) {
				lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_minutes_label, value_text_color, 0);
			}
			if (modal->gauge_ui[i].detail_view_minutes_letter) {
				lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_minutes_letter, value_text_color, 0);
			}
			if (modal->gauge_ui[i].detail_view_seconds_label) {
				lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_seconds_label, value_text_color, 0);
			}
			if (modal->gauge_ui[i].detail_view_seconds_letter) {
				lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_seconds_letter, value_text_color, 0);
			}
		}

		// Set group container and title colors - only one view can be active at a time
		if (i == modal->selected_gauge) {
			// This is the selected gauge - highlight active view with cyan, dim inactive with gray
			if (modal->selected_is_current_view) {
				// Current view is active - priority: being_edited > has_changed > active
				if (modal->gauge_ui[i].current_view_group) {
					lv_color_t border_color;
					int border_width;
					if (current_view_being_edited) {
						border_color = BEING_EDITED_GROUP_BORDER_COLOR;
						border_width = BEING_EDITED_GROUP_BORDER_WIDTH;
					} else if (current_view_has_changed) {
						border_color = CHANGED_GAUGE_SECTION_BORDER_COLOR;
						border_width = CHANGED_GAUGE_SECTION_BORDER_WIDTH;
					} else {
						border_color = ACTIVE_VIEW_CONTAINER_BORDER_COLOR;
						border_width = ACTIVE_VIEW_CONTAINER_BORDER_WIDTH;
					}
					lv_obj_set_style_border_color(modal->gauge_ui[i].current_view_group, border_color, 0);
					lv_obj_set_style_border_width(modal->gauge_ui[i].current_view_group, border_width, 0);
				}
				if (modal->gauge_ui[i].current_view_title) {
					lv_color_t title_bg_color;
					lv_color_t title_text_color;
					if (current_view_being_edited) {
						title_bg_color = BEING_EDITED_GROUP_TITLE_BACKGROUND_COLOR;
						title_text_color = BEING_EDITED_GROUP_TITLE_TEXT_COLOR;
					} else if (current_view_has_changed) {
						title_bg_color = CHANGED_GAUGE_TITLE_BACKGROUND_COLOR;
						title_text_color = CHANGED_GAUGE_TITLE_TEXT_COLOR;
					} else {
						title_bg_color = ACTIVE_VIEW_TITLE_BACKGROUND_COLOR;
						title_text_color = ACTIVE_VIEW_TITLE_TEXT_COLOR;
					}
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_title, title_text_color, 0);
					lv_obj_set_style_bg_color(modal->gauge_ui[i].current_view_title, title_bg_color, 0);
				}

				// Detail view is inactive - gray border and title
				if (modal->gauge_ui[i].detail_view_group) {
					lv_obj_set_style_border_color(modal->gauge_ui[i].detail_view_group, INACTIVE_VIEW_CONTAINER_BORDER_COLOR, 0);
					lv_obj_set_style_border_width(modal->gauge_ui[i].detail_view_group, INACTIVE_VIEW_CONTAINER_BORDER_WIDTH, 0);
				}
				if (modal->gauge_ui[i].detail_view_title) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_title, INACTIVE_VIEW_TITLE_TEXT_COLOR, 0);
					lv_obj_set_style_bg_color(modal->gauge_ui[i].detail_view_title, INACTIVE_VIEW_TITLE_BACKGROUND_COLOR, 0);
				}
			} else {
				// Detail view is active - priority: being_edited > has_changed > active
				if (modal->gauge_ui[i].detail_view_group) {
					lv_color_t border_color;
					int border_width;
					if (detail_view_being_edited) {
						border_color = BEING_EDITED_GROUP_BORDER_COLOR;
						border_width = BEING_EDITED_GROUP_BORDER_WIDTH;
					} else if (detail_view_has_changed) {
						border_color = CHANGED_GAUGE_SECTION_BORDER_COLOR;
						border_width = CHANGED_GAUGE_SECTION_BORDER_WIDTH;
					} else {
						border_color = ACTIVE_VIEW_CONTAINER_BORDER_COLOR;
						border_width = ACTIVE_VIEW_CONTAINER_BORDER_WIDTH;
					}
					lv_obj_set_style_border_color(modal->gauge_ui[i].detail_view_group, border_color, 0);
					lv_obj_set_style_border_width(modal->gauge_ui[i].detail_view_group, border_width, 0);
				}
				if (modal->gauge_ui[i].detail_view_title) {
					lv_color_t title_bg_color;
					lv_color_t title_text_color;
					if (detail_view_being_edited) {
						title_bg_color = BEING_EDITED_GROUP_TITLE_BACKGROUND_COLOR;
						title_text_color = BEING_EDITED_GROUP_TITLE_TEXT_COLOR;
					} else if (detail_view_has_changed) {
						title_bg_color = CHANGED_GAUGE_TITLE_BACKGROUND_COLOR;
						title_text_color = CHANGED_GAUGE_TITLE_TEXT_COLOR;
					} else {
						title_bg_color = ACTIVE_VIEW_TITLE_BACKGROUND_COLOR;
						title_text_color = ACTIVE_VIEW_TITLE_TEXT_COLOR;
					}
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_title, title_text_color, 0);
					lv_obj_set_style_bg_color(modal->gauge_ui[i].detail_view_title, title_bg_color, 0);
				}

				// Current view is inactive - gray border and title
				if (modal->gauge_ui[i].current_view_group) {
					lv_obj_set_style_border_color(modal->gauge_ui[i].current_view_group, INACTIVE_VIEW_CONTAINER_BORDER_COLOR, 0);
					lv_obj_set_style_border_width(modal->gauge_ui[i].current_view_group, INACTIVE_VIEW_CONTAINER_BORDER_WIDTH, 0);
				}
				if (modal->gauge_ui[i].current_view_title) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_title, INACTIVE_VIEW_TITLE_TEXT_COLOR, 0);
					lv_obj_set_style_bg_color(modal->gauge_ui[i].current_view_title, INACTIVE_VIEW_TITLE_BACKGROUND_COLOR, 0);
				}
			}
		} else {
			// This is not the selected gauge - dim only if there's an active view, otherwise use default colors
			if (has_selected_gauge) {
				// There's an active view - dim this gauge
				if (modal->gauge_ui[i].current_view_group) {
					lv_obj_set_style_border_color(modal->gauge_ui[i].current_view_group, DIM_GROUP_BORDER_COLOR, 0);
					lv_obj_set_style_border_width(modal->gauge_ui[i].current_view_group, DIM_GROUP_BORDER_WIDTH, 0);
				}
				if (modal->gauge_ui[i].current_view_title) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_title, DIM_GROUP_TITLE_TEXT_COLOR, 0);
					lv_obj_set_style_bg_color(modal->gauge_ui[i].current_view_title, DIM_GROUP_TITLE_BACKGROUND_COLOR, 0);
				}

				if (modal->gauge_ui[i].detail_view_group) {
					lv_obj_set_style_border_color(modal->gauge_ui[i].detail_view_group, DIM_GROUP_BORDER_COLOR, 0);
					lv_obj_set_style_border_width(modal->gauge_ui[i].detail_view_group, DIM_GROUP_BORDER_WIDTH, 0);
				}
				if (modal->gauge_ui[i].detail_view_title) {
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_title, DIM_GROUP_TITLE_TEXT_COLOR, 0);
					lv_obj_set_style_bg_color(modal->gauge_ui[i].detail_view_title, DIM_GROUP_TITLE_BACKGROUND_COLOR, 0);
				}
			} else {
				// No active view - use default colors or green if changed
				// Current view
				if (modal->gauge_ui[i].current_view_group) {
					lv_color_t border_color = current_view_has_changed ? CHANGED_GAUGE_SECTION_BORDER_COLOR : DEFAULT_GROUP_BORDER_COLOR;
					lv_obj_set_style_border_color(modal->gauge_ui[i].current_view_group, border_color, 0);
					lv_obj_set_style_border_width(modal->gauge_ui[i].current_view_group, DEFAULT_GROUP_BORDER_WIDTH, 0);
				}
				if (modal->gauge_ui[i].current_view_title) {
					lv_color_t title_bg_color = current_view_has_changed ? CHANGED_GAUGE_TITLE_BACKGROUND_COLOR : DEFAULT_CURRENT_VIEW_TITLE_BACKGROUND_COLOR;
					lv_color_t title_text_color = current_view_has_changed ? CHANGED_GAUGE_TITLE_TEXT_COLOR : DEFAULT_CURRENT_VIEW_TITLE_TEXT_COLOR;
					lv_obj_set_style_text_color(modal->gauge_ui[i].current_view_title, title_text_color, 0);
					lv_obj_set_style_bg_color(modal->gauge_ui[i].current_view_title, title_bg_color, 0);
				}

				// Detail view
				if (modal->gauge_ui[i].detail_view_group) {
					lv_color_t border_color = detail_view_has_changed ? CHANGED_GAUGE_SECTION_BORDER_COLOR : DEFAULT_GROUP_BORDER_COLOR;
					lv_obj_set_style_border_color(modal->gauge_ui[i].detail_view_group, border_color, 0);
					lv_obj_set_style_border_width(modal->gauge_ui[i].detail_view_group, DEFAULT_GROUP_BORDER_WIDTH, 0);
				}
				if (modal->gauge_ui[i].detail_view_title) {
					lv_color_t title_bg_color = detail_view_has_changed ? CHANGED_GAUGE_TITLE_BACKGROUND_COLOR : DEFAULT_DETAIL_VIEW_TITLE_BACKGROUND_COLOR;
					lv_color_t title_text_color = detail_view_has_changed ? CHANGED_GAUGE_TITLE_TEXT_COLOR : DEFAULT_DETAIL_VIEW_TITLE_TEXT_COLOR;
					lv_obj_set_style_text_color(modal->gauge_ui[i].detail_view_title, title_text_color, 0);
					lv_obj_set_style_bg_color(modal->gauge_ui[i].detail_view_title, title_bg_color, 0);
				}
			}
		}
	}
}

// Find gauge by direct object reference - no loops needed
static int find_gauge_by_section(timeline_modal_t* modal, lv_obj_t* target)
{
	if (!modal || !target) {
		printf("[D] timeline_modal: find_gauge_by_section early return - modal: %p, target: %p\n", modal, target);
		return -1;
	}

	printf("[D] timeline_modal: find_gauge_by_section checking %d gauges\n", modal->config.gauge_count);

	// Direct object comparison - flat data structure, no loops needed
	for (int i = 0; i < modal->config.gauge_count; i++) {
		// Check if target is the gauge container (stored reference)
		if (modal->gauge_ui[i].gauge_container == target) {
			printf("[D] timeline_modal: Found matching gauge %d (gauge container)\n", i);
			return i;
		}

		// Check if target is the gauge section itself
		if (modal->gauge_sections[i] == target) {
			printf("[D] timeline_modal: Found matching gauge %d (gauge section)\n", i);
			return i;
		}

		// Check if target is a group container
		if (modal->gauge_ui[i].current_view_group == target ||
			modal->gauge_ui[i].detail_view_group == target) {
			printf("[D] timeline_modal: Found matching gauge %d (group container)\n", i);
			return i;
		}

		// Check if target is a value label (individual H, M, S components)
		if (modal->gauge_ui[i].current_view_hours_label == target ||
			modal->gauge_ui[i].current_view_hours_letter == target ||
			modal->gauge_ui[i].current_view_minutes_label == target ||
			modal->gauge_ui[i].current_view_minutes_letter == target ||
			modal->gauge_ui[i].current_view_seconds_label == target ||
			modal->gauge_ui[i].current_view_seconds_letter == target ||
			modal->gauge_ui[i].detail_view_hours_label == target ||
			modal->gauge_ui[i].detail_view_hours_letter == target ||
			modal->gauge_ui[i].detail_view_minutes_label == target ||
			modal->gauge_ui[i].detail_view_minutes_letter == target ||
			modal->gauge_ui[i].detail_view_seconds_label == target ||
			modal->gauge_ui[i].detail_view_seconds_letter == target) {
			printf("[D] timeline_modal: Found matching gauge %d (value label)\n", i);
			return i;
		}

		// Check if target is a title
		if (modal->gauge_titles[i] == target ||
			modal->gauge_ui[i].current_view_title == target ||
			modal->gauge_ui[i].detail_view_title == target) {
			printf("[D] timeline_modal: Found matching gauge %d (title)\n", i);
			return i;
		}
	}

	printf("[D] timeline_modal: No matching gauge found\n");
	return -1;
}

// Update selected gauge highlight (legacy function for compatibility)
static void update_selected_gauge_highlight( timeline_modal_t* modal )
{

	update_gauge_ui( modal );
}


// Timeline click handler - single listener for all clicks
static void timeline_click_handler(lv_event_t* e)
{
	lv_obj_t* target = lv_event_get_target( e );
	timeline_modal_t* modal = (timeline_modal_t*)lv_event_get_user_data( e );

	printf("[D] timeline_modal: timeline_click_handler called, target: %p, modal: %p\n", target, modal);

	if( !modal || !target ){

		printf("[D] timeline_modal: Early return - modal: %p, target: %p\n", modal, target);
		return;
	}

	// Check if a gauge section was clicked
	int gauge_index = find_gauge_by_section( modal, target );

	if (gauge_index >= 0) {
		// Check if a timeline value label was clicked (these should activate the view)
		bool is_current_view = (target == modal->gauge_ui[gauge_index].current_view_hours_label ||
								target == modal->gauge_ui[gauge_index].current_view_minutes_label ||
								target == modal->gauge_ui[gauge_index].current_view_minutes_letter ||
								target == modal->gauge_ui[gauge_index].current_view_seconds_label ||
								target == modal->gauge_ui[gauge_index].current_view_seconds_letter);
		bool is_detail_view = (target == modal->gauge_ui[gauge_index].detail_view_hours_label ||
								target == modal->gauge_ui[gauge_index].detail_view_hours_letter ||
								target == modal->gauge_ui[gauge_index].detail_view_minutes_label ||
								target == modal->gauge_ui[gauge_index].detail_view_minutes_letter ||
								target == modal->gauge_ui[gauge_index].detail_view_seconds_label ||
								target == modal->gauge_ui[gauge_index].detail_view_seconds_letter);

		// Check if a group container was clicked (these should also activate the view)
		bool is_current_group = (target == modal->gauge_ui[gauge_index].current_view_group);
		bool is_detail_group = (target == modal->gauge_ui[gauge_index].detail_view_group);

		printf("[D] timeline_modal: View detection - current_view: %d, detail_view: %d, current_group: %d, detail_group: %d\n",
			is_current_view, is_detail_view, is_current_group, is_detail_group);

		if (is_current_view || is_current_group) {
			// Current view clicked - check if it's already active
			if (modal->selected_gauge == gauge_index && modal->selected_is_current_view) {
				// Same current view clicked - deactivate it
				if (modal->time_input) {
					time_input_hide(modal->time_input);
				}
				modal->gauge_ui[gauge_index].current_view_being_edited = false;
				modal->selected_gauge = -1;
				update_gauge_ui(modal);
				printf("[I] timeline_modal: Current view deactivated\n");
			} else {
				// Current view clicked - activate it
				// Clear being_edited for previously selected gauge
				if (modal->selected_gauge >= 0 && modal->selected_gauge < modal->config.gauge_count) {
					modal->gauge_ui[modal->selected_gauge].current_view_being_edited = false;
					modal->gauge_ui[modal->selected_gauge].detail_view_being_edited = false;
				}
				modal->selected_gauge = gauge_index;
				modal->selected_is_current_view = true;
				modal->gauge_ui[gauge_index].current_view_being_edited = true;
				update_gauge_ui(modal);

			// Show time input component beside the selected gauge
			if (modal->time_input) {
					float duration = modal->gauge_ui[gauge_index].current_view_duration;
				int total_seconds = (int)duration;
				int hours = total_seconds / 3600;
					int minutes = (total_seconds % 3600) / 60;
				int seconds = total_seconds % 60;

				time_input_set_values(modal->time_input, hours, minutes, seconds);
					time_input_show_outside_container(modal->time_input, modal->gauge_sections[gauge_index], modal->gauge_ui[gauge_index].gauge_container);

					printf("[I] timeline_modal: Gauge %d current view activated, duration: %.2f\n", gauge_index, duration);
				}
			}
		} else if (is_detail_view || is_detail_group) {
			// Detail view clicked - check if it's already active
			if (modal->selected_gauge == gauge_index && !modal->selected_is_current_view) {
				// Same detail view clicked - deactivate it
				if (modal->time_input) {
					time_input_hide(modal->time_input);
				}
				modal->gauge_ui[gauge_index].detail_view_being_edited = false;
				modal->selected_gauge = -1;
				update_gauge_ui(modal);
				printf("[I] timeline_modal: Detail view deactivated\n");
			} else {
				// Detail view clicked - activate it
				// Clear being_edited for previously selected gauge
				if (modal->selected_gauge >= 0 && modal->selected_gauge < modal->config.gauge_count) {
					modal->gauge_ui[modal->selected_gauge].current_view_being_edited = false;
					modal->gauge_ui[modal->selected_gauge].detail_view_being_edited = false;
				}
				modal->selected_gauge = gauge_index;
				modal->selected_is_current_view = false;
				modal->gauge_ui[gauge_index].detail_view_being_edited = true;
				update_gauge_ui(modal);

				// Show time input component beside the selected gauge
				if (modal->time_input) {
					float duration = modal->gauge_ui[gauge_index].detail_view_duration;
					int total_seconds = (int)duration;
					int hours = total_seconds / 3600;
					int minutes = (total_seconds % 3600) / 60;
					int seconds = total_seconds % 60;

					time_input_set_values(modal->time_input, hours, minutes, seconds);
					time_input_show_outside_container(modal->time_input, modal->gauge_sections[gauge_index], modal->gauge_ui[gauge_index].gauge_container);

					printf("[I] timeline_modal: Gauge %d detail view activated, duration: %.2f\n", gauge_index, duration);
				}
			}
		} else {
			// Clicked on gauge container, gauge section, or title - toggle behavior
			if (modal->selected_gauge == gauge_index) {
				// Same gauge clicked - toggle off (close time input and deselect)
				if (modal->time_input) {
				time_input_hide(modal->time_input);
				}
				modal->selected_gauge = -1;
				update_gauge_ui(modal);
				printf("[I] timeline_modal: Same gauge clicked, hiding time input (toggle off)\n");
			} else {
				// Different gauge - select it and show time input (default to current view)
				modal->selected_gauge = gauge_index;
				modal->selected_is_current_view = true;
				update_gauge_ui(modal);

				// Show time input component beside the selected gauge
				if (modal->time_input) {
					float duration = modal->gauge_ui[gauge_index].current_view_duration;
					int total_seconds = (int)duration;
					int hours = total_seconds / 3600;
					int minutes = (total_seconds % 3600) / 60;
					int seconds = total_seconds % 60;

					time_input_set_values(modal->time_input, hours, minutes, seconds);
					time_input_show_outside_container(modal->time_input, modal->gauge_sections[gauge_index], modal->gauge_ui[gauge_index].gauge_container);

					printf("[I] timeline_modal: Gauge %d selected, duration: %.2f\n", gauge_index, duration);
				}
			}
		}
	} else {
		// Click was not on a gauge - check if we should close any open view
		bool is_gauge_container = false;

		// Check if click was on any gauge container across all gauges
		for (int i = 0; i < modal->config.gauge_count; i++) {
			if (target == modal->gauge_ui[i].gauge_container ||
				target == modal->gauge_sections[i] ||
				target == modal->gauge_titles[i] ||
				target == modal->gauge_ui[i].current_view_group ||
				target == modal->gauge_ui[i].detail_view_group ||
				target == modal->gauge_ui[i].current_view_hours_label ||
				target == modal->gauge_ui[i].current_view_hours_letter ||
				target == modal->gauge_ui[i].current_view_minutes_label ||
				target == modal->gauge_ui[i].current_view_minutes_letter ||
				target == modal->gauge_ui[i].current_view_seconds_label ||
				target == modal->gauge_ui[i].current_view_seconds_letter ||
				target == modal->gauge_ui[i].detail_view_hours_label ||
				target == modal->gauge_ui[i].detail_view_hours_letter ||
				target == modal->gauge_ui[i].detail_view_minutes_label ||
				target == modal->gauge_ui[i].detail_view_minutes_letter ||
				target == modal->gauge_ui[i].detail_view_seconds_label ||
				target == modal->gauge_ui[i].detail_view_seconds_letter ||
				target == modal->gauge_ui[i].current_view_title ||
				target == modal->gauge_ui[i].detail_view_title) {
				is_gauge_container = true;
				break;
			}
		}

		// If not a gauge container and there's an active view, close it
		if (!is_gauge_container && modal->selected_gauge >= 0) {
		if (modal->time_input) {
			time_input_hide(modal->time_input);
		}
		modal->selected_gauge = -1;
			update_gauge_ui(modal);
			printf("[I] timeline_modal: Clicked outside gauge container, closed active view\n");
		} else if (is_gauge_container) {
			printf("[I] timeline_modal: Clicked on gauge container but gauge not found - this shouldn't happen\n");
		}
	}
}


// Close button clicked event
static void close_button_clicked(lv_event_t* e)
{
	timeline_modal_t* modal = (timeline_modal_t*)lv_event_get_user_data(e);
	if (modal) {
		printf("[I] timeline_modal: Close button clicked\n");
		timeline_modal_hide(modal);
	}
}

// Cancel button clicked event
static void cancel_button_clicked(lv_event_t* e)
{
	timeline_modal_t* modal = (timeline_modal_t*)lv_event_get_user_data(e);
	if (modal) {
		printf("[I] timeline_modal: Cancel button clicked\n");
		timeline_modal_hide(modal);
	}
}

// Create timeline modal
timeline_modal_t* timeline_modal_create(const timeline_modal_config_t* config, void (*on_close_callback)(void))
{
	printf("[I] timeline_modal: Creating timeline modal\n");

	if (!config) {
		printf("[E] timeline_modal: Configuration is required\n");
		return NULL;
	}

	if (config->gauge_count <= 0 || !config->gauges) {
		printf("[E] timeline_modal: Invalid gauge configuration\n");
		return NULL;
	}

	timeline_modal_t* modal = malloc(sizeof(timeline_modal_t));
	if (!modal) {
		printf("[E] timeline_modal: Failed to allocate memory for timeline modal\n");
		return NULL;
	}

	memset(modal, 0, sizeof(timeline_modal_t));

	// Store configuration
	modal->config = *config;
	modal->current_duration = 30; // Default to 30 seconds

	// Allocate dynamic arrays
	modal->gauge_sections = calloc(config->gauge_count, sizeof(lv_obj_t*));
	modal->gauge_titles = calloc(config->gauge_count, sizeof(lv_obj_t*));
	modal->gauge_ui = calloc(config->gauge_count, sizeof(timeline_ui_t));

	if (!modal->gauge_sections || !modal->gauge_titles || !modal->gauge_ui) {
		printf("[E] timeline_modal: Failed to allocate memory for UI arrays\n");
		free(modal);
		return NULL;
	}

	// Initialize gauge UI fields
	for (int i = 0; i < config->gauge_count; i++) {
		// Initialize individual durations
		modal->gauge_ui[i].current_view_duration = 30.0f;
		modal->gauge_ui[i].detail_view_duration = 30.0f;
		modal->gauge_ui[i].current_view_has_changed = false;
		modal->gauge_ui[i].detail_view_has_changed = false;
	}

	// Create animation manager
	animation_config_t anim_config = {
		.duration = 0.3f,    // 0.3 second animation (faster)
		.frame_rate = 16     // ~60 FPS
	};
	modal->animation_manager = animation_manager_create(
		config->gauge_count, &anim_config,
		gauge_animation_callback, modal
	);

	modal->on_close = on_close_callback;

	// Modal Container
	modal->background = lv_obj_create(lv_screen_active());
	lv_obj_set_size(modal->background, LV_PCT(100), LV_PCT(100));
	lv_obj_set_pos(modal->background, 0, 0);
	lv_obj_set_style_bg_color(modal->background, PALETTE_BLACK, 0);
	lv_obj_set_style_bg_opa(modal->background, LV_OPA_COVER, 0); // Use full opacity for better performance
	lv_obj_set_style_border_width(modal->background, 0, 0);
	lv_obj_set_style_pad_top(modal->background, 0, 0);
	lv_obj_set_style_pad_bottom(modal->background, 0, 0);
	lv_obj_set_style_pad_left(modal->background, 5, 0);
	lv_obj_set_style_pad_right(modal->background, 5, 0);
	lv_obj_clear_flag(modal->background, LV_OBJ_FLAG_SCROLLABLE);

	// Content Container
	modal->content_container = lv_obj_create(modal->background);
	lv_obj_set_size(modal->content_container, LV_PCT(100), LV_PCT(100));
	// lv_obj_align(modal->content_container, LV_ALIGN_CENTER, 0, 0);
	lv_obj_set_style_bg_color(modal->content_container, PALETTE_BLACK, 0);
	lv_obj_set_style_border_color(modal->content_container, PALETTE_BLACK, 0);
	lv_obj_set_style_border_width(modal->content_container, 0, 0);
	lv_obj_set_style_pad_left(modal->content_container, 5, 0);
	lv_obj_set_style_pad_right(modal->content_container, 5, 0);
	lv_obj_set_style_pad_top(modal->content_container, 0, 0);
	lv_obj_set_style_pad_bottom(modal->content_container, 0, 0);
	lv_obj_clear_flag(modal->content_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(modal->content_container, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(modal->content_container, timeline_click_handler, LV_EVENT_CLICKED, modal);

	// Flexbox Rules
	lv_obj_set_layout(modal->content_container, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(modal->content_container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(modal->content_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

	// Gauges Container
	lv_obj_t* gauges_container = lv_obj_create(modal->content_container);
	lv_obj_set_size(gauges_container, LV_PCT(100), LV_PCT(91));
	lv_obj_set_layout(gauges_container, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(gauges_container, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(gauges_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_bg_color(gauges_container, PALETTE_BLACK, 0);
	lv_obj_set_style_bg_opa(gauges_container, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(gauges_container, 0, 0);
	lv_obj_set_style_pad_all(gauges_container, 5, 0);
	lv_obj_clear_flag(gauges_container, LV_OBJ_FLAG_SCROLLABLE);

	// Gauge Sections
	for (int i = 0; i < config->gauge_count; i++) {

		create_gauge_section( modal, i, gauges_container );
	}

	// Initialize selected gauge
	modal->selected_gauge = -1; // No gauge selected initially
	modal->selected_is_current_view = true; // Default to current view

	// Create shared time input component
	time_input_config_t time_config = TIME_INPUT_DEFAULT_CONFIG;
	modal->time_input = time_input_create( &time_config, modal->background );

	if( modal->time_input ){
		// Set up callbacks
		time_input_set_callbacks(
			modal->time_input,
			time_input_value_changed,
			time_input_enter,
			time_input_cancel,
			modal
		);
	}

	// Button Container
	lv_obj_t* button_container = lv_obj_create(modal->content_container);
	lv_obj_set_size(button_container, LV_PCT(100), LV_PCT(9));
	lv_obj_set_layout(button_container, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(button_container, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(button_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_bg_color(button_container, PALETTE_BLACK, 0);
	lv_obj_set_style_bg_opa(button_container, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(button_container, 0, 0);
	lv_obj_set_style_pad_all(button_container, 0, 0);
	lv_obj_clear_flag(button_container, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(button_container, LV_OBJ_FLAG_EVENT_BUBBLE);
	lv_obj_add_flag(button_container, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(button_container, timeline_click_handler, LV_EVENT_CLICKED, modal);

	// Cancel Button - left side (RED)
	modal->cancel_button = lv_button_create(button_container);
	lv_obj_set_size(modal->cancel_button, 100, 50);
	lv_obj_set_style_bg_color(modal->cancel_button, PALETTE_BLACK, 0);
	lv_obj_set_style_bg_color(modal->cancel_button, PALETTE_RED, LV_STATE_PRESSED);
	lv_obj_set_style_border_width(modal->cancel_button, 2, 0);
	lv_obj_set_style_border_color(modal->cancel_button, PALETTE_RED, 0);
	lv_obj_set_style_text_color(modal->cancel_button, PALETTE_RED, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(modal->cancel_button, PALETTE_BLACK, LV_PART_MAIN | LV_STATE_PRESSED);
	lv_obj_set_style_radius(modal->cancel_button, 8, 0);
	lv_obj_set_style_pad_all(modal->cancel_button, 8, 0);
	lv_obj_set_style_shadow_width(modal->cancel_button, 0, 0); // Remove drop shadow
	lv_obj_add_event_cb(modal->cancel_button, cancel_button_clicked, LV_EVENT_CLICKED, modal);

	lv_obj_t* cancel_label = lv_label_create(modal->cancel_button);
	lv_label_set_text(cancel_label, "CANCEL");
	lv_obj_center(cancel_label);

	// Close Button - right side (GREEN, "DONE")
	modal->close_button = lv_button_create(button_container);
	lv_obj_set_size(modal->close_button, 100, 50);
	lv_obj_set_style_bg_color(modal->close_button, PALETTE_BLACK, 0);
	lv_obj_set_style_bg_color(modal->close_button, PALETTE_GREEN, LV_STATE_PRESSED);
	lv_obj_set_style_border_width(modal->close_button, 2, 0);
	lv_obj_set_style_border_color(modal->close_button, PALETTE_GREEN, 0);
	lv_obj_set_style_text_color(modal->close_button, PALETTE_GREEN, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_text_color(modal->close_button, PALETTE_BLACK, LV_PART_MAIN | LV_STATE_PRESSED);
	lv_obj_set_style_radius(modal->close_button, 8, 0);
	lv_obj_set_style_pad_all(modal->close_button, 8, 0);
	lv_obj_set_style_shadow_width(modal->close_button, 0, 0); // Remove drop shadow
	lv_obj_add_event_cb(modal->close_button, close_button_clicked, LV_EVENT_CLICKED, modal);

	lv_obj_t* close_label = lv_label_create(modal->close_button);
	lv_label_set_text(close_label, "DONE");
	lv_obj_center(close_label);

	// Add single click handler to the modal background for all clicks
	lv_obj_add_event_cb(modal->background, timeline_click_handler, LV_EVENT_CLICKED, modal);

	// Initialize gauge borders
	update_gauge_ui(modal);

	// Load current gauge timeline settings from device state
	load_current_gauge_timeline_settings(modal);

	printf("[I] timeline_modal: Timeline modal created successfully\n");
	return modal;
}

// Show timeline modal
void timeline_modal_show(timeline_modal_t* modal)
{
	if (!modal) return;

	printf("[I] timeline_modal: Showing timeline modal\n");

	// Update all gauge displays to show current duration
	for (int i = 0; i < modal->config.gauge_count; i++) {
		update_timeline_display(modal, i, true);  // Current view
		update_timeline_display(modal, i, false); // Detail view
	}

	lv_obj_clear_flag(modal->background, LV_OBJ_FLAG_HIDDEN);
	modal->is_visible = true;
}


// Hide timeline modal
void timeline_modal_hide(timeline_modal_t* modal)
{
	if (!modal) return;

	printf("[I] timeline_modal: Hiding timeline modal\n");

	// Individual changes are already saved via callback, no need to save all

	lv_obj_add_flag(modal->background, LV_OBJ_FLAG_HIDDEN);
	modal->is_visible = false;
}

// Check if timeline modal is visible
bool timeline_modal_is_visible(timeline_modal_t* modal)
{
	if (!modal) return false;
	return modal->is_visible;
}

// Get current timeline duration
int timeline_modal_get_duration(timeline_modal_t* modal)
{
	if (!modal) return -1;
	return modal->current_duration;
}

// Set gauge value
void timeline_modal_set_gauge_value(timeline_modal_t* modal, int gauge, float value)
{
	if (!modal || gauge < 0 || gauge >= modal->config.gauge_count) return;

	update_gauge_value(modal, gauge, value, true); // Default to current view
}

// Load current gauge timeline settings from device state
static void load_current_gauge_timeline_settings(timeline_modal_t* modal)
{
	if (!modal) return;

	printf("[I] timeline_modal: Loading current gauge timeline settings\n");

	// Load current timeline duration for each gauge
	for (int i = 0; i < modal->config.gauge_count; i++) {
		// Map gauge index to data type
		power_monitor_data_type_t gauge_type;
		switch (i) {
			case 0: gauge_type = POWER_MONITOR_DATA_STARTER_VOLTAGE; break;
			case 1: gauge_type = POWER_MONITOR_DATA_STARTER_CURRENT; break;
			case 2: gauge_type = POWER_MONITOR_DATA_HOUSE_VOLTAGE; break;
			case 3: gauge_type = POWER_MONITOR_DATA_HOUSE_CURRENT; break;
			case 4: gauge_type = POWER_MONITOR_DATA_SOLAR_VOLTAGE; break;
			case 5: gauge_type = POWER_MONITOR_DATA_SOLAR_CURRENT; break;
			default:
				printf("[E] timeline_modal: Invalid gauge index %d\n", i);
				continue;
		}

		// Get current and detail view timeline durations for this gauge
		// Helper function to convert gauge type to string name
		const char* gauge_type_to_string(power_monitor_data_type_t gauge_type) {
			switch (gauge_type) {
				case POWER_MONITOR_DATA_STARTER_VOLTAGE: return "starter_voltage";
				case POWER_MONITOR_DATA_STARTER_CURRENT: return "starter_current";
				case POWER_MONITOR_DATA_HOUSE_VOLTAGE: return "house_voltage";
				case POWER_MONITOR_DATA_HOUSE_CURRENT: return "house_current";
				case POWER_MONITOR_DATA_SOLAR_VOLTAGE: return "solar_voltage";
				case POWER_MONITOR_DATA_SOLAR_CURRENT: return "solar_current";
				default: return "unknown";
			}
		}

		char current_path[256];
		char detail_path[256];
		snprintf(current_path, sizeof(current_path), "power_monitor.gauge_timeline_settings.%s.current_view", gauge_type_to_string(gauge_type));
		snprintf(detail_path, sizeof(detail_path), "power_monitor.gauge_timeline_settings.%s.detail_view", gauge_type_to_string(gauge_type));

		int current_duration = device_state_get_int(current_path);
		int detail_duration = device_state_get_int(detail_path);
		// Set each view duration to its respective loaded value
		modal->gauge_ui[i].current_view_duration = (float)current_duration;
		modal->gauge_ui[i].detail_view_duration = (float)detail_duration;
		// Store original values for comparison
		modal->gauge_ui[i].original_current_view_duration = (float)current_duration;
		modal->gauge_ui[i].original_detail_view_duration = (float)detail_duration;
		// Initialize being_edited flags
		modal->gauge_ui[i].current_view_being_edited = false;
		modal->gauge_ui[i].detail_view_being_edited = false;

		printf("[I] timeline_modal: Gauge %d (%d) current duration: %d seconds, detail duration: %d seconds\n",
			   i, gauge_type, current_duration, detail_duration);

		// Update the UI display for this gauge
		update_timeline_display(modal, i, true);  // Current view
		update_timeline_display(modal, i, false); // Detail view
	}
}

// Deferred destroy state
static bool s_timeline_destroy_pending = false;
static lv_timer_t* s_timeline_destroy_timer = NULL;

static void timeline_modal_destroy_timer_cb(lv_timer_t* timer)
{
	// Retrieve modal pointer passed as user data
	timeline_modal_t* modal = (timeline_modal_t*)lv_timer_get_user_data(timer);
	printf("[I] timeline_modal: (timer) Destroying timeline modal\n");

	if (!modal) {
		if (timer) lv_timer_del(timer);
		s_timeline_destroy_timer = NULL;
		s_timeline_destroy_pending = false;
		return;
	}

	// Ensure hidden to stop interactions
	timeline_modal_hide(modal);

	// Destroy subcomponents that own their own LVGL objects before deleting the tree
	if (modal->time_input) {
		time_input_destroy(modal->time_input);
		modal->time_input = NULL;
	}
	if (modal->animation_manager) {
		animation_manager_destroy(modal->animation_manager);
		modal->animation_manager = NULL;
	}

	// Delete the LVGL tree (removes event callbacks bound to the object)
	if (modal->background && lv_obj_is_valid(modal->background)) {
		lv_obj_del(modal->background);
		modal->background = NULL;
	}

	// Free allocated memory arrays
	if (modal->gauge_sections) {
		free(modal->gauge_sections);
		modal->gauge_sections = NULL;
	}
	if (modal->gauge_titles) {
		free(modal->gauge_titles);
		modal->gauge_titles = NULL;
	}
	if (modal->gauge_ui) {
		free(modal->gauge_ui);
		modal->gauge_ui = NULL;
	}

	// Free modal struct last
	free(modal);

	// Clear pending state and delete the timer
	s_timeline_destroy_pending = false;
	s_timeline_destroy_timer = NULL;
	if (timer) lv_timer_del(timer);
}

// Destroy timeline modal (deferred to avoid re-entrancy with LVGL events)
void timeline_modal_destroy(timeline_modal_t* modal)
{
	if (!modal) return;

	printf("[I] timeline_modal: Destroying timeline modal (deferred)\n");

	if (s_timeline_destroy_pending) {
		printf("[W] timeline_modal: Destroy already pending, ignoring duplicate request\n");
		return;
	}
	s_timeline_destroy_pending = true;

	// Cancel any previous timer defensively
	if (s_timeline_destroy_timer) {
		lv_timer_del(s_timeline_destroy_timer);
		s_timeline_destroy_timer = NULL;
	}

	// Schedule a short delay to allow LVGL to flush pending events before deletion
	s_timeline_destroy_timer = lv_timer_create(timeline_modal_destroy_timer_cb, 50, modal);
}