/*
 * LVGL Port Header for Raspberry Pi
 * Replaces ESP32-specific LVGL port with Linux/SDL2 implementation
 */

#ifndef LVGL_PORT_PI_H
#define LVGL_PORT_PI_H

#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize LVGL for Raspberry Pi
int lvgl_port_init(void);

// Deinitialize LVGL
void lvgl_port_deinit(void);

// Main render loop
void lvgl_port_render_loop(void);

// Main event loop to keep window alive and handle events
void lvgl_port_main_loop(void);

// Get display dimensions
void lvgl_port_get_display_size(uint32_t *width, uint32_t *height);

// Set display dimensions (called during initialization)
void lvgl_port_set_display_size(uint32_t width, uint32_t height);

// Get the active display object
lv_display_t* lvgl_port_get_display(void);

// Force all screens to use the correct dimensions
void lvgl_port_force_screen_dimensions(lv_obj_t *screen);

// Thread synchronization functions
bool lvgl_port_lock(uint32_t timeout_ms);
void lvgl_port_unlock(void);

#ifdef __cplusplus
}
#endif

#endif // LVGL_PORT_PI_H
