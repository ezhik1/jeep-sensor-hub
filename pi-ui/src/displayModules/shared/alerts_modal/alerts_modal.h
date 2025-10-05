#ifndef ALERTS_MODAL_H
#define ALERTS_MODAL_H

#include <lvgl.h>
#include <stdbool.h>
#include "../numberpad/numberpad.h"

#ifdef __cplusplus
extern "C" {
#endif

// Number of editable fields per Gauge type
#define FIELD_COUNT_PER_GAUGE 5
#define GAUGE_COUNT 3
#define TOTAL_FIELD_COUNT (FIELD_COUNT_PER_GAUGE * GAUGE_COUNT)

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

// Gauge types
typedef enum {
	GAUGE_STARTER = 0,
	GAUGE_HOUSE = 1,
	GAUGE_SOLAR = 2
} gauge_type_t;

// Field UI structure - only for UI layout and objects
typedef struct {
	lv_obj_t* button;           // The UI button
	lv_obj_t* label;            // The value label
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

	// Field identification
	int gauge_index;          // Which Gauge this belongs to (0-2)
	int field_index;            // Which field type this is (0-4)
	int group_type;             // Which group it belongs to (ALERTS=0, GAUGE=1)

	// UI state
	lv_color_t border_color;    // Current border color
	int border_width;           // Current border width
	lv_color_t text_color;      // Current text color
} field_data_t;

/**
 * @brief Enhanced Alerts Modal Structure
 *
 * An interactive modal that displays and allows editing of voltage alert thresholds
 * and gauge configuration settings for all Gauge types.
 */
typedef struct {
	lv_obj_t* background;           // Modal background
	lv_obj_t* content_container;    // Main content container
	lv_obj_t* title_label;          // Modal title
	lv_obj_t* close_button;         // Close button

	// Gauge sections
	lv_obj_t* gauge_sections[GAUGE_COUNT];      // Gauge section containers
	lv_obj_t* alert_groups[GAUGE_COUNT];          // Alert group containers
	lv_obj_t* gauge_groups[GAUGE_COUNT];          // Gauge group containers

	// Title labels for caching
	lv_obj_t* gauge_titles[GAUGE_COUNT];         // Gauge section titles
	lv_obj_t* alert_titles[GAUGE_COUNT];         // Alert group titles
	lv_obj_t* gauge_group_titles[GAUGE_COUNT];   // Gauge group titles

	// Field UI objects - 1D array for UI layout
	field_ui_t field_ui[GAUGE_COUNT * FIELD_COUNT_PER_GAUGE];

	// Field data - 1D array for data and state management
	field_data_t field_data[GAUGE_COUNT * FIELD_COUNT_PER_GAUGE];

	int current_field_id;              // Currently editing field ID (-1 = none)

	// Shared numberpad component
	numberpad_t* numberpad;

	void (*on_close)(void);         // Close callback
	bool is_visible;                // Visibility state
	bool numberpad_visible;         // Numberpad visibility state
} alerts_modal_t;

/**
 * @brief Create a new enhanced alerts modal
 * @param on_close_callback Function to call when modal is closed
 * @return Pointer to created modal, or NULL on failure
 */
alerts_modal_t* alerts_modal_create(void (*on_close_callback)(void));

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
 * @brief Show the numberpad for editing a field
 * @param modal Pointer to the modal
 * @param Gauge Gauge type being edited
 * @param field Field type being edited
 */
void alerts_modal_show_numberpad(alerts_modal_t* modal, gauge_type_t Gauge, field_type_t field);

/**
 * @brief Hide the numberpad
 * @param modal Pointer to the modal
 */
void alerts_modal_hide_numberpad(alerts_modal_t* modal);

// Dim all elements except the target field
void alerts_modal_dim_for_focus(alerts_modal_t* modal, gauge_type_t Gauge, field_type_t field);

// Restore all elements to normal colors
void alerts_modal_restore_colors(alerts_modal_t* modal);

/**
 * @brief Get the current value of a field
 * @param modal Pointer to the modal
 * @param Gauge Gauge type
 * @param field Field type
 * @return Current field value
 */
float alerts_modal_get_field_value(alerts_modal_t* modal, gauge_type_t Gauge, field_type_t field);

/**
 * @brief Set the value of a field
 * @param modal Pointer to the modal
 * @param Gauge Gauge type
 * @param field Field type
 * @param value New value to set
 */
void alerts_modal_set_field_value(alerts_modal_t* modal, gauge_type_t Gauge, field_type_t field, float value);

#ifdef __cplusplus
}
#endif

#endif // ALERTS_MODAL_H
