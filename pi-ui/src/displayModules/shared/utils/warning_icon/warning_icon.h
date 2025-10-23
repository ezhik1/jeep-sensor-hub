#ifndef WARNING_ICON_H
#define WARNING_ICON_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Warning icon sizes
typedef enum {
	WARNING_ICON_SIZE_16 = 16,
	WARNING_ICON_SIZE_24 = 24,
	WARNING_ICON_SIZE_32 = 32,
	WARNING_ICON_SIZE_48 = 48
} warning_icon_size_t;

// Create a colorable warning icon
// The icon is a simple warning triangle that can be colored at runtime
lv_obj_t* warning_icon_create(lv_obj_t* parent, warning_icon_size_t size, lv_color_t color);

// Set the color of an existing warning icon
void warning_icon_set_color(lv_obj_t* icon, lv_color_t color);

// Get the recommended icon size based on the warning_icon_size parameter
warning_icon_size_t warning_icon_get_size_from_coord(lv_coord_t coord);

#ifdef __cplusplus
}
#endif

#endif // WARNING_ICON_H
