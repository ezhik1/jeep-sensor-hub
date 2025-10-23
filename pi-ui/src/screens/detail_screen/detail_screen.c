#include <stdio.h>
#include "detail_screen.h"
#include "../../displayModules/shared/palette.h"

#include "../../lvgl_port_pi.h"
#include "../../fonts/lv_font_noplato_24.h"
#include <stdlib.h>
#include <string.h>

// Simple modal tracking - only store modal pointers, read state from modal itself
typedef struct {
	char name[32];
	void* modal_ptr;
	void (*destroy_func)(void* modal);
	bool (*is_visible_func)(void* modal); // Function to check if modal is visible
} modal_tracking_t;

#define MAX_MODALS 10
static modal_tracking_t modal_tracking[MAX_MODALS] = {0};
static int modal_count = 0;

// Find modal tracking slot
static int detail_screen_find_modal_slot(const char* modal_name)
{
	for (int i = 0; i < modal_count; i++) {
		if (strcmp(modal_tracking[i].name, modal_name) == 0) {
			return i;
		}
	}
	return -1; // Not found
}

// Allocate new modal tracking slot
static int detail_screen_allocate_modal_slot(const char* modal_name)
{
	if (modal_count >= MAX_MODALS) {
		printf("[E] detail_screen: Maximum number of modals reached\n");
		return -1;
	}

	int slot = modal_count++;
	strncpy(modal_tracking[slot].name, modal_name, sizeof(modal_tracking[slot].name) - 1);
	modal_tracking[slot].name[sizeof(modal_tracking[slot].name) - 1] = '\0';
	modal_tracking[slot].modal_ptr = NULL;
	modal_tracking[slot].destroy_func = NULL;
	modal_tracking[slot].is_visible_func = NULL;

	return slot;
}

// Callback to clear modal pointer when modal is destroyed externally
static void detail_screen_modal_destroyed_callback(const char* modal_name)
{
	printf("[I] detail_screen: Modal destroyed callback for: %s\n", modal_name);

	int slot = detail_screen_find_modal_slot(modal_name);
	if (slot != -1) {
		modal_tracking[slot].modal_ptr = NULL;
		printf("[I] detail_screen: Modal %s pointer cleared\n", modal_name);
	}
}

// Wrapper callbacks for each modal type
static void detail_screen_timeline_modal_destroyed_wrapper(void)
{
	// Find the modal and hide it before clearing the pointer
	int slot = detail_screen_find_modal_slot("timeline");
	if (slot != -1 && modal_tracking[slot].modal_ptr) {
		extern void timeline_modal_hide(void* modal);
		timeline_modal_hide(modal_tracking[slot].modal_ptr);
	}
	detail_screen_modal_destroyed_callback("timeline");
}

static void detail_screen_alerts_modal_destroyed_wrapper(void)
{
	// Find the modal and hide it before clearing the pointer
	int slot = detail_screen_find_modal_slot("alerts");
	if (slot != -1 && modal_tracking[slot].modal_ptr) {
		extern void alerts_modal_hide(void* modal);
		alerts_modal_hide(modal_tracking[slot].modal_ptr);
	}
	detail_screen_modal_destroyed_callback("alerts");
}

void detail_screen_toggle_modal(const char* modal_name,
	void* (*create_func)(void* config, void (*on_close)(void)),
	void (*destroy_func)(void* modal),
	void (*show_func)(void* modal),
	bool (*is_visible_func)(void* modal), // Function to check modal visibility
	void* config,
	void (*on_close_callback)(void))
{
	printf("[I] detail_screen: Toggling modal: %s\n", modal_name);

	int slot = detail_screen_find_modal_slot(modal_name);
	if (slot == -1) {
		slot = detail_screen_allocate_modal_slot(modal_name);
		if (slot == -1) return;
	}

	// Check if modal exists and is visible by reading from modal itself
	bool is_visible = false;
	if (modal_tracking[slot].modal_ptr && modal_tracking[slot].is_visible_func) {
		printf("[D] detail_screen: Checking modal visibility for %s, modal_ptr=%p\n", modal_name, modal_tracking[slot].modal_ptr);
		is_visible = modal_tracking[slot].is_visible_func(modal_tracking[slot].modal_ptr);
		printf("[D] detail_screen: Modal %s visibility: %s\n", modal_name, is_visible ? "visible" : "hidden");
	} else {
		printf("[D] detail_screen: Modal %s not found or invalid (ptr=%p, func=%p)\n", modal_name, modal_tracking[slot].modal_ptr, modal_tracking[slot].is_visible_func);
	}

	if (is_visible) {
		// Close modal
		if (modal_tracking[slot].modal_ptr && modal_tracking[slot].destroy_func) {
			modal_tracking[slot].destroy_func(modal_tracking[slot].modal_ptr);
		}
		modal_tracking[slot].modal_ptr = NULL;
		printf("[I] detail_screen: Modal %s closed\n", modal_name);
	} else {
		// Open modal - use appropriate wrapper callback
		void (*wrapper_callback)(void) = NULL;
		if (strcmp(modal_name, "timeline") == 0) {
			wrapper_callback = detail_screen_timeline_modal_destroyed_wrapper;
		} else if (strcmp(modal_name, "alerts") == 0) {
			wrapper_callback = detail_screen_alerts_modal_destroyed_wrapper;
		}

		modal_tracking[slot].destroy_func = destroy_func;
		modal_tracking[slot].is_visible_func = is_visible_func;
		printf("[D] detail_screen: Creating modal %s with config=%p, callback=%p\n", modal_name, config, wrapper_callback);
		modal_tracking[slot].modal_ptr = create_func(config, wrapper_callback);
		printf("[D] detail_screen: Modal %s created, ptr=%p\n", modal_name, modal_tracking[slot].modal_ptr);
		if (modal_tracking[slot].modal_ptr) {
			printf("[D] detail_screen: Showing modal %s\n", modal_name);
			show_func(modal_tracking[slot].modal_ptr);
			printf("[I] detail_screen: Modal %s opened\n", modal_name);
		} else {
			printf("[E] detail_screen: Failed to create modal %s\n", modal_name);
		}
	}
}

