#include <stdio.h>
#include "detail_screen.h"

#include "../../lvgl_port_pi.h"
#include "../../displayModules/power-monitor/power-monitor.h"
#include "../../data/lerp_data/lerp_data.h"
#include "../../state/device_state.h"
#include "../../fonts/lv_font_noplato_24.h"
#include <stdlib.h>
#include <string.h>

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
#define CURRENT_VIEW_PADDING    2     // Internal padding for current view
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
	printf("[I] detail_screen: BACK button event callback triggered\n");
	detail_screen_t *detail = (detail_screen_t*)lv_event_get_user_data(e);
	if (detail && detail->on_back_clicked) {
		printf("[I] detail_screen: Calling back button handler\n");
		detail->on_back_clicked();
	} else {
		printf("[W] detail_screen: Back button handler not available\n");
	}
}

static void setting_button_event_cb(lv_event_t *e)
{
	printf("[I] detail_screen: Setting button event callback triggered\n");
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

static void view_container_event_cb(lv_event_t *e)
{
	printf("[I] detail_screen: Current view container clicked - cycling view\n");
	detail_screen_t *detail = (detail_screen_t*)lv_event_get_user_data(e);
	if (!detail) return;

	// Call the power monitor's view cycling function
	extern void power_monitor_cycle_current_view(void);
	power_monitor_cycle_current_view();

	// Also call the original callback if it exists
	if (detail->on_view_clicked) {
		detail->on_view_clicked();
	}
}

detail_screen_t* detail_screen_create(const detail_screen_config_t* config)
{
	if (!config || !config->module_name || !config->display_name) {
		printf("[E] detail_screen: Invalid configuration for detail screen\n");
		return NULL;
	}

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
	lv_obj_set_style_bg_color(detail->root, lv_color_hex(0x000000), 0);
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
	lv_obj_set_style_bg_color(detail->main_content, lv_color_hex(0x000000), 0);
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

	printf("[I] detail_screen: Using flexbox layout: screen_height=%d, content_height=%d, left_column_width=%d, current_view=%dx%d\n",
		screen_height, content_height, left_column_width, current_view_width, current_view_height);

	// Debug: Check if main_content is valid
	if (!detail->main_content) {
		printf("[E] detail_screen: ERROR: main_content is NULL!\n");
		return NULL;
	}
		printf("[I] detail_screen: main_content is valid: %p\n", detail->main_content);

	// Create left column container for current view and raw values
	detail->left_column = lv_obj_create(detail->main_content);
	if (!detail->left_column) {
		printf("[E] detail_screen: Failed to create left_column\n");
		return NULL; // Return early if creation failed
	}

	lv_obj_set_size(detail->left_column, left_column_width, LV_PCT(100));
	lv_obj_set_style_flex_grow(detail->left_column, 0, 0); // Don't grow, use fixed width
	lv_obj_set_style_bg_opa(detail->left_column, LV_OPA_TRANSP, 0);
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
		lv_obj_set_style_bg_color(detail->gauges_container, lv_color_hex(0x000000), 0);
		lv_obj_set_style_border_width(detail->gauges_container, 0, 0); // No border around gauges container
		lv_obj_set_style_radius(detail->gauges_container, 0, 0);
		lv_obj_clear_flag(detail->gauges_container, LV_OBJ_FLAG_SCROLLABLE);

		// Set up flexbox for gauges (vertical stack with padding)
		lv_obj_set_flex_flow(detail->gauges_container, LV_FLEX_FLOW_COLUMN);
		lv_obj_set_flex_align(detail->gauges_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
		lv_obj_set_style_pad_gap(detail->gauges_container, 4, 0); // 4px padding between gauges
		printf("[I] detail_screen: Gauges container ID: %p\n", detail->gauges_container);
	printf("[I] detail_screen: *** GAUGES CONTAINER CREATED: %p with flexbox layout ***\n", detail->gauges_container);

		// Note: The actual gauge initialization will be done by the power-monitor module
		// when it calls power_monitor_create_current_view_in_container()
		// This container is ready for the gauges to be added
		}
	}

	// Create current view container in left column
	detail->current_view_container = lv_obj_create(detail->left_column);
	if (!detail->current_view_container) {
		printf("[E] detail_screen: Failed to create current_view_container\n");
		return NULL; // Return early if creation failed
	}

	lv_obj_add_flag(detail->current_view_container, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_add_event_cb(detail->current_view_container, view_container_event_cb, LV_EVENT_CLICKED, detail);
	lv_obj_set_size(detail->current_view_container, LV_PCT(100), LV_SIZE_CONTENT); // Content-based height
	lv_obj_set_style_flex_grow(detail->current_view_container, CURRENT_VIEW_GROW, 0); // Grow factor from config

	// Debug: Log initial container size
		printf("[I] detail_screen: Initial current_view_container size: %dx%d (before content)\n",
		lv_obj_get_width(detail->current_view_container), lv_obj_get_height(detail->current_view_container));
	lv_obj_set_style_pad_all(detail->current_view_container, CURRENT_VIEW_PADDING, 0); // Internal padding from config
	lv_obj_set_style_bg_color(detail->current_view_container, lv_color_hex(0x000000), 0);
	lv_obj_set_style_border_width(detail->current_view_container, 1, 0); // Match view container style
	lv_obj_set_style_border_color(detail->current_view_container, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_radius(detail->current_view_container, 4, 0);
	lv_obj_clear_flag(detail->current_view_container, LV_OBJ_FLAG_SCROLLABLE);

	// Create raw values section (below current view, left side, matching original)

	detail->sensor_data_section = lv_obj_create(detail->left_column);
	if (!detail->sensor_data_section) {
		printf("[E] detail_screen: Failed to create sensor_data_section\n");
		return NULL; // Return early if creation failed
	}

	lv_obj_set_size(detail->sensor_data_section, LV_PCT(100), LV_SIZE_CONTENT); // Content-based height
	lv_obj_set_style_flex_grow(detail->sensor_data_section, RAW_VALUES_GROW, 0); // Grow factor from config
	lv_obj_set_style_pad_all(detail->sensor_data_section, OTHER_SECTIONS_PADDING, 0); // Internal padding from config
	lv_obj_set_style_pad_top(detail->sensor_data_section, 16, 0); // Extra top padding for overlay title space
	lv_obj_set_style_bg_color(detail->sensor_data_section, lv_color_hex(0x000000), 0);
	lv_obj_set_style_border_width(detail->sensor_data_section, 1, 0); // Match current_view container
	lv_obj_set_style_border_color(detail->sensor_data_section, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_radius(detail->sensor_data_section, 4, 0);
	lv_obj_clear_flag(detail->sensor_data_section, LV_OBJ_FLAG_SCROLLABLE);

	// Set up flexbox for sensor data section to optimize space usage
	lv_obj_set_flex_flow(detail->sensor_data_section, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(detail->sensor_data_section, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_gap(detail->sensor_data_section, 2, 0); // Minimal gap between elements

	// Create sensor data labels (owned by detail screen)
	detail_screen_create_sensor_labels(detail);

	// Create overlay title for raw values section AFTER content is added (matching POWER MONITOR style)
	lv_obj_t *sensor_title = lv_label_create(detail->root);
	lv_obj_set_style_text_font(sensor_title, &lv_font_montserrat_14, 0);
	lv_obj_set_style_text_color(sensor_title, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_bg_color(sensor_title, lv_color_hex(0x000000), 0); // Black background to obscure border
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
		lv_obj_set_style_pad_top(detail->settings_section, 16, 0); // Extra top padding for overlay title space
		lv_obj_set_style_bg_color(detail->settings_section, lv_color_hex(0x000000), 0);
		lv_obj_set_style_border_width(detail->settings_section, 1, 0); // Match current_view container
		lv_obj_set_style_border_color(detail->settings_section, lv_color_hex(0xFFFFFF), 0);
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

		// Create overlay title for settings section (matching POWER MONITOR style)
		lv_obj_t *settings_title = lv_label_create(detail->root);
		lv_obj_set_style_text_font(settings_title, &lv_font_montserrat_14, 0);
		lv_obj_set_style_text_color(settings_title, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_bg_color(settings_title, lv_color_hex(0x000000), 0); // Black background to obscure border
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

		// Create container for settings buttons
		lv_obj_t *buttons_container = lv_obj_create(detail->settings_section);
		lv_obj_set_size(buttons_container, LV_PCT(100), LV_SIZE_CONTENT);
		lv_obj_set_style_bg_opa(buttons_container, LV_OPA_TRANSP, 0);
		lv_obj_set_style_border_width(buttons_container, 0, 0);
		lv_obj_set_style_pad_all(buttons_container, 0, 0);
		lv_obj_clear_flag(buttons_container, LV_OBJ_FLAG_SCROLLABLE);

		// Set up flexbox for buttons (2 columns)
		lv_obj_set_flex_flow(buttons_container, LV_FLEX_FLOW_ROW_WRAP);
		lv_obj_set_flex_align(buttons_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
		lv_obj_set_style_pad_gap(buttons_container, 8, 8); // 8px gap between buttons

		// Dynamic settings buttons
		int button_height = 40; // larger settings buttons

		// Create dynamic buttons
		printf("[I] detail_screen: Creating %d dynamic buttons\n", detail->setting_buttons_count);
		for (int i = 0; i < detail->setting_buttons_count; i++) {
			detail->setting_buttons[i] = lv_btn_create(buttons_container);
			lv_obj_set_size(detail->setting_buttons[i], LV_PCT(48), button_height); // 48% width for 2 columns with gap
			printf("[I] detail_screen: Created button %d: %s\n", i, config->setting_buttons[i].text);
			lv_obj_set_style_bg_color(detail->setting_buttons[i], lv_color_hex(0x1a1a1a), 0); // Almost-black gray
			lv_obj_set_style_border_width(detail->setting_buttons[i], 1, 0);
			lv_obj_set_style_border_color(detail->setting_buttons[i], lv_color_hex(0xFFFFFF), 0);
			lv_obj_set_style_radius(detail->setting_buttons[i], 4, 0);

			// Ensure button is clickable
			lv_obj_add_flag(detail->setting_buttons[i], LV_OBJ_FLAG_CLICKABLE);
			lv_obj_clear_flag(detail->setting_buttons[i], LV_OBJ_FLAG_SCROLLABLE);

			lv_obj_add_event_cb(detail->setting_buttons[i], setting_button_event_cb, LV_EVENT_CLICKED, detail);
			printf("[I] detail_screen: Added event callback for button %d\n", i);

			// Store button index in user data
			lv_obj_set_user_data(detail->setting_buttons[i], (void*)(intptr_t)i);
			printf("[I] detail_screen: Set user data for button %d: index=%d\n", i, i);

			// Create label
			lv_obj_t *label = lv_label_create(detail->setting_buttons[i]);
			lv_label_set_text(label, config->setting_buttons[i].text);
			lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
			lv_obj_center(label);
		}

		// BACK button (full width, fills remaining space with proper padding) - created outside flexbox container
		printf("[I] detail_screen: Creating BACK button\n");
		detail->back_button = lv_btn_create(detail->settings_section);

		lv_obj_set_size(detail->back_button, LV_PCT(100), LV_PCT(100)); // Full width and height
		lv_obj_set_style_pad_all(detail->back_button, 0, 0); // No internal padding
		lv_obj_align(detail->back_button, LV_ALIGN_BOTTOM_MID, 0, -OTHER_SECTIONS_PADDING); // Align to bottom with padding
		lv_obj_set_style_bg_color(detail->back_button, lv_color_hex(0x1a1a1a), 0); // Almost-black gray
		lv_obj_set_style_border_width(detail->back_button, 1, 0);
		lv_obj_set_style_border_color(detail->back_button, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_radius(detail->back_button, 4, 0);

		// Ensure button is clickable
		lv_obj_add_flag(detail->back_button, LV_OBJ_FLAG_CLICKABLE);
		lv_obj_clear_flag(detail->back_button, LV_OBJ_FLAG_SCROLLABLE);

		lv_obj_add_event_cb(detail->back_button, back_button_event_cb, LV_EVENT_CLICKED, detail);
		printf("[I] detail_screen: Added event callback for BACK button\n");

		lv_obj_t *back_label = lv_label_create(detail->back_button);
		lv_label_set_text(back_label, "BACK");
		lv_obj_set_style_text_color(back_label, lv_color_hex(0xFFFFFF), 0);
		lv_obj_set_style_text_align(back_label, LV_TEXT_ALIGN_CENTER, 0); // Center text alignment
		lv_obj_center(back_label); // Center the label object itself
	}

	// Position RAW VALUES overlay title inline with the settings section's top border (like POWER MONITOR)
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
		lv_obj_set_style_border_color(detail->status_container, lv_color_hex(0xFFFFFF), 0);
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

	// Create content in all containers
	// Call the module-specific function to create the content for each section
	extern void power_monitor_create_current_view_in_container(lv_obj_t* container);
	extern void power_monitor_create_current_view_content(lv_obj_t* container);

	// Create current view content directly in the current view container
	if (detail->current_view_container) {
		int child_count = lv_obj_get_child_cnt(detail->current_view_container);
		printf("[I] detail_screen: Current view container child count: %d\n", child_count);

		// Always refresh current view content to ensure it shows updated data when cycling views
		// The power monitor function handles its own cleanup
		printf("[I] detail_screen: Creating/refreshing current view content (always update for view cycling)\n");

		// Force layout calculation on parent container (left_column) to ensure flexbox sizing
		lv_obj_update_layout(detail->left_column);
		lv_coord_t width_after_layout = lv_obj_get_width(detail->current_view_container);
		lv_coord_t height_after_layout = lv_obj_get_height(detail->current_view_container);
		printf("[I] detail_screen: Current_view_container size after parent layout update: %dx%d\n", width_after_layout, height_after_layout);

		// Additional check: if size is still too small, force a second layout update
		if (width_after_layout < 200 || height_after_layout < 150) {
			printf("[W] detail_screen: Container size seems too small (%dx%d), forcing additional layout update\n", width_after_layout, height_after_layout);
			lv_obj_update_layout(lv_scr_act()); // Update entire screen layout
			lv_obj_update_layout(detail->left_column); // Update parent again
			width_after_layout = lv_obj_get_width(detail->current_view_container);
			height_after_layout = lv_obj_get_height(detail->current_view_container);
			printf("[I] detail_screen: Container size after additional layout update: %dx%d\n", width_after_layout, height_after_layout);
		}

		// Register the detail container and use the new simple current view rendering
		power_monitor_show_in_container_detail(detail->current_view_container);
	}

	// Create gauges content in the gauges container
	if (detail->gauges_container) {
		int gauges_child_count = lv_obj_get_child_cnt(detail->gauges_container);
		printf("[I] detail_screen: Gauges container child count: %d\n", gauges_child_count);
		if (gauges_child_count == 0) {
			printf("[I] detail_screen: Creating gauges content (container empty)\n");
			power_monitor_create_current_view_in_container(detail->gauges_container);
		} else {
			printf("[I] detail_screen: Skipping gauges content creation (container already populated)\n");
		}
	}

	// Create sensor data content in the sensor data section
	if (detail->sensor_data_section) {
		if (lv_obj_get_child_cnt(detail->sensor_data_section) == 0) {
			power_monitor_create_current_view_in_container(detail->sensor_data_section);
		}
	}

		printf("[I] detail_screen: Detail screen shown for module: %s\n", detail->module_name);
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

void detail_screen_update(detail_screen_t* detail, const power_monitor_data_t* data)
{
	if (!detail || !data) return;

	// Update title if needed
	// Update status indicators if they exist
	// This can be extended based on specific module needs

		printf("[D] detail_screen: Detail screen updated for module: %s\n", detail->module_name);
}

void detail_screen_destroy(detail_screen_t* detail)
{
	if (!detail) return;

	// Destroy overlay
	// Overlay functionality not implemented in Pi port

	// Free dynamic buttons array
	if (detail->setting_buttons) {
		free(detail->setting_buttons);
	}

	// Free button configs array
	if (detail->button_configs) {
		free(detail->button_configs);
	}

	// Clean up sensor labels (they will be destroyed with the root, just clear the flag)
	detail->sensor_labels_created = false;
	for (int i = 0; i < 15; i++) {
		detail->sensor_labels[i] = NULL;
	}

	// Destroy root (this will destroy all children)
	if (detail->root) {
		lv_obj_del_async(detail->root);
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
// SENSOR LABELS MANAGEMENT
// ============================================================================

void detail_screen_create_sensor_labels(detail_screen_t* detail)
{
	if (!detail || !detail->sensor_data_section) {
		printf("[E] detail_screen: Invalid detail screen or sensor data section\n");
		return;
	}

	// Check if labels are already created
	if (detail->sensor_labels_created) {
		printf("[W] detail_screen: Sensor data labels already created, skipping\n");
		return;
	}

	printf("[I] detail_screen: *** CREATING SENSOR DATA LABELS ***\n");
	printf("[I] detail_screen: Container: %p, size: %dx%d\n", detail->sensor_data_section,
		lv_obj_get_width(detail->sensor_data_section), lv_obj_get_height(detail->sensor_data_section));

	// Initialize all sensor labels to NULL
	for (int i = 0; i < 15; i++) {
		detail->sensor_labels[i] = NULL;
	}

	// Create sensor data labels with horizontal layout (label: value on same line)
	lv_color_t labelColor = lv_color_hex(0x00bbe6);
	lv_color_t valueColor = lv_color_hex(0x39ab00);
	lv_color_t groupColor = lv_color_hex(0xFFFFFF);

	// Sensor data layout: Group headers + horizontal label:value pairs
	// Index mapping: 0=Starter Header, 1=Starter Volts, 2=Starter Volts Value, 3=Starter Amps, 4=Starter Amps Value
	//                5=House Header, 6=House Volts, 7=House Volts Value, 8=House Amps, 9=House Amps Value
	//                10=Solar Header, 11=Solar Volts, 12=Solar Volts Value, 13=Solar Amps, 14=Solar Amps Value

	const char* group_names[] = {"Starter Battery", "House Battery", "Solar Input"};
	const char* value_labels[] = {"Volts:", "Amperes:"};

	int label_index = 0;

	// Create 3 groups (Starter, House, Solar)
	for (int group = 0; group < 3; group++) {
		// Group header
		detail->sensor_labels[label_index] = lv_label_create(detail->sensor_data_section);
		lv_obj_set_style_text_font(detail->sensor_labels[label_index], &lv_font_montserrat_16, 0);
		lv_obj_set_style_text_color(detail->sensor_labels[label_index], groupColor, 0);
		lv_label_set_text(detail->sensor_labels[label_index], group_names[group]);
		lv_obj_set_style_pad_top(detail->sensor_labels[label_index], group == 0 ? 5 : 10, 0); // Extra spacing between groups
		printf("[D] detail_screen: Created static label %d: %s\n", label_index, group_names[group]);
		label_index++;

		// Create 2 value pairs (Volts, Amperes) for each group
		for (int value_type = 0; value_type < 2; value_type++) {
			// Create horizontal container for label:value pair
			lv_obj_t* value_row = lv_obj_create(detail->sensor_data_section);
			lv_obj_set_size(value_row, LV_PCT(100), LV_SIZE_CONTENT);
			lv_obj_set_style_bg_opa(value_row, LV_OPA_TRANSP, 0);
			lv_obj_set_style_border_width(value_row, 0, 0);
			lv_obj_set_style_pad_all(value_row, 2, 0);
			lv_obj_clear_flag(value_row, LV_OBJ_FLAG_SCROLLABLE);

			// Set up horizontal flexbox for label:value layout
			lv_obj_set_flex_flow(value_row, LV_FLEX_FLOW_ROW);
			lv_obj_set_flex_align(value_row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

			// Label (left side)
			detail->sensor_labels[label_index] = lv_label_create(value_row);
			lv_obj_set_style_text_font(detail->sensor_labels[label_index], &lv_font_montserrat_14, 0);
			lv_obj_set_style_text_color(detail->sensor_labels[label_index], labelColor, 0);
			lv_label_set_text(detail->sensor_labels[label_index], value_labels[value_type]);
			printf("[D] detail_screen: Created static label %d: %s\n", label_index, value_labels[value_type]);
			label_index++;

			// Value (right side)
			detail->sensor_labels[label_index] = lv_label_create(value_row);
			lv_obj_set_style_text_font(detail->sensor_labels[label_index], &lv_font_noplato_24, 0);
			lv_obj_set_style_text_color(detail->sensor_labels[label_index], valueColor, 0);
			lv_obj_set_style_text_align(detail->sensor_labels[label_index], LV_TEXT_ALIGN_RIGHT, 0);
			lv_label_set_text(detail->sensor_labels[label_index], "0.0");
			printf("[D] detail_screen: Created dynamic label %d: 0.0\n", label_index);
			label_index++;
		}
	}

	// Mark as created
	detail->sensor_labels_created = true;
	printf("[I] detail_screen: Sensor data labels created successfully\n");
}

void detail_screen_update_sensor_labels(detail_screen_t* detail, const power_monitor_data_t* data)
{
	if (!detail || !data || !detail->sensor_labels_created) {
		return; // No detail screen, data, or labels not created
	}

	// Get LERP data for smooth display values (same pattern as power grid view)
	lerp_power_monitor_data_t lerp_data;
	lerp_data_get_current(&lerp_data);

	// Update only dynamic values (indices 2,4,7,9,12,14) with LERP display values
	char sensor_text[32];

	// Starter Battery values
	snprintf(sensor_text, sizeof(sensor_text), "%.1f", lerp_value_get_display(&lerp_data.starter_voltage));
	if (detail->sensor_labels[2]) {
		lv_label_set_text(detail->sensor_labels[2], sensor_text);
	}

	snprintf(sensor_text, sizeof(sensor_text), "%.1f", lerp_value_get_display(&lerp_data.starter_current));
	if (detail->sensor_labels[4]) {
		lv_label_set_text(detail->sensor_labels[4], sensor_text);
	}

	// House Battery values
	snprintf(sensor_text, sizeof(sensor_text), "%.1f", lerp_value_get_display(&lerp_data.house_voltage));
	if (detail->sensor_labels[7]) {
		lv_label_set_text(detail->sensor_labels[7], sensor_text);
	}

	snprintf(sensor_text, sizeof(sensor_text), "%.1f", lerp_value_get_display(&lerp_data.house_current));
	if (detail->sensor_labels[9]) {
		lv_label_set_text(detail->sensor_labels[9], sensor_text);
	}

	// Solar Input values
	snprintf(sensor_text, sizeof(sensor_text), "%.1f", lerp_value_get_display(&lerp_data.solar_voltage));
	if (detail->sensor_labels[12]) {
		lv_label_set_text(detail->sensor_labels[12], sensor_text);
	}

	snprintf(sensor_text, sizeof(sensor_text), "%.1f", lerp_value_get_display(&lerp_data.solar_current));
	if (detail->sensor_labels[14]) {
		lv_label_set_text(detail->sensor_labels[14], sensor_text);
	}

	// Apply alert flashing effects (same pattern as power grid view)
	detail_screen_apply_alert_flashing(detail, &lerp_data);
}

void detail_screen_apply_alert_flashing(detail_screen_t* detail, const void* lerp_data_ptr)
{
	if (!detail || !detail->sensor_labels_created || !lerp_data_ptr) {
		return;
	}

	const lerp_power_monitor_data_t* lerp_data = (const lerp_power_monitor_data_t*)lerp_data_ptr;

	// Get alert thresholds (same pattern as power grid view)
	int starter_lo = device_state_get_int("power_monitor.starter_alert_low_voltage_v");
	int starter_hi = device_state_get_int("power_monitor.starter_alert_high_voltage_v");
	int house_lo = device_state_get_int("power_monitor.house_alert_low_voltage_v");
	int house_hi = device_state_get_int("power_monitor.house_alert_high_voltage_v");
	int solar_lo = device_state_get_int("power_monitor.solar_alert_low_voltage_v");
	int solar_hi = device_state_get_int("power_monitor.solar_alert_high_voltage_v");

	// Blink timing - asymmetric: 1 second on, 0.5 seconds off (1.5 second total cycle)
	uint32_t tick_ms = lv_tick_get(); // Use LVGL's tick instead of FreeRTOS
	bool blink_on = (tick_ms % 1500) < 1000;

	// Starter voltage at index 2 - use raw value for threshold checking (same as power grid view)
	float starter_v_raw = lerp_value_get_raw(&lerp_data->starter_voltage);
	bool starter_alert = (starter_v_raw <= (float)starter_lo) || (starter_v_raw >= (float)starter_hi);
	if (starter_alert && detail->sensor_labels[2]) {
		// Only change color, no hiding/showing to prevent flashing (same as power grid view)
		if (blink_on) {
			lv_obj_set_style_text_color(detail->sensor_labels[2], lv_color_hex(0xFF3333), 0); // Red when blinking
		} else {
			lv_obj_set_style_text_color(detail->sensor_labels[2], lv_color_hex(0x00FF00), 0); // Green when not blinking
		}
	} else if (detail->sensor_labels[2]) {
		lv_obj_set_style_text_color(detail->sensor_labels[2], lv_color_hex(0x39ab00), 0); // Normal green
	}

	// House voltage at index 7 - use raw value for threshold checking
	float house_v_raw = lerp_value_get_raw(&lerp_data->house_voltage);
	bool house_alert = (house_v_raw <= (float)house_lo) || (house_v_raw >= (float)house_hi);
	if (house_alert && detail->sensor_labels[7]) {
		// Only change color, no hiding/showing to prevent flashing (same as power grid view)
		if (blink_on) {
			lv_obj_set_style_text_color(detail->sensor_labels[7], lv_color_hex(0xFF3333), 0); // Red when blinking
		} else {
			lv_obj_set_style_text_color(detail->sensor_labels[7], lv_color_hex(0x0080FF), 0); // Blue when not blinking
		}
	} else if (detail->sensor_labels[7]) {
		lv_obj_set_style_text_color(detail->sensor_labels[7], lv_color_hex(0x39ab00), 0); // Normal green
	}

	// Solar voltage at index 12 - use raw value for threshold checking
	float solar_v_raw = lerp_value_get_raw(&lerp_data->solar_voltage);
	bool solar_alert = (solar_v_raw <= (float)solar_lo) || (solar_v_raw >= (float)solar_hi);
	if (solar_alert && detail->sensor_labels[12]) {
		// Only change color, no hiding/showing to prevent flashing (same as power grid view)
		if (blink_on) {
			lv_obj_set_style_text_color(detail->sensor_labels[12], lv_color_hex(0xFF3333), 0); // Red when blinking
		} else {
			lv_obj_set_style_text_color(detail->sensor_labels[12], lv_color_hex(0xFF8000), 0); // Orange when not blinking
		}
	} else if (detail->sensor_labels[12]) {
		lv_obj_set_style_text_color(detail->sensor_labels[12], lv_color_hex(0x39ab00), 0); // Normal green
	}
}
