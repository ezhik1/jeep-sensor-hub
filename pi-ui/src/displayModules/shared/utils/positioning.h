#ifndef POSITIONING_H
#define POSITIONING_H

#include "lvgl.h"

/**
 * @brief Smart positioning algorithm for UI elements
 *
 * Positions a UI element relative to a target field and container using multiple strategies
 * to ensure it appears outside the container and fits on screen.
 *
 * @param element The UI element to position
 * @param target_field The field to align with
 * @param container The container to avoid overlapping with
 * @param min_gap Minimum gap between element and field/container (default: 20)
 * @param screen_margin Screen margin (default: 5)
 */
void smart_position_outside_container(lv_obj_t* element, lv_obj_t* target_field, lv_obj_t* container,
									lv_coord_t min_gap, lv_coord_t screen_margin);

/**
 * @brief Smart positioning with default parameters
 */
void smart_position_outside_container_default(lv_obj_t* element, lv_obj_t* target_field, lv_obj_t* container);

#endif // POSITIONING_H
