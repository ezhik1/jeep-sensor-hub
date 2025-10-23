#ifndef TIMELINE_MODAL_H
#define TIMELINE_MODAL_H

#include <lvgl.h>
#include <stdbool.h>
#include "../../time_input/time_input.h"
#include "../../utils/animation/animation.h"

#ifdef __cplusplus
extern "C" {
#endif

// Timeline option types
typedef enum {
	TIMELINE_30_SECONDS = 0,
	TIMELINE_1_MINUTE = 1,
	TIMELINE_30_MINUTES = 2,
	TIMELINE_1_HOUR = 3,
	TIMELINE_3_HOURS = 4,
	TIMELINE_COUNT = 5
} timeline_option_t;

// Timeline option configuration
typedef struct {
	const char* label;           // Display label (e.g., "30s", "1min")
	int duration_seconds;        // Duration in seconds
	bool is_selected;            // Whether this option is currently selected
} timeline_option_config_t;

// Timeline gauge configuration
typedef struct {
	const char* name;            // Gauge name (e.g., "STARTER (V)", "HOUSE (V)")
	const char* unit;            // Unit string (e.g., "V", "A", "W")
	bool is_enabled;             // Whether this gauge is enabled
} timeline_gauge_config_t;

// Timeline modal configuration
typedef struct {
	int gauge_count;                                    // Number of gauges
	const timeline_gauge_config_t* gauges;             // Gauge configurations
	const timeline_option_config_t* options;           // Timeline option configurations
	const char* modal_title;                           // Modal title
	void (*on_timeline_changed)(int gauge_index, int duration_seconds, bool is_current_view); // Callback for timeline changes
} timeline_modal_config_t;

// Timeline UI structure - simplified for gauge display
typedef struct {
	lv_obj_t* gauge_container;     // Gauge container parent (holds section and title)
	lv_obj_t* unit_label;          // Unit label
	bool current_view_has_changed; // Whether the current view has been modified
	bool detail_view_has_changed;  // Whether the detail view has been modified
	bool current_view_being_edited; // Whether the current view is currently being edited
	bool detail_view_being_edited;  // Whether the detail view is currently being edited
	float original_current_view_duration;  // Original current view duration for comparison
	float original_detail_view_duration;   // Original detail view duration for comparison

	// Group containers (similar to alerts modal)
	lv_obj_t* current_view_group;       // Current view group container
	lv_obj_t* current_view_title;       // "CURRENT" group title
	lv_obj_t* detail_view_group;        // Detail view group container
	lv_obj_t* detail_view_title;        // "DETAIL" group title

	// Value labels (directly in group containers) - individual H, M, S components
	lv_obj_t* current_view_hours_label;      // Current view hours number (monospace)
	lv_obj_t* current_view_hours_letter;     // Current view "H" letter (Montserrat)
	lv_obj_t* current_view_minutes_label;    // Current view minutes number (monospace)
	lv_obj_t* current_view_minutes_letter;   // Current view "M" letter (Montserrat)
	lv_obj_t* current_view_seconds_label;    // Current view seconds number (monospace)
	lv_obj_t* current_view_seconds_letter;   // Current view "S" letter (Montserrat)
	float current_view_duration;             // Current view timeline duration in seconds

	lv_obj_t* detail_view_hours_label;       // Detail view hours number (monospace)
	lv_obj_t* detail_view_hours_letter;      // Detail view "H" letter (Montserrat)
	lv_obj_t* detail_view_minutes_label;     // Detail view minutes number (monospace)
	lv_obj_t* detail_view_minutes_letter;    // Detail view "M" letter (Montserrat)
	lv_obj_t* detail_view_seconds_label;     // Detail view seconds number (monospace)
	lv_obj_t* detail_view_seconds_letter;    // Detail view "S" letter (Montserrat)
	float detail_view_duration;              // Detail view timeline duration in seconds
} timeline_ui_t;

/**
 * @brief Timeline Modal Structure
 *
 * An interactive modal that displays timeline options for each gauge,
 * following the same layout structure as the alerts modal.
 */
typedef struct {
	lv_obj_t* background;           // Modal background
	lv_obj_t* content_container;    // Main content container
	lv_obj_t* close_button;         // Close button
	lv_obj_t* cancel_button;        // Cancel button

	// Dynamic gauge sections (allocated based on config)
	lv_obj_t** gauge_sections;      // Gauge section containers
	lv_obj_t** gauge_titles;        // Gauge section titles

	// Timeline UI objects - one per gauge
	timeline_ui_t* gauge_ui;

	// Shared time input component
	time_input_t* time_input;       // Shared time input component
	int selected_gauge;             // Currently selected gauge index
	bool selected_is_current_view;  // Whether the selected timeline is for current view

	// Configuration
	timeline_modal_config_t config; // Modal configuration
	int current_duration;           // Current selected duration in seconds

	// Animation
	animation_manager_t* animation_manager; // Animation manager for smooth value transitions

	void (*on_close)(void);         // Close callback
	bool is_visible;                // Visibility state
} timeline_modal_t;

/**
 * @brief Create a new timeline modal
 * @param config Configuration for the modal (gauges, options, callbacks, etc.)
 * @param on_close_callback Function to call when modal is closed
 * @return Pointer to created modal, or NULL on failure
 */
timeline_modal_t* timeline_modal_create(const timeline_modal_config_t* config, void (*on_close_callback)(void));

/**
 * @brief Show the timeline modal
 * @param modal Pointer to the modal
 */
void timeline_modal_show(timeline_modal_t* modal);

/**
 * @brief Hide the timeline modal
 * @param modal Pointer to the modal
 */
void timeline_modal_hide(timeline_modal_t* modal);

/**
 * @brief Check if the timeline modal is visible
 * @param modal Timeline modal instance
 * @return true if visible, false otherwise
 */
bool timeline_modal_is_visible(timeline_modal_t* modal);

/**
 * @brief Get the current timeline duration
 * @param modal Pointer to the modal
 * @return Duration in seconds, or -1 if modal is invalid
 */
int timeline_modal_get_duration(timeline_modal_t* modal);

/**
 * @brief Set a gauge value
 * @param modal Pointer to the modal
 * @param gauge Gauge index (0-based)
 * @param value Value to set
 */
void timeline_modal_set_gauge_value(timeline_modal_t* modal, int gauge, float value);

/**
 * @brief Destroy the timeline modal
 * @param modal Pointer to the modal
 */
void timeline_modal_destroy(timeline_modal_t* modal);

#ifdef __cplusplus
}
#endif

#endif // TIMELINE_MODAL_H