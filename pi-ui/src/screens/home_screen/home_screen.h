#ifndef HOME_SCREEN_H
#define HOME_SCREEN_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// External access to home container for detail views
extern lv_obj_t *home_container;

void home_screen_init(void);
void home_screen_update_status(const char *status);
void home_screen_update_modules(void);
void home_screen_update_module_data(void);
void home_screen_hide_modules(void);  // Actually hides entire home screen
void home_screen_show_modules(void);  // Actually shows entire home screen
void home_screen_update_context_panel(const char *connection_status, const char *signal_type, const char *telemetry);
void home_screen_cleanup(void);

// Screen manager wrapper functions
void home_screen_show(void);
void home_screen_destroy(void);
lv_obj_t* get_power_monitor_container(void);

#ifdef __cplusplus
}
#endif

#endif // HOME_SCREEN_H
