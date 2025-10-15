#include "positioning.h"
#include <stdio.h>
#include <math.h>

void smart_position_outside_container(lv_obj_t* element, lv_obj_t* target_field, lv_obj_t* container,
									lv_coord_t min_gap, lv_coord_t screen_margin)
{
	if (!element || !target_field || !container) return;

	// Ensure layout is updated before getting coordinates
	lv_obj_update_layout(target_field);
	lv_obj_update_layout(container);

	// Get target field coordinates (for alignment)
	lv_area_t field_coords;
	lv_obj_get_coords(target_field, &field_coords);

	// Get container coordinates (for boundary checking)
	lv_area_t container_coords;
	lv_obj_get_coords(container, &container_coords);

	// Get screen dimensions
	lv_coord_t screen_width = lv_obj_get_width(lv_screen_active());
	lv_coord_t screen_height = lv_obj_get_height(lv_screen_active());

	// Get element dimensions
	lv_coord_t element_width = lv_obj_get_width(element);
	lv_coord_t element_height = lv_obj_get_height(element);

	// Calculate field center (for alignment)
	lv_coord_t field_center_x = field_coords.x1 + lv_area_get_width(&field_coords) / 2;
	lv_coord_t field_center_y = field_coords.y1 + lv_area_get_height(&field_coords) / 2;

	// Step 1: Determine optimal X position (left-right) that fits on screen and avoids container collision
	lv_coord_t best_x = 0;
	bool x_found = false;

	// Try to left-align to screen first
	best_x = screen_margin;

	// Check if left-aligned position fits on screen and doesn't collide with container
	if (best_x + element_width <= screen_width - screen_margin &&
		!(best_x < container_coords.x2 && best_x + element_width > container_coords.x1)) {
		x_found = true;
		printf("[I] positioning: Left-aligned X position works\n");
	}

	// If centered doesn't work, try to the right of container
	if (!x_found) {
		best_x = container_coords.x2 + min_gap;
		if (best_x >= screen_margin && best_x + element_width <= screen_width - screen_margin) {
			x_found = true;
			printf("[I] positioning: Positioned X to the right of container\n");
		}
	}

	// If right doesn't work, try to the left of container
	if (!x_found) {
		best_x = container_coords.x1 - element_width - min_gap;
		if (best_x >= screen_margin && best_x + element_width <= screen_width - screen_margin) {
			x_found = true;
			printf("[I] positioning: Positioned X to the left of container\n");
		}
	}

	// If still no X position, force it to screen boundaries
	if (!x_found) {
		best_x = field_center_x - element_width / 2;
		if (best_x < screen_margin) best_x = screen_margin;
		if (best_x + element_width > screen_width - screen_margin) best_x = screen_width - element_width - screen_margin;
		printf("[I] positioning: Forced X to screen boundaries\n");
	}

	// Step 2: Determine optimal Y position (top-bottom) that fits on screen and avoids container collision
	lv_coord_t best_y = 0;
	bool found_position = false;

	// Strategy 1: Try below container (preferred)
	best_y = container_coords.y2 + min_gap;
	if (best_y >= screen_margin && best_y + element_height <= screen_height - screen_margin &&
		!(best_y < container_coords.y2 && best_y + element_height > container_coords.y1)) {
		found_position = true;
		printf("[I] positioning: Positioned below container\n");
	}

	// Strategy 2: Try above container
	if (!found_position) {
		best_y = container_coords.y1 - element_height - min_gap;
		if (best_y >= screen_margin && best_y + element_height <= screen_height - screen_margin &&
			!(best_y < container_coords.y2 && best_y + element_height > container_coords.y1)) {
			found_position = true;
			printf("[I] positioning: Positioned above container\n");
		}
	}

	// Strategy 3: Try to the right of container (if X allows)
	if (!found_position && best_x >= container_coords.x2 + min_gap) {
		best_y = field_center_y - element_height / 2;
		if (best_y >= screen_margin && best_y + element_height <= screen_height - screen_margin) {
			found_position = true;
			printf("[I] positioning: Positioned to the right of container\n");
		}
	}

	// Strategy 4: Try to the left of container (if X allows)
	if (!found_position && best_x + element_width <= container_coords.x1 - min_gap) {
		best_y = field_center_y - element_height / 2;
		if (best_y >= screen_margin && best_y + element_height <= screen_height - screen_margin) {
			found_position = true;
			printf("[I] positioning: Positioned to the left of container\n");
		}
	}

	// Strategy 5: Fallback - force below container with screen adjustments
	if (!found_position) {
		best_y = container_coords.y2 + min_gap;
		if (best_y + element_height > screen_height - screen_margin) {
			best_y = screen_height - element_height - screen_margin;
		}
		found_position = true;
		printf("[I] positioning: Fallback - forced below container with screen adjustments\n");
	}

	// Set the final position
	lv_obj_set_pos(element, best_x, best_y);

	printf("[I] positioning: Element positioned at (%d, %d) for field at (%d, %d), container at (%d, %d)\n",
		   best_x, best_y, field_coords.x1, field_coords.y1, container_coords.x1, container_coords.y1);
}

void smart_position_outside_container_default(lv_obj_t* element, lv_obj_t* target_field, lv_obj_t* container)
{
	smart_position_outside_container(element, target_field, container, 20, 5);
}

int clamp_int(int value, int min, int max)
{
	return (int)fmax(min, fmin(max, value));
}

float clamp_float(float value, float min, float max)
{
	return fmax(min, fmin(max, value));
}
