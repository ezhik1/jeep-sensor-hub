#include "lvgl_port_pi.h"
#include "screens/screen_manager.h"
#include "screens/home_screen/home_screen.h"
#include "screens/boot_screen/boot_screen.h"

#include "state/device_state.h"
#include "displayModules/shared/module_interface.h"

#include "data/config.h"
#include "data/mock_data/mock_data.h"
#include "data/real_data/real_data.h"

#include "esp_compat.h"
#include <unistd.h>

static const char *TAG = "main";

/* =========================
   LVGL UI UPDATE (LV TIMER)
   ========================= */
void ui_update_timer_callback(lv_timer_t *timer)
{

	// Check for view cycling timeouts to prevent stuck states
	extern void view_state_check_timeout(void);
	view_state_check_timeout();


	// Update all display modules (data only, no UI structure changes)
	display_modules_update_all();

	// View transitions are now handled by individual modules

	// Handle screen transitions using screen manager
	screen_manager_update();

	// Note: Module updates are now handled by screen update functions to avoid double updates
}

/* =========================
   DATA PRODUCER TASK
   ========================= */
static void data_task(void *arg)
{


	while (1) {
		if (data_config_get_source() == DATA_SOURCE_MOCK) {
			mock_data_update();
			mock_data_write_to_state_objects();
		} else {
			real_data_update();
			real_data_write_to_state_objects();
		}

		// If you need to nudge UI without touching LVGL from here:
		// (optional) lv_async_call(some_lightweight_cb, user_data);

		// Minimal throttling for maximum performance
		esp_vTaskDelay(20); // 20ms delay for maximum responsiveness
	}
}

/* =========================
   APP MAIN
   ========================= */
void app_main(void)
{
	ESP_LOGI(TAG, "Starting Jeep Sensor Hub UI");

	// 1) Init LVGL port for Pi (starts LVGL tasks & tick internally)
	int ret = lvgl_port_init();
	if (ret != 0) {
		ESP_LOGE(TAG, "Failed to initialize LVGL display: %s", esp_err_to_name(ret));
		return;
	}
	ESP_LOGI(TAG, "LVGL display initialized successfully");


	// 2) Device/data init
	device_state_init();
	data_config_init();

	// Choose your data source
	data_config_set_source(DATA_SOURCE_MOCK);   // or DATA_SOURCE_REAL

	if (data_config_get_source() == DATA_SOURCE_MOCK) {
		mock_data_init();
		mock_data_enable(true);  // Enable mock data updates
		ESP_LOGI(TAG, "Mock data component initialized");
	} else {
		real_data_init();
		ESP_LOGI(TAG, "Real data component initialized");
	}

	// 3) Display modules (create LVGL objs → lock per call)
	ESP_LOGI(TAG, "Initializing display modules");

	// Initialize all display modules via standardized interface
	display_modules_init_all();

	ESP_LOGI(TAG, "Display modules initialized");

	// 4) Display ready (no backlight control needed for SDL)
	ESP_LOGI(TAG, "Display ready");

	// 5) Boot screen
	ESP_LOGI(TAG, "Initializing screens and boot screen");

	boot_screen_init();
	boot_screen_update_progress(100);
	boot_screen_cleanup();

	// 6) Init screen manager (creates Home/Detail etc.)
	screen_manager_init();
	ESP_LOGI(TAG, "Screens initialized successfully");

	// 7) Create LVGL UI update timer (runs in LVGL context)
	lv_timer_t *ui_timer = lv_timer_create(ui_update_timer_callback, 8, NULL); // 8ms (120 FPS) for maximum performance
	lv_timer_set_repeat_count(ui_timer, -1);
	ESP_LOGI(TAG, "UI update timer created");

	// 8) Start data producer task (no LVGL calls inside)
	esp_xTaskCreatePinnedToCore(data_task, "data_task", 8192, NULL, 3, NULL, 0);

	// app_main can return, or sleep forever if you prefer:
	// esp_vTaskDelete(NULL);
}

/* =========================
   MAIN FUNCTION (Linux)
   ========================= */
int main(int argc, char *argv[])
{
	// Initialize the application
	app_main();

	// Start the main event loop to keep the window alive and handle events
	lvgl_port_main_loop();

	return 0;
}
