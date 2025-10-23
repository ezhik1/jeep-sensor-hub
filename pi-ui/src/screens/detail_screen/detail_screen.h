#ifndef DETAIL_SCREEN_H
#define DETAIL_SCREEN_H

#include <lvgl.h>

// Forward declaration for overlay (not used in Pi port)
// typedef struct overlay_instance overlay_instance_t; // Removed to avoid conflicts

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Detail Screen Template
 *
 * Provides a standardized detail screen layout that all modules can use.
 * This eliminates code duplication and ensures consistent UX across modules.
 *
 * Layout structure:
 * - Header with module name and back button
 * - Current view container (for overlay)
 * - Gauges/metrics section
 * - Settings button
 * - Status indicators
 */

// Button configuration structure
typedef struct {
	const char* text;                    // Button text
	void (*on_clicked)(void);            // Button click callback
} detail_button_config_t;

typedef struct {
	lv_obj_t* root;                      // Root container overlay on main screen
	lv_obj_t* main_content;              // Main content container
	lv_obj_t* left_column;               // Left column container for current view and raw values
	lv_obj_t* header;                    // Header section
	lv_obj_t* back_button;               // Back navigation button
	lv_obj_t* title_label;               // Module title
	lv_obj_t* current_view_container;    // Container for current view overlay
	lv_obj_t* gauges_container;          // Container for additional gauges
	lv_obj_t* sensor_data_section;       // Raw sensor data section
	lv_obj_t* settings_section;          // Settings section container
	lv_obj_t** setting_buttons;          // Dynamic array of settings buttons
	int setting_buttons_count;           // Number of settings buttons
	lv_obj_t* status_container;          // Status indicators container

	// Sensor data labels removed - modules should handle their own data display

	// Module-specific data
	const char* module_name;             // Module identifier
	const char* display_name;            // Human-readable name
	void* current_view_overlay; // Shared overlay instance (not used in Pi port)

	// Button configurations (stored for event handling)
	detail_button_config_t* button_configs; // Array of button configurations

	// Callbacks
	void (*on_back_clicked)(void);       // Back button callback
	void (*on_view_clicked)(void);       // Current view click callback (for cycling)
	void (*on_current_view_created)(lv_obj_t* container); // Callback when current view container is ready
	void (*on_gauges_created)(lv_obj_t* container);       // Callback when gauges container is ready
	void (*on_sensor_data_created)(lv_obj_t* container);  // Callback when sensor data container is ready
} detail_screen_t;

/**
 * @brief Configuration for detail screen creation
 */
typedef struct {
	const char* module_name;             // Module identifier
	const char* display_name;            // Human-readable title

	// Layout customization
	bool show_gauges_section;            // Whether to show additional gauges
	bool show_settings_button;           // Whether to show settings button
	bool show_status_indicators;         // Whether to show status section

	// Dynamic button configuration
	detail_button_config_t* setting_buttons; // Array of button configurations
	int setting_buttons_count;           // Number of buttons

	// Callbacks
	void (*on_back_clicked)(void);       // Back button handler
	void (*on_view_clicked)(void);       // View click handler
	void (*on_current_view_created)(lv_obj_t* container); // Callback when current view container is ready
	void (*on_gauges_created)(lv_obj_t* container);       // Callback when gauges container is ready
	void (*on_sensor_data_created)(lv_obj_t* container);  // Callback when sensor data container is ready

	// Overlay configuration
	lv_obj_t* (*overlay_creator)(lv_obj_t* parent, void* user_data); // Overlay content creator
	void* overlay_user_data;             // Data passed to overlay creator
} detail_screen_config_t;

/**
 * @brief Create a detail screen using the template
 * @param config Configuration for the detail screen
 * @return Detail screen instance
 */
detail_screen_t* detail_screen_create(const detail_screen_config_t* config);

/**
 * @brief Show the detail screen
 * @param detail Detail screen instance
 */
void detail_screen_show(detail_screen_t* detail);

/**
 * @brief Hide the detail screen
 * @param detail Detail screen instance
 */
void detail_screen_hide(detail_screen_t* detail);

/**
 * @brief Update detail screen with new data
 * @param detail Detail screen instance
 * @param data Generic module data (void* for flexibility)
 */
void detail_screen_update(detail_screen_t* detail, const void* data);

/**
 * @brief Destroy detail screen
 * @param detail Detail screen instance
 */
void detail_screen_destroy(detail_screen_t* detail);

/**
 * @brief Get the current view container for overlay positioning
 * @param detail Detail screen instance
 * @return Current view container
 */
lv_obj_t* detail_screen_get_current_view_container(detail_screen_t* detail);

// Sensor-specific functions removed - modules should implement their own data display

/**
 * @brief Get the gauges container for additional widgets
 * @param detail Detail screen instance
 * @return Gauges container
 */
lv_obj_t* detail_screen_get_gauges_container(detail_screen_t* detail);

/**
 * @brief Get the status container for status indicators
 * @param detail Detail screen instance
 * @return Status container
 */
lv_obj_t* detail_screen_get_status_container(detail_screen_t* detail);

/**
 * @brief Restore current view container styling after clean operation
 * @param container The current view container to restore styling for
 */
void detail_screen_restore_current_view_styling(lv_obj_t* container);

/**
 * @brief Prepare current view container layout for content creation
 * @param detail Detail screen instance
 * @return true if layout preparation was successful, false otherwise
 */
bool detail_screen_prepare_current_view_layout(detail_screen_t* detail);

/**
 * @brief Generic modal toggle function
 * @param modal_name Name of the modal to toggle
 * @param create_func Function to create the modal
 * @param destroy_func Function to destroy the modal
 * @param show_func Function to show the modal
 * @param is_visible_func Function to check if modal is visible
 * @param config Configuration for the modal
 * @param on_close_callback Callback when modal is closed
 */
void detail_screen_toggle_modal(const char* modal_name,
	void* (*create_func)(void* config, void (*on_close)(void)),
	void (*destroy_func)(void* modal),
	void (*show_func)(void* modal),
	bool (*is_visible_func)(void* modal),
	void* config,
	void (*on_close_callback)(void));

/**
 * @brief Check if a modal is currently visible
 * @param modal_name Name of the modal to check
 * @return true if modal is visible, false otherwise
 */
bool detail_screen_is_modal_visible(const char* modal_name);

/**
 * @brief Reset modal tracking system (call when detail screen is recreated)
 */
void detail_screen_reset_modal_tracking(void);

#ifdef __cplusplus
}
#endif

#endif // DETAIL_SCREEN_H