bool detail_screen_is_modal_visible(const char* modal_name)
{
	int slot = detail_screen_find_modal_slot(modal_name);
	if (slot == -1 || !modal_tracking[slot].modal_ptr || !modal_tracking[slot].is_visible_func) {
		return false;
	}
	return modal_tracking[slot].is_visible_func(modal_tracking[slot].modal_ptr);
}

// Reset modal tracking system (call when detail screen is recreated)
void detail_screen_reset_modal_tracking(void)
{
	printf("[I] detail_screen: Resetting modal tracking system\n");
	for (int i = 0; i < modal_count; i++) {
		if (modal_tracking[i].modal_ptr && modal_tracking[i].destroy_func) {
			printf("[I] detail_screen: Destroying existing modal %s\n", modal_tracking[i].name);
			// Call destroy function and immediately clear the pointer to prevent double-free
			void* modal_ptr = modal_tracking[i].modal_ptr;
			modal_tracking[i].modal_ptr = NULL;
			modal_tracking[i].destroy_func(modal_ptr);
		}
		modal_tracking[i].modal_ptr = NULL;
		modal_tracking[i].destroy_func = NULL;
		modal_tracking[i].is_visible_func = NULL;
	}
	modal_count = 0;
}

static const char *TAG = "detail_screen_template";

// ============================================================================
// LAYOUT CONFIGURATION - Edit these values to change the layout
// ============================================================================

// Left column split percentages (must add up to 100)
#define CURRENT_VIEW_PERCENT    30    // Current view section percentage
#define RAW_VALUES_PERCENT      50    // Raw values section percentage
#define SETTINGS_PERCENT        20    // Settings section percentage

// Padding configuration
#define CONTAINER_GAP_PX        10    // Vertical gap between containers
#define OTHER_SECTIONS_PADDING  5     // Internal padding for raw values and settings

// Left column width as percentage of screen width
#define LEFT_COLUMN_WIDTH_PERCENT 50  // Left column takes 50% of screen width

// ============================================================================
// CALCULATED VALUES - These are computed from the configuration above
// ============================================================================

// Convert percentages to flexbox grow factors (divide by 10 to fit in uint8_t)
#define CURRENT_VIEW_GROW    (CURRENT_VIEW_PERCENT / 10)
#define RAW_VALUES_GROW      (RAW_VALUES_PERCENT / 10)
#define SETTINGS_GROW        (SETTINGS_PERCENT / 10)

// Internal event handlers
static void back_button_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		printf("[I] detail_screen: BACK button clicked\n");
		detail_screen_t *detail = (detail_screen_t*)lv_event_get_user_data(e);
		if (detail && detail->on_back_clicked) {
			printf("[I] detail_screen: Calling back button handler\n");
			detail->on_back_clicked();
		} else {
			printf("[W] detail_screen: Back button handler not available\n");
		}
	}
	// LV_STATE_PRESSED and LV_STATE_RELEASED are handled automatically by LVGL styling
}

static void setting_button_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_CLICKED) {
		printf("[I] detail_screen: Setting button clicked\n");
		lv_obj_t *button = lv_event_get_target(e);
		detail_screen_t *detail = (detail_screen_t*)lv_event_get_user_data(e);
		if (!detail) {
			printf("[W] detail_screen: No detail screen data available\n");
			return;
		}

		// Get the button index from the button's user data
		int button_index = (int)(intptr_t)lv_obj_get_user_data(button);
		printf("[I] detail_screen: Button index: %d\n", button_index);

		// Validate index and call the appropriate handler
		if (button_index >= 0 && button_index < detail->setting_buttons_count) {
			printf("[I] detail_screen: Setting button %d clicked: %s\n", button_index, detail->button_configs[button_index].text);

			// Call the button's click handler
			if (detail->button_configs[button_index].on_clicked) {
				printf("[I] detail_screen: Calling button handler for: %s\n", detail->button_configs[button_index].text);
				detail->button_configs[button_index].on_clicked();
			} else {
				printf("[W] detail_screen: No handler available for button: %s\n", detail->button_configs[button_index].text);
			}
		} else {
			printf("[W] detail_screen: Invalid button index: %d (count: %d)\n", button_index, detail->setting_buttons_count);
		}
	}
	// LV_STATE_PRESSED and LV_STATE_RELEASED are handled automatically by LVGL styling
}

