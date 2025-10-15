#include "modal_buttons.h"
#include "../palette.h"
#include <stdio.h>

modal_button_container_t modal_buttons_create(
	lv_obj_t* parent,
	lv_coord_t width,
	lv_coord_t height,
	lv_event_cb_t cancel_callback,
	lv_event_cb_t close_callback,
	void* user_data
) {
	modal_button_container_t container = {0};

	// Create button container
	container.container = lv_obj_create(parent);
	lv_obj_set_size(container.container, width, height);
	lv_obj_set_layout(container.container, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(container.container, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(container.container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_bg_color(container.container, PALETTE_BLACK, 0);
	lv_obj_set_style_bg_opa(container.container, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(container.container, 0, 0);
	lv_obj_set_style_pad_all(container.container, 0, 0);
	lv_obj_clear_flag(container.container, LV_OBJ_FLAG_SCROLLABLE);

	// Cancel Button - left side (RED)
	container.cancel_button = lv_button_create(container.container);
	lv_obj_set_size(container.cancel_button, 100, 50);
	lv_obj_set_style_bg_color(container.cancel_button, PALETTE_RED, 0);
	if (cancel_callback) {
		lv_obj_add_event_cb(container.cancel_button, cancel_callback, LV_EVENT_CLICKED, user_data);
	}

	lv_obj_t* cancel_label = lv_label_create(container.cancel_button);
	lv_label_set_text(cancel_label, "CANCEL");
	lv_obj_set_style_text_color(cancel_label, PALETTE_WHITE, 0);
	lv_obj_center(cancel_label);

	// Close Button - right side (GREEN, "DONE")
	container.close_button = lv_button_create(container.container);
	lv_obj_set_size(container.close_button, 100, 50);
	lv_obj_set_style_bg_color(container.close_button, PALETTE_GREEN, 0);
	if (close_callback) {
		lv_obj_add_event_cb(container.close_button, close_callback, LV_EVENT_CLICKED, user_data);
	}

	lv_obj_t* close_label = lv_label_create(container.close_button);
	lv_label_set_text(close_label, "DONE");
	lv_obj_set_style_text_color(close_label, PALETTE_WHITE, 0);
	lv_obj_center(close_label);

	printf("[I] modal_buttons: Standardized button container created\n");
	return container;
}

void modal_buttons_destroy(modal_button_container_t* container) {
	if (!container) return;

	if (container->container) {
		lv_obj_del(container->container);
	}

	// Clear the structure
	container->container = NULL;
	container->cancel_button = NULL;
	container->close_button = NULL;

	printf("[I] modal_buttons: Button container destroyed\n");
}
