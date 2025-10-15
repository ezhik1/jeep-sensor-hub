#ifndef MODAL_BUTTONS_H
#define MODAL_BUTTONS_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Standardized modal button container structure
typedef struct {
	lv_obj_t* container;
	lv_obj_t* cancel_button;
	lv_obj_t* close_button;
} modal_button_container_t;

/**
 * @brief Create a standardized button container for modals
 *
 * Creates a button container with consistent styling containing:
 * - Cancel button (left side, RED background, "CANCEL" text)
 * - Close/Done button (right side, GREEN background, "DONE" text)
 *
 * @param parent Parent container to add the button container to
 * @param width Width of the button container (use LV_PCT(100) for full width)
 * @param height Height of the button container
 * @param cancel_callback Callback function for cancel button clicks
 * @param close_callback Callback function for close button clicks
 * @param user_data User data to pass to callbacks
 * @return modal_button_container_t Structure containing the created buttons
 */
modal_button_container_t modal_buttons_create(
	lv_obj_t* parent,
	lv_coord_t width,
	lv_coord_t height,
	lv_event_cb_t cancel_callback,
	lv_event_cb_t close_callback,
	void* user_data
);

/**
 * @brief Destroy a modal button container
 *
 * @param container Button container structure to destroy
 */
void modal_buttons_destroy(modal_button_container_t* container);

#ifdef __cplusplus
}
#endif

#endif // MODAL_BUTTONS_H