static void view_container_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_PRESSED) {

	detail_screen_t *detail = (detail_screen_t*)lv_event_get_user_data(e);

		// Call the generic view callback if it exists
	if (detail->on_view_clicked) {

		detail->on_view_clicked();

		}
	}
}

detail_screen_t* detail_screen_create(const detail_screen_config_t* config)
{
	if (!config || !config->module_name || !config->display_name) {
		printf("[E] detail_screen: Invalid configuration for detail screen\n");
		return NULL;
	}

	// Reset modal tracking system when creating new detail screen
	detail_screen_reset_modal_tracking();

	detail_screen_t* detail = malloc(sizeof(detail_screen_t));
	if (!detail) {
		printf("[E] detail_screen: Failed to allocate detail screen\n");
		return NULL;
	}

	// Initialize structure
	memset(detail, 0, sizeof(detail_screen_t));
	detail->module_name = config->module_name;
	detail->display_name = config->display_name;
	detail->on_back_clicked = config->on_back_clicked;
	detail->on_view_clicked = config->on_view_clicked;
	detail->on_current_view_created = config->on_current_view_created;
	detail->on_gauges_created = config->on_gauges_created;
	detail->on_sensor_data_created = config->on_sensor_data_created;

	// callbacks set

	// Root overlay will be created on active screen

	// Store button configuration
	detail->setting_buttons_count = config->setting_buttons_count;
	if (config->setting_buttons_count > 0) {
		detail->setting_buttons = calloc(config->setting_buttons_count, sizeof(lv_obj_t*));
		if (!detail->setting_buttons) {
			printf("[E] detail_screen: Failed to allocate setting buttons array\n");
			free(detail);
			return NULL;
		}

		// Store button configurations
		detail->button_configs = calloc(config->setting_buttons_count, sizeof(detail_button_config_t));
		if (!detail->button_configs) {
			printf("[E] detail_screen: Failed to allocate button configs array\n");
			free(detail->setting_buttons);
			free(detail);
			return NULL;
		}

		// Copy button configurations
		for (int i = 0; i < config->setting_buttons_count; i++) {
			detail->button_configs[i] = config->setting_buttons[i];
		}
	}

	// Create root overlay container on the active screen
	lv_obj_t *scr = lv_scr_act();
	detail->root = lv_obj_create(scr);
	if (!detail->root) {
		printf("[E] detail_screen: Failed to create detail root container\n");
		free(detail);
		return NULL;
	}
	lv_obj_set_size(detail->root, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_bg_color(detail->root, PALETTE_BLACK, 0);
	lv_obj_set_style_bg_opa(detail->root, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_all(detail->root, 0, 0);
	lv_obj_set_style_border_width(detail->root, 0, 0);
	lv_obj_clear_flag(detail->root, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(detail->root, LV_OBJ_FLAG_OVERFLOW_VISIBLE); // Allow children to extend outside root
	// Hidden by default
	lv_obj_add_flag(detail->root, LV_OBJ_FLAG_HIDDEN);

	// Create main content container
	detail->main_content = lv_obj_create(detail->root);
	if (!detail->main_content) {
		printf("[E] detail_screen: Failed to create main_content\n");
		if (detail->root) lv_obj_del(detail->root);
		free(detail);
		return NULL;
	}
	lv_obj_set_size(detail->main_content, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_bg_color(detail->main_content, PALETTE_BLACK, 0);
	lv_obj_set_style_border_width(detail->main_content, 0, 0);
	lv_obj_set_style_pad_all(detail->main_content, 10, 0);
	lv_obj_clear_flag(detail->main_content, LV_OBJ_FLAG_SCROLLABLE);
	lv_obj_add_flag(detail->main_content, LV_OBJ_FLAG_OVERFLOW_VISIBLE); // Allow children to extend outside main content

	// Set up flexbox layout for main content
	lv_obj_set_flex_flow(detail->main_content, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(detail->main_content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

	// Layout calculations (match active display from port, fallback to 800x480)
	uint32_t disp_w = 800, disp_h = 480;
	lvgl_port_get_display_size(&disp_w, &disp_h);
	int screen_width = (int)disp_w;
	int screen_height = (int)disp_h;
	int padding = 10; // Increased padding
	int section_spacing = 15; // Increased spacing between sections
	int header_height = 30; // Shrunk header
	int content_height = screen_height - header_height;

	// Current view dimensions (same as home screen module)
	int current_view_width = 239;  // Same as home screen module
	int current_view_height = 189; // Same as home screen module

	// Calculate left column width from configuration
	int left_column_width = (screen_width * LEFT_COLUMN_WIDTH_PERCENT) / 100;

	// layout info

	// Debug: Check if main_content is valid
	if (!detail->main_content) {
		printf("[E] detail_screen: ERROR: main_content is NULL!\n");
		return NULL;
	}
		// main_content is valid

	// Create left column container for current view and raw values
	detail->left_column = lv_obj_create(detail->main_content);
	if (!detail->left_column) {
		printf("[E] detail_screen: Failed to create left_column\n");
		return NULL; // Return early if creation failed
	}

	lv_obj_set_size(detail->left_column, left_column_width, LV_PCT(100));
	lv_obj_set_style_flex_grow(detail->left_column, 0, 0); // Don't grow, use fixed width
	lv_obj_set_style_bg_color(detail->left_column, PALETTE_BLACK, 0);
	lv_obj_set_style_bg_opa(detail->left_column, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(detail->left_column, 0, 0);
	lv_obj_set_style_pad_all(detail->left_column, 0, 0);
	lv_obj_clear_flag(detail->left_column, LV_OBJ_FLAG_SCROLLABLE);

	// Set up flexbox for left column (vertical stack)
	lv_obj_set_flex_flow(detail->left_column, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(detail->left_column, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_gap(detail->left_column, 0, CONTAINER_GAP_PX); // Vertical gap between containers
	lv_obj_add_flag(detail->left_column, LV_OBJ_FLAG_OVERFLOW_VISIBLE); // Allow children to extend outside column

	// Create gauges section (right side, full height)
	if (config->show_gauges_section) {
		detail->gauges_container = lv_obj_create(detail->main_content);
		if (!detail->gauges_container) {
			printf("[E] detail_screen: Failed to create gauges_container\n");
		} else {
		lv_obj_set_size(detail->gauges_container, LV_PCT(100), LV_PCT(100));
		lv_obj_set_style_flex_grow(detail->gauges_container, 1, 0);
		lv_obj_set_style_pad_all(detail->gauges_container, 0, 0);
		lv_obj_set_style_bg_color(detail->gauges_container, PALETTE_BLACK, 0);
		lv_obj_set_style_border_width(detail->gauges_container, 0, 0); // No border around gauges container
		lv_obj_set_style_radius(detail->gauges_container, 0, 0);
		lv_obj_clear_flag(detail->gauges_container, LV_OBJ_FLAG_SCROLLABLE);

		// Set up flexbox for gauges (vertical stack with padding)
		lv_obj_set_flex_flow(detail->gauges_container, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(detail->gauges_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
		lv_obj_set_style_pad_gap(detail->gauges_container, 4, 0); // 4px padding between gauges
		// gauges container created

		// Note: The actual gauge initialization will be done by the module
		// when it calls the on_gauges_created callback
		// This container is ready for the gauges to be added
		}
	}

	// Create current view container in left column
	detail->current_view_container = lv_obj_create(detail->left_column);

	// Size and Style
	// Set up current view container styling (reusable function)
	detail_screen_restore_current_view_styling(detail->current_view_container);

	// Clickable Container with event callback
	lv_obj_add_flag(detail->current_view_container, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(detail->current_view_container, view_container_event_cb, LV_EVENT_PRESSED, detail);

	// RAW Values section
	detail->sensor_data_section = lv_obj_create(detail->left_column);

	// Size and Style
	lv_obj_set_size(detail->sensor_data_section, LV_PCT(100), LV_SIZE_CONTENT); // Content-based height
	lv_obj_set_style_flex_grow(detail->sensor_data_section, RAW_VALUES_GROW, 0); // Grow factor from config
	lv_obj_set_style_pad_all(detail->sensor_data_section, OTHER_SECTIONS_PADDING, 0); // Internal padding from config
	lv_obj_set_style_pad_top(detail->sensor_data_section, 16, 0); // Extra top padding for overlay title space
	lv_obj_set_style_bg_color(detail->sensor_data_section, PALETTE_BLACK, 0);
	lv_obj_set_style_border_width(detail->sensor_data_section, 1, 0); // Match current_view container
	lv_obj_set_style_border_color(detail->sensor_data_section, PALETTE_WHITE, 0);
	lv_obj_set_style_radius(detail->sensor_data_section, 4, 0);
	lv_obj_clear_flag(detail->sensor_data_section, LV_OBJ_FLAG_SCROLLABLE);

	// Flexbox Layout
	lv_obj_set_flex_flow(detail->sensor_data_section, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(detail->sensor_data_section, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_gap(detail->sensor_data_section, 2, 0); // Minimal gap between elements

	// Create overlay title for raw values section AFTER content is added
	lv_obj_t *sensor_title = lv_label_create(detail->root);
	lv_obj_set_style_text_font(sensor_title, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(sensor_title, PALETTE_WHITE, 0);
	lv_obj_set_style_bg_color(sensor_title, PALETTE_BLACK, 0); // Black background to obscure border
	lv_obj_set_style_bg_opa(sensor_title, LV_OPA_COVER, 0);
	lv_obj_set_style_pad_left(sensor_title, 8, 0);
	lv_obj_set_style_pad_right(sensor_title, 8, 0);
	lv_obj_set_style_pad_top(sensor_title, 2, 0);
	lv_obj_set_style_pad_bottom(sensor_title, 2, 0);
	lv_label_set_text(sensor_title, detail->display_name ? detail->display_name : "Raw Values");

	// Force layout update to ensure section has proper position and size
	lv_obj_update_layout(detail->left_column);

	// Create settings section (bottom left of screen, matching original)
	if (config->show_settings_button) {
		detail->settings_section = lv_obj_create(detail->left_column);
		if (!detail->settings_section) {
			printf("[E] detail_screen: Failed to create settings_section\n");
			return NULL; // Return early if creation failed
		}
		lv_obj_set_size(detail->settings_section, LV_PCT(100), LV_SIZE_CONTENT); // Content-based height
		lv_obj_set_style_flex_grow(detail->settings_section, SETTINGS_GROW, 0); // Grow factor from config
		lv_obj_set_style_pad_all(detail->settings_section, OTHER_SECTIONS_PADDING, 0); // Internal padding from config
		lv_obj_set_style_pad_top(detail->settings_section, 8, 0);
		lv_obj_set_style_bg_color(detail->settings_section, PALETTE_BLACK, 0);
		lv_obj_set_style_border_width(detail->settings_section, 1, 0); // Match current_view container
		lv_obj_set_style_border_color(detail->settings_section, PALETTE_WHITE, 0);
		lv_obj_add_flag(detail->settings_section, LV_OBJ_FLAG_OVERFLOW_VISIBLE); // Allow children to extend outside container
		lv_obj_set_style_radius(detail->settings_section, 4, 0);
		lv_obj_clear_flag(detail->settings_section, LV_OBJ_FLAG_SCROLLABLE);

		// Set up flexbox for settings section (vertical stack)
		lv_obj_set_flex_flow(detail->settings_section, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(detail->settings_section, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
		lv_obj_set_style_pad_gap(detail->settings_section, 0, 8); // 8px vertical gap between elements

		// Ensure settings section doesn't block touch events for its children
		lv_obj_add_flag(detail->settings_section, LV_OBJ_FLAG_CLICKABLE);

		// Ensure overlay doesn't intercept clicks intended for settings by keeping overlay's clip strictly to current_view
		lv_obj_move_foreground(detail->settings_section);

		// Create overlay title for settings section
		lv_obj_t *settings_title = lv_label_create(detail->root);
		lv_obj_set_style_text_font(settings_title, &lv_font_montserrat_14, 0);
		lv_obj_set_style_text_color(settings_title, PALETTE_WHITE, 0);
		lv_obj_set_style_bg_color(settings_title, PALETTE_BLACK, 0); // Black background to obscure border
		lv_obj_set_style_bg_opa(settings_title, LV_OPA_COVER, 0);
		lv_obj_set_style_pad_left(settings_title, 8, 0);
		lv_obj_set_style_pad_right(settings_title, 8, 0);
		lv_obj_set_style_pad_top(settings_title, 2, 0);
		lv_obj_set_style_pad_bottom(settings_title, 2, 0);
		lv_label_set_text(settings_title, "SETTINGS");

		// Force layout update to ensure section has proper position and size
		lv_obj_update_layout(detail->left_column);

		// Position Title for Settings section inline with the section's top border
		lv_obj_align_to(settings_title, detail->settings_section, LV_ALIGN_OUT_TOP_LEFT, 20, 10);
		printf("[I] detail_screen: Created settings title inline with section border\n");

		// Get the settings section dimensions for proper button sizing
		lv_coord_t settings_width = lv_obj_get_width(detail->settings_section);
		lv_coord_t settings_height = lv_obj_get_height(detail->settings_section);
		printf("[I] detail_screen: Settings section size: %dx%d\n", settings_width, settings_height);

		// If settings section height is 0, use a default value
		if (settings_height <= 0) {
			settings_height = 100; // Default height for settings section
			printf("[W] detail_screen: Settings section height is 0, using default: %d\n", settings_height);
		}

		// Create main button container with flexbox layout
		lv_obj_t *main_button_container = lv_obj_create(detail->settings_section);
		lv_obj_set_size(main_button_container, LV_PCT(100), LV_PCT(100));
		lv_obj_set_style_pad_all(main_button_container, 8, 0);
		lv_obj_set_style_bg_color(main_button_container, PALETTE_BLACK, 0);
		lv_obj_set_style_bg_opa(main_button_container, LV_OPA_COVER, 0);
		lv_obj_set_style_border_width(main_button_container, 0, 0);
		lv_obj_clear_flag(main_button_container, LV_OBJ_FLAG_SCROLLABLE);
		lv_obj_set_style_pad_gap(main_button_container, 8, 0);


		// Set up flexbox layout: BACK button on left, other buttons on right
		lv_obj_set_flex_flow(main_button_container, LV_FLEX_FLOW_ROW);
		lv_obj_set_flex_align(main_button_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		// BACK button Left side
		printf("[I] detail_screen: Creating BACK button\n");
		detail->back_button = lv_btn_create(main_button_container);
		lv_obj_set_size(detail->back_button, LV_PCT(49), LV_PCT(96));

		// Style BACK button (green)
		lv_obj_set_style_border_width(detail->back_button, 2, 0);
		lv_obj_set_style_border_color(detail->back_button, PALETTE_GREEN, 0);
		lv_obj_set_style_bg_color(detail->back_button, PALETTE_BLACK, 0);
		lv_obj_set_style_bg_color(detail->back_button, PALETTE_GREEN, LV_STATE_PRESSED);
		lv_obj_set_style_text_color(detail->back_button, PALETTE_GREEN, LV_PART_MAIN | LV_STATE_DEFAULT );
		lv_obj_set_style_text_color(detail->back_button, PALETTE_BLACK, LV_PART_MAIN | LV_STATE_PRESSED );
		lv_obj_set_style_radius(detail->back_button, 8, 0);
		lv_obj_set_style_shadow_width(detail->back_button, 0, 0); // Remove drop shadow

		// Ensure button is clickable
		lv_obj_add_flag(detail->back_button, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(detail->back_button, LV_OBJ_FLAG_SCROLLABLE);

		lv_obj_add_event_cb(detail->back_button, back_button_event_cb, LV_EVENT_ALL, detail);
		printf("[I] detail_screen: Added event callback for BACK button\n");

		lv_obj_t *back_label = lv_label_create(detail->back_button);
		lv_label_set_text(back_label, "BACK");
		lv_obj_center(back_label);

		// Right side container for ALERTS and TIMELINE buttons
		lv_obj_t* right_buttons_container = lv_obj_create(main_button_container);
		lv_obj_set_size(right_buttons_container, LV_PCT(49), LV_PCT(100));
		lv_obj_set_style_bg_color(right_buttons_container, PALETTE_BLACK, 0);
		lv_obj_set_style_bg_opa(right_buttons_container, LV_OPA_COVER, 0);
		lv_obj_set_style_border_width(right_buttons_container, 0, 0);
		lv_obj_add_flag(right_buttons_container, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
		lv_obj_set_style_pad_all(right_buttons_container, 2, 0);
		lv_obj_set_style_pad_gap(right_buttons_container, 8, 0);

		// Set up vertical flexbox for right buttons
		lv_obj_set_flex_flow(right_buttons_container, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(right_buttons_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

		// ALERTS, TIMELINE Buttons
		for (int i = 0; i < detail->setting_buttons_count; i++) {

			detail->setting_buttons[i] = lv_btn_create(right_buttons_container);
			lv_obj_set_size(detail->setting_buttons[i], LV_PCT(100), LV_PCT(47));

			// Determine colors based on button text
			lv_color_t border_color, text_color, pressed_bg_color, pressed_text_color;
			if (strcmp(config->setting_buttons[i].text, "ALERTS") == 0) {

				border_color = PALETTE_YELLOW; // Yellow
				text_color = PALETTE_YELLOW;
				pressed_bg_color = PALETTE_YELLOW;
				pressed_text_color = PALETTE_BLACK;
			} else if (strcmp(config->setting_buttons[i].text, "TIMELINE") == 0) {

				border_color = PALETTE_CYAN; // Cyan
				text_color = PALETTE_CYAN;
				pressed_bg_color = PALETTE_CYAN;
				pressed_text_color = PALETTE_BLACK;
			} else {

				border_color = PALETTE_WHITE; // Default white
				text_color = PALETTE_WHITE;
				pressed_bg_color = PALETTE_WHITE;
				pressed_text_color = PALETTE_BLACK;
			}

			// Style button
			lv_obj_set_style_bg_color(detail->setting_buttons[i], PALETTE_BLACK, 0);
			lv_obj_set_style_bg_color(detail->setting_buttons[i], pressed_bg_color, LV_PART_MAIN | LV_STATE_PRESSED );
			lv_obj_set_style_text_color( detail->setting_buttons[i], text_color, LV_PART_MAIN | LV_STATE_DEFAULT );
			lv_obj_set_style_text_color( detail->setting_buttons[i], pressed_text_color, LV_PART_MAIN | LV_STATE_PRESSED );

			lv_obj_set_style_border_width(detail->setting_buttons[i], 2, 0);
			lv_obj_set_style_border_color(detail->setting_buttons[i], border_color, 0);
			lv_obj_set_style_radius(detail->setting_buttons[i], 8, 0);
			lv_obj_set_style_shadow_width(detail->setting_buttons[i], 0, 0); // Remove drop shadow

			// Ensure button is clickable
			lv_obj_add_flag(detail->setting_buttons[i], LV_OBJ_FLAG_CLICKABLE);
			lv_obj_clear_flag(detail->setting_buttons[i], LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_add_event_cb(detail->setting_buttons[i], setting_button_event_cb, LV_EVENT_ALL, detail);

			// Store button index in user data
			lv_obj_set_user_data(detail->setting_buttons[i], (void*)(intptr_t)i);

			// Create label
			lv_obj_t *label = lv_label_create( detail->setting_buttons[ i ] );
			lv_label_set_text( label, config->setting_buttons[ i ].text );

			lv_obj_center( label );
		}
	}

	// Position RAW VALUES overlay title inline with the settings section's top border
	lv_obj_align_to(sensor_title, detail->sensor_data_section, LV_ALIGN_OUT_TOP_LEFT, 20, 10);
	printf("[I] detail_screen: Created sensor title inline with section border\n");

	// Create status container if requested
	if (config->show_status_indicators) {
				int status_y = current_view_height + section_spacing + 200 + section_spacing; // Approximate raw values height
		if (config->show_settings_button) {
			status_y += 100 + 10; // Approximate settings height + padding
		}

		detail->status_container = lv_obj_create(detail->main_content);
		lv_obj_set_size(detail->status_container, screen_width - 20, 100);
		lv_obj_align(detail->status_container, LV_ALIGN_TOP_MID, 0, status_y);
		lv_obj_set_style_pad_all(detail->status_container, 10, 0);
		lv_obj_set_style_bg_color(detail->status_container, lv_color_hex(0x0A0A0A), 0);
		lv_obj_set_style_border_width(detail->status_container, 2, 0);
		lv_obj_set_style_border_color(detail->status_container, PALETTE_WHITE, 0);
		lv_obj_set_style_radius(detail->status_container, 4, 0);
		lv_obj_clear_flag(detail->status_container, LV_OBJ_FLAG_SCROLLABLE);
	}

	// Overlay functionality not implemented in Pi port
	detail->current_view_overlay = NULL;

		printf("[I] detail_screen: Detail screen created for module: %s\n", config->module_name);
	return detail;
}

void detail_screen_show(detail_screen_t* detail)
{
	if (!detail) return;

	// Show overlay root on top
	if (detail->root && lv_obj_is_valid(detail->root)) {
		lv_obj_clear_flag(detail->root, LV_OBJ_FLAG_HIDDEN);
		lv_obj_move_foreground(detail->root);
	} else {
		printf("[E] detail_screen: Detail root container is NULL or invalid for module: %s\n", detail->module_name);
		return;
	}

	// Create content in all containers using generic callbacks
	// Create current view content directly in the current view container
	if (detail->current_view_container) {
		int child_count = lv_obj_get_child_cnt(detail->current_view_container);
		// current view container child count

		// Prepare layout using reusable function
		if (!detail_screen_prepare_current_view_layout(detail)) {
			// failed to prepare layout
			return;
		}

		// Call the generic callback to create current view content
		if (detail->on_current_view_created) {
			detail->on_current_view_created(detail->current_view_container);
		}
	}

	// Create gauges content in the gauges container
	if (detail->gauges_container) {
		int gauges_child_count = lv_obj_get_child_cnt(detail->gauges_container);
		if (gauges_child_count == 0) {
			// creating gauges content
			// Call the generic callback to create gauges content
			if (detail->on_gauges_created) {
				detail->on_gauges_created(detail->gauges_container);
			}
		} else {
			// skipping gauges content creation
		}
	}

	// Create sensor data content in the sensor data section
	if (detail->sensor_data_section) {
		if (lv_obj_get_child_cnt(detail->sensor_data_section) == 0) {
			// Call the generic callback to create sensor data content
			if (detail->on_sensor_data_created) {
				detail->on_sensor_data_created(detail->sensor_data_section);
			}
		}
	}

				// detail screen shown
}

void detail_screen_hide(detail_screen_t* detail)
{
	if (!detail) return;

	// Hide overlay root
	if (detail->root && lv_obj_is_valid(detail->root)) {
		lv_obj_add_flag(detail->root, LV_OBJ_FLAG_HIDDEN);
		printf("[I] detail_screen: Detail screen hidden for module: %s\n", detail->module_name);
	} else {
		printf("[W] detail_screen: Detail root container is NULL or invalid for module: %s\n", detail->module_name);
	}
}

void detail_screen_update(detail_screen_t* detail, const void* data)
{
	if (!detail) return;

	// Update title if needed
	// Update status indicators if they exist
	// This can be extended based on specific module needs

		printf("[D] detail_screen: Detail screen updated for module: %s\n", detail->module_name);
}

void detail_screen_destroy(detail_screen_t* detail)
{
	if (!detail) return;


	// Free dynamic buttons array
	if (detail->setting_buttons) {
		free(detail->setting_buttons);
	}

	// Free button configs array
	if (detail->button_configs) {
		free(detail->button_configs);
	}

	// Sensor labels cleanup removed - modules handle their own cleanup

	// Destroy root (this will destroy all children)
	if (detail->root) {
		lv_obj_del(detail->root);
	}

	free(detail);
	printf("[I] detail_screen: Detail screen destroyed\n");
}

lv_obj_t* detail_screen_get_current_view_container(detail_screen_t* detail)
{
	if (!detail) return NULL;
	return detail->current_view_container;
}

lv_obj_t* detail_screen_get_gauges_container(detail_screen_t* detail)
{
	if (!detail) return NULL;
	return detail->gauges_container;
}

lv_obj_t* detail_screen_get_status_container(detail_screen_t* detail)
{
	if (!detail) return NULL;
	return detail->status_container;
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

/**
 * @brief Restore current view container styling after clean operation
 * @param container The current view container to restore styling for
 */
void detail_screen_restore_current_view_styling(lv_obj_t* container)
{
	if (!container) {
		printf("[E] detail_screen: Container is NULL in restore styling\n");
		return;
	}

	// Log container dimensions BEFORE styling
	lv_coord_t width_before = lv_obj_get_width(container);
	lv_coord_t height_before = lv_obj_get_height(container);
	printf("[D] detail_screen: Container BEFORE styling: %dx%d\n", width_before, height_before);

	// Restore size and layout properties
	lv_obj_set_size(container, LV_PCT(100), LV_SIZE_CONTENT); // Content-based height
	lv_obj_set_style_flex_grow(container, CURRENT_VIEW_GROW, 0); // Grow factor from config
	lv_obj_set_style_pad_all(container, 2, 0); // Internal padding from config

	// Ensure this container does NOT manage child layout (so child alignment works)
	lv_obj_set_layout(container, LV_LAYOUT_NONE);

	// CRITICAL: Reset flex flow to 0 (no flex flow) to match initial creation
	// lv_obj_clean() sets flex flow to 1 (column), but we need 0 (none)
	lv_obj_set_style_flex_flow(container, 0, 0);

	// Restore visual styling
	lv_obj_set_style_bg_color(container, PALETTE_BLACK, 0);
	lv_obj_set_style_border_width(container, 1, 0); // Match view container style
	lv_obj_set_style_border_color(container, PALETTE_WHITE, 0);
	lv_obj_set_style_radius(container, 4, 0);
	lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);

	// Log container dimensions AFTER styling
	lv_coord_t width_after = lv_obj_get_width(container);
	lv_coord_t height_after = lv_obj_get_height(container);
	printf("[D] detail_screen: Container AFTER styling: %dx%d\n", width_after, height_after);

	printf("[I] detail_screen: Current view container styling restored\n");
}

/**
 * @brief Prepare current view container layout for content creation
 * @param detail Detail screen instance
 * @return true if layout preparation was successful, false otherwise
 */
bool detail_screen_prepare_current_view_layout(detail_screen_t* detail)
{
	if (!detail || !detail->current_view_container || !detail->left_column) {
		printf("[E] detail_screen: Invalid detail screen or containers in layout preparation\n");
		return false;
	}

	// Log container dimensions BEFORE layout update
	lv_coord_t width_before_layout = lv_obj_get_width(detail->current_view_container);
	lv_coord_t height_before_layout = lv_obj_get_height(detail->current_view_container);
	printf("[D] detail_screen: Container BEFORE layout update: %dx%d\n", width_before_layout, height_before_layout);

	// Force layout calculation on parent container (left_column) to ensure flexbox sizing
	lv_obj_update_layout(detail->left_column);
	lv_coord_t width_after_layout = lv_obj_get_width(detail->current_view_container);
	lv_coord_t height_after_layout = lv_obj_get_height(detail->current_view_container);
	printf("[D] detail_screen: Container AFTER layout update: %dx%d\n", width_after_layout, height_after_layout);

	// Additional check: if size is still too small, force a second layout update
	if (width_after_layout < 200 || height_after_layout < 150) {
		printf("[W] detail_screen: Container size seems too small (%dx%d), forcing additional layout update\n", width_after_layout, height_after_layout);
		lv_obj_update_layout(lv_scr_act()); // Update entire screen layout
		lv_obj_update_layout(detail->left_column); // Update parent again
		width_after_layout = lv_obj_get_width(detail->current_view_container);
		height_after_layout = lv_obj_get_height(detail->current_view_container);
		printf("[I] detail_screen: Container size after additional layout update: %dx%d\n", width_after_layout, height_after_layout);
	}

	// Validate final dimensions
	if (width_after_layout < 100 || height_after_layout < 50) {
		printf("[E] detail_screen: Container dimensions too small after layout preparation: %dx%d\n", width_after_layout, height_after_layout);
		return false;
	}

	printf("[I] detail_screen: Layout preparation completed successfully: %dx%d\n", width_after_layout, height_after_layout);
	return true;
}

// ============================================================================
// SENSOR LABELS MANAGEMENT - REMOVED
// ============================================================================
// Sensor-specific functionality has been removed to make detail_screen generic.
// Modules should implement their own sensor data display using the callbacks.
