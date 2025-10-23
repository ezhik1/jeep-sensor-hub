#ifndef ALERTS_MODAL_H
#define ALERTS_MODAL_H

#include <lvgl.h>
#include <stdbool.h>
#include "../../numberpad/numberpad.h"

#ifdef __cplusplus
extern "C" {
#endif

// Field types for each Gauge
typedef enum {
	FIELD_ALERT_LOW = 0,
	FIELD_ALERT_HIGH = 1,
	FIELD_GAUGE_LOW = 2,
	FIELD_GAUGE_BASELINE = 3,
	FIELD_GAUGE_HIGH = 4
} field_type_t;

// Group types
typedef enum {
	GROUP_ALERTS = 0,
	GROUP_GAUGE = 1
} group_type_t;

// Callback function types for getting/setting values
typedef float (*alerts_modal_get_value_callback_t)(int gauge_index, int field_type);
typedef void (*alerts_modal_set_value_callback_t)(int gauge_index, int field_type, float value);
typedef void (*alerts_modal_refresh_callback_t)(void);

// Field configuration structure
typedef struct {
	const char* name;           // Field name (e.g., "LOW", "HIGH", "BASE")
	float min_value;            // Minimum allowed value
	float max_value;            // Maximum allowed value
	float default_value;        // Default value
	bool is_baseline;           // Whether this is a baseline field (affects display mode)
} alerts_modal_field_config_t;

// Gauge configuration structure
typedef struct {
	const char* name;           // Gauge name (e.g., "STARTER (V)", "HOUSE (V)")
	const char* unit;           // Unit string (e.g., "V", "A", "W")
	float raw_min_value;        // RAW_MIN: absolute minimum value of raw data
	float raw_max_value;        // RAW_MAX: absolute maximum value of raw data
	alerts_modal_field_config_t fields[5]; // Field configurations (LOW, HIGH, LOW, BASE, HIGH)
	bool has_baseline;          // Whether this gauge has a baseline field
} alerts_modal_gauge_config_t;

// Modal configuration structure
typedef struct {
	int gauge_count;                                    // Number of gauges
	alerts_modal_gauge_config_t* gauges;               // Gauge configurations
	alerts_modal_get_value_callback_t get_value_cb;    // Callback to get current values
	alerts_modal_set_value_callback_t set_value_cb;    // Callback to set values
	alerts_modal_refresh_callback_t refresh_cb;        // Callback to refresh displays
	const char* modal_title;                           // Modal title (optional)
} alerts_modal_config_t;

// Field UI structure - only for UI layout and objects
typedef struct {
	lv_obj_t* button;           // The UI button
	lv_obj_t* label;            // The value label
	lv_obj_t* title;            // The title label
} field_ui_t;

// Field data structure - complete state management for each field
typedef struct {
	// Value data
	float current_value;        // Current value
	float original_value;       // Value when modal opened
	float min_value;            // Minimum allowed value
	float max_value;            // Maximum allowed value
	float default_value;        // Default value

	// State flags
	bool is_being_edited;       // Currently being edited
	bool has_changed;           // Changed from original
	bool is_out_of_range;       // Currently out of range
	bool is_warning_highlighted; // Currently highlighted for warning
	bool is_updated_warning;    // Currently showing "UPDATED" warning

	// Field identification
	int gauge_index;          // Which Gauge this belongs to
	int field_index;            // Which field type this is (0-4)
	int group_type;             // Which group it belongs to (ALERTS=0, GAUGE=1)

	// UI state
	lv_color_t border_color;    // Current border color
	int border_width;           // Current border width
	lv_color_t text_color;      // Current text color
	lv_color_t text_background_color; // Current text background color
	lv_color_t title_color;      // Current title color
	lv_color_t title_background_color; // Current title background color
	lv_color_t button_background_color; // Current button background color
} field_data_t;

/**
 * @brief Generic Alerts Modal Structure
 *
 * An interactive modal that displays and allows editing of alert thresholds
 * and gauge configuration settings for any gauge types.
 */
typedef struct {
	lv_obj_t* background;           // Modal background
	lv_obj_t* content_container;    // Main content container
	lv_obj_t* title_label;          // Modal title
	lv_obj_t* close_button;         // Close button
	lv_obj_t* cancel_button;        // Cancel button

	// Dynamic gauge sections (allocated based on config)
	lv_obj_t** gauge_sections;      // Gauge section containers
	lv_obj_t** alert_groups;        // Alert group containers
	lv_obj_t** gauge_groups;        // Gauge group containers

	// Title labels for caching (allocated based on config)
	lv_obj_t** gauge_titles;        // Gauge section titles
	lv_obj_t** alert_titles;        // Alert group titles
	lv_obj_t** gauge_group_title;   // Gauge group titles

	// Field UI objects - 1D array for UI layout (allocated based on config)
	field_ui_t* field_ui;

	// Field data - 1D array for data and state management (allocated based on config)
	field_data_t* field_data;

	// Configuration
	alerts_modal_config_t config;   // Modal configuration
	int total_field_count;          // Total number of fields (gauge_count * 5)

	int current_field_id;              // Currently editing field ID (-1 = none)

	// Shared numberpad component
	numberpad_t* numberpad;

	void (*on_close)(void);         // Close callback
	bool is_visible;                // Visibility state
	bool numberpad_visible;         // Numberpad visibility state
	bool field_transition_in_progress; // Flag to prevent click handler from closing during field transition
} alerts_modal_t;

/**
 * @brief Create a new generic alerts modal
 * @param config Configuration for the modal (gauge types, callbacks, etc.)
 * @param on_close_callback Function to call when modal is closed
 * @return Pointer to created modal, or NULL on failure
 */
alerts_modal_t* alerts_modal_create(const alerts_modal_config_t* config, void (*on_close_callback)(void));

/**
 * @brief Show the alerts modal
 * @param modal Pointer to the modal
 */
void alerts_modal_show(alerts_modal_t* modal);

/**
 * @brief Hide the alerts modal
 * @param modal Pointer to the modal
 */
void alerts_modal_hide(alerts_modal_t* modal);

/**
 * @brief Destroy the alerts modal and free resources
 * @param modal Pointer to the modal
 */
void alerts_modal_destroy(alerts_modal_t* modal);

/**
 * @brief Check if the modal is currently visible
 * @param modal Pointer to the modal
 * @return true if visible, false otherwise
 */
bool alerts_modal_is_visible(alerts_modal_t* modal);

/**
 * @brief Update all gauge ranges and alert thresholds after modal changes
 * This function should be called after the modal closes to refresh all displays
 * @param modal Pointer to the modal
 */
void alerts_modal_refresh_gauges_and_alerts(alerts_modal_t* modal);

#ifdef __cplusplus
}
#endif

#endif // ALERTS_MODAL_H
