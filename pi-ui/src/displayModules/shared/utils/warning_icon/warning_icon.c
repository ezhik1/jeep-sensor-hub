#include "warning_icon.h"
#include <stdio.h>
#include <string.h>

// Warning icon bitmap data from arduino projects (30x30 pixels)
// This is a warning triangle icon that can be colorized at runtime
static const uint8_t warning_icon_30x30_data[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x03, 0x00, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0x07, 0x80, 0x00, 0x00, 0x0c, 0xc0, 0x00,
	0x00, 0x1c, 0xe0, 0x00, 0x00, 0x18, 0x60, 0x00, 0x00, 0x30, 0x30, 0x00, 0x00, 0x33, 0x30, 0x00,
	0x00, 0x63, 0x18, 0x00, 0x00, 0xe3, 0x1c, 0x00, 0x00, 0xc3, 0x0c, 0x00, 0x01, 0x83, 0x06, 0x00,
	0x01, 0x83, 0x06, 0x00, 0x03, 0x03, 0x03, 0x00, 0x03, 0x03, 0x03, 0x00, 0x06, 0x00, 0x01, 0x80,
	0x06, 0x00, 0x01, 0x80, 0x0c, 0x03, 0x00, 0xc0, 0x1c, 0x00, 0x00, 0xe0, 0x18, 0x00, 0x00, 0x60,
	0x38, 0x00, 0x00, 0x70, 0x3f, 0xff, 0xff, 0xf0, 0x3f, 0xff, 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


// Convert the 30x30 bitmap data to LVGL image descriptor
static void create_warning_image_descriptor(lv_img_dsc_t* img_dsc, lv_color_t color, int size) {
	// Calculate scaling factor
	float scale = (float)size / 30.0f;

	// Allocate buffer for the scaled image
	static uint8_t img_buffer[48 * 48 * 2]; // Max size buffer (48x48 RGB565)

	// Clear buffer
	memset(img_buffer, 0, sizeof(img_buffer));

	// Convert the 30x30 bitmap to the target size
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			// Map to original 30x30 coordinates
			int orig_x = (int)(x / scale);
			int orig_y = (int)(y / scale);

			if (orig_x >= 30) orig_x = 29;
			if (orig_y >= 30) orig_y = 29;

			// Calculate byte and bit position in original bitmap
			int byte_pos = (orig_y * 4) + (orig_x / 8);
			int bit_pos = 7 - (orig_x % 8);

			if (byte_pos < sizeof(warning_icon_30x30_data)) {
				// Check if bit is set (warning triangle pixel)
				if (warning_icon_30x30_data[byte_pos] & (1 << bit_pos)) {
					// Set pixel in RGB565 format
					int pixel_pos = (y * size + x) * 2;
					if (pixel_pos + 1 < sizeof(img_buffer)) {
						uint16_t rgb565 = lv_color_to_u16(color);
						img_buffer[pixel_pos] = rgb565 & 0xFF;
						img_buffer[pixel_pos + 1] = (rgb565 >> 8) & 0xFF;
					}
				}
			}
		}
	}

	// Set up image descriptor
	img_dsc->header.cf = LV_COLOR_FORMAT_RGB565;
	img_dsc->header.w = size;
	img_dsc->header.h = size;
	img_dsc->data_size = size * size * 2;
	img_dsc->data = img_buffer;
}

lv_obj_t* warning_icon_create(lv_obj_t* parent, warning_icon_size_t size, lv_color_t color) {
	if (!parent) return NULL;

	// Always use 30x30 size for consistency
	int icon_size = 30;

	// Create image descriptor for the warning icon
	static lv_img_dsc_t img_dsc;
	create_warning_image_descriptor(&img_dsc, color, icon_size);

	// Create image object
	lv_obj_t* img = lv_img_create(parent);
	lv_img_set_src(img, &img_dsc);
	lv_obj_set_size(img, icon_size, icon_size);

	// Mark as warning icon for future reference
	lv_obj_add_flag(img, LV_OBJ_FLAG_USER_1);

	return img;
}

void warning_icon_set_color(lv_obj_t* icon, lv_color_t color) {
	if (!icon || !lv_obj_has_flag(icon, LV_OBJ_FLAG_USER_1)) return;

	// Always use the full 30x30 size from arduino projects
	int icon_size = 30;

	// Create new image descriptor with new color
	static lv_img_dsc_t img_dsc;
	create_warning_image_descriptor(&img_dsc, color, icon_size);

	// Update the image source
	lv_img_set_src(icon, &img_dsc);
}

warning_icon_size_t warning_icon_get_size_from_coord(lv_coord_t coord) {
	// Map coordinate size to appropriate enum
	if (coord <= 16) return WARNING_ICON_SIZE_16;
	if (coord <= 24) return WARNING_ICON_SIZE_24;
	if (coord <= 32) return WARNING_ICON_SIZE_32;
	if (coord <= 48) return WARNING_ICON_SIZE_48;
	// For sizes > 48, use the largest available size
	return WARNING_ICON_SIZE_48;
}
