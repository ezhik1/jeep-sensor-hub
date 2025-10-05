#include "lvgl_port_pi.h"
#include "screens/screen_manager.h"
#include "screens/home_screen/home_screen.h"
#include "screens/boot_screen/boot_screen.h"

#include "state/device_state.h"
#include "displayModules/shared/module_interface.h"

#include "data/config.h"
#include "data/mock_data/mock_data.h"
#include "data/real_data/real_data.h"

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

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
		usleep(20000); // 20ms delay for maximum responsiveness
	}
}

/* =========================
   APP MAIN
   ========================= */
void app_main(void)
{
	printf("[I] main: Starting Jeep Sensor Hub UI\n");

	// 1) Init LVGL port for Pi (starts LVGL tasks & tick internally)
	int ret = lvgl_port_init();
	if (ret != 0) {
		printf("[E] main: Failed to initialize LVGL display: %d\n", ret);
		return;
	}
	printf("[I] main: LVGL display initialized successfully\n");


	// 2) Device/data init
	device_state_init();
	data_config_init();

	// Choose your data source
	data_config_set_source(DATA_SOURCE_MOCK);   // or DATA_SOURCE_REAL

	if (data_config_get_source() == DATA_SOURCE_MOCK) {
		mock_data_init();
		mock_data_enable(true);  // Enable mock data updates
		printf("[I] main: Mock data component initialized\n");
	} else {
		real_data_init();
		printf("[I] main: Real data component initialized\n");
	}

	// 3) Display modules (create LVGL objs → lock per call)
	printf("[I] main: Initializing display modules\n");

	// Initialize all display modules via standardized interface
	display_modules_init_all();

	printf("[I] main: Display modules initialized\n");

	// 4) Display ready (no backlight control needed for SDL)
	printf("[I] main: Display ready\n");

	// 5) Boot screen
	printf("[I] main: Initializing screens and boot screen\n");

	boot_screen_init();
	boot_screen_update_progress(100);
	boot_screen_cleanup();

	// 6) Init screen manager (creates Home/Detail etc.)
	screen_manager_init();
	printf("[I] main: Screens initialized successfully\n");

	// 7) Create LVGL UI update timer (runs in LVGL context)
	lv_timer_t *ui_timer = lv_timer_create(ui_update_timer_callback, 8, NULL); // 8ms (120 FPS) for maximum performance
	lv_timer_set_repeat_count(ui_timer, -1);
	printf("[I] main: UI update timer created\n");

	// 8) Start data producer task (no LVGL calls inside)
	pthread_t data_thread;
	pthread_create(&data_thread, NULL, (void*)data_task, NULL);
	pthread_detach(data_thread);

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
