#include "boot_screen.h"
#include <lvgl.h>
#include "esp_compat.h"

static const char *TAG = "boot_screen";

static lv_obj_t *boot_container = NULL;
static lv_obj_t *logo_label = NULL;
static lv_obj_t *loading_label = NULL;
static lv_obj_t *progress_bar = NULL;

void boot_screen_init(void)
{
	ESP_LOGI(TAG, "Initializing boot screen");

	// Create main container
	boot_container = lv_obj_create(lv_scr_act());
	lv_obj_set_size(boot_container, LV_PCT(100), LV_PCT(100));
	lv_obj_set_style_pad_all(boot_container, 0, 0);
	lv_obj_set_style_bg_color(boot_container, lv_color_hex(0x000000), 0); // Black background
	lv_obj_set_style_border_width(boot_container, 0, 0);

	ESP_LOGI(TAG, "Boot screen container created");

	// Create logo/title label
	logo_label = lv_label_create(boot_container);
	lv_label_set_text(logo_label, "JEEP SENSOR HUB");
	lv_obj_set_style_text_font(logo_label, &lv_font_montserrat_24, 0);
	lv_obj_set_style_text_color(logo_label, lv_color_hex(0xFFFFFF), 0); // White text on black background
	lv_obj_align(logo_label, LV_ALIGN_CENTER, 0, -50);

	// Create loading label
	loading_label = lv_label_create(boot_container);
	lv_label_set_text(loading_label, "Initializing...");
	lv_obj_set_style_text_font(loading_label, &lv_font_montserrat_16, 0);
	lv_obj_set_style_text_color(loading_label, lv_color_hex(0xCCCCCC), 0);
	lv_obj_align(loading_label, LV_ALIGN_CENTER, 0, 0);

	// Create progress bar
	progress_bar = lv_bar_create(boot_container);
	lv_obj_set_size(progress_bar, 200, 20);
	lv_obj_align(progress_bar, LV_ALIGN_CENTER, 0, 50);
	lv_bar_set_range(progress_bar, 0, 100);
	lv_bar_set_value(progress_bar, 0, LV_ANIM_OFF);
	lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x333333), LV_PART_MAIN);
	lv_obj_set_style_bg_color(progress_bar, lv_color_hex(0x00AA00), LV_PART_INDICATOR);

	ESP_LOGI(TAG, "Boot screen initialized successfully");
}

void boot_screen_update_progress(int progress)
{
	ESP_LOGI(TAG, "Updating boot screen progress: %d%%", progress);
	if (progress_bar) {
		lv_bar_set_value(progress_bar, progress, LV_ANIM_ON);
		ESP_LOGI(TAG, "Progress bar updated to %d%%", progress);
	} else {
		ESP_LOGW(TAG, "Progress bar not available for update");
	}
}

void boot_screen_cleanup(void)
{
	ESP_LOGI(TAG, "Cleaning up boot screen");
	if (boot_container) {
	// Clear all child objects first to avoid event handler issues
	lv_obj_t *child = lv_obj_get_child(boot_container, 0);
	while (child) {
		lv_obj_t *next = lv_obj_get_child(boot_container, 0);
		if (child != next) {  // Safety check to prevent infinite loop
			lv_obj_del(child);
			child = next;
		} else {
			lv_obj_del(child);
			break;
		}
	}

	// Use lv_obj_del for safer deletion
	lv_obj_del(boot_container);
		boot_container = NULL;
		logo_label = NULL;
		loading_label = NULL;
		progress_bar = NULL;
		ESP_LOGI(TAG, "Boot screen objects deleted");
	} else {
		ESP_LOGW(TAG, "Boot container not found for cleanup");
	}
}
