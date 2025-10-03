#ifndef ALERTS_MODAL_H
#define ALERTS_MODAL_H

#include <lvgl.h>
#include <stdbool.h>
#include "../numberpad/numberpad.h"

#ifdef __cplusplus
extern "C" {
#endif

// Number of editable fields per battery type
#define FIELD_COUNT_PER_BATTERY 5
#define BATTERY_COUNT 3
#define TOTAL_FIELD_COUNT (FIELD_COUNT_PER_BATTERY * BATTERY_COUNT)

// Field types for each battery
typedef enum {
	FIELD_ALERT_LOW = 0,
	FIELD_ALERT_HIGH = 1,
	FIELD_GAUGE_LOW = 2,
	FIELD_GAUGE_BASELINE = 3,
	FIELD_GAUGE_HIGH = 4
} field_type_t;

// Battery types
typedef enum {
	BATTERY_STARTER = 0,
	BATTERY_HOUSE = 1,
	BATTERY_SOLAR = 2
} battery_type_t;

/**
 * @brief Enhanced Alerts Modal Structure
 *
 * An interactive modal that displays and allows editing of voltage alert thresholds
 * and gauge configuration settings for all battery types.
 */
typedef struct {
	lv_obj_t* background;           // Modal background
	lv_obj_t* content_container;    // Main content container
	lv_obj_t* scroll_container;     // Scrollable content container
	lv_obj_t* title_label;          // Modal title
	lv_obj_t* close_button;         // Close button

	// Battery sections
	lv_obj_t* battery_sections[BATTERY_COUNT];      // Battery section containers
	lv_obj_t* alert_groups[BATTERY_COUNT];          // Alert group containers
	lv_obj_t* gauge_groups[BATTERY_COUNT];          // Gauge group containers

	// Editable fields - organized as [battery][field_type]
	lv_obj_t* field_buttons[BATTERY_COUNT][FIELD_COUNT_PER_BATTERY];
	float field_values[BATTERY_COUNT][FIELD_COUNT_PER_BATTERY];
	bool field_opened_before[BATTERY_COUNT][FIELD_COUNT_PER_BATTERY]; // Track if field was opened before

	// Shared numberpad component
	numberpad_t* numberpad;
	int current_battery;               // Currently editing battery (-1 = none)
	int current_field;                 // Currently editing field (-1 = none)

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
 * @param battery Battery type being edited
 * @param field Field type being edited
 */
void alerts_modal_show_numberpad(alerts_modal_t* modal, battery_type_t battery, field_type_t field);

/**
 * @brief Hide the numberpad
 * @param modal Pointer to the modal
 */
void alerts_modal_hide_numberpad(alerts_modal_t* modal);

// Dim all elements except the target field
void alerts_modal_dim_for_focus(alerts_modal_t* modal, battery_type_t battery, field_type_t field);

// Restore all elements to normal colors
void alerts_modal_restore_colors(alerts_modal_t* modal);

/**
 * @brief Get the current value of a field
 * @param modal Pointer to the modal
 * @param battery Battery type
 * @param field Field type
 * @return Current field value
 */
float alerts_modal_get_field_value(alerts_modal_t* modal, battery_type_t battery, field_type_t field);

/**
 * @brief Set the value of a field
 * @param modal Pointer to the modal
 * @param battery Battery type
 * @param field Field type
 * @param value New value to set
 */
void alerts_modal_set_field_value(alerts_modal_t* modal, battery_type_t battery, field_type_t field, float value);

#ifdef __cplusplus
}
#endif

#endif // ALERTS_MODAL_H
