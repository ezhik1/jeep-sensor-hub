/*
 * LVGL Port for Raspberry Pi using SDL2
 * Updated for LVGL v9 API
 */
#include "lvgl_port_pi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <SDL2/SDL.h>
#include <lvgl.h>
#include "drivers/evdev/lv_evdev.h"

static const char *TAG = "lv_port_pi";

// Logical resolution for LVGL (portrait)
#define LVGL_HOR_RES 480
#define LVGL_VER_RES 800

// Physical display resolution (landscape)
#define DISP_HOR_RES 800
#define DISP_VER_RES 480

// Global flag to control main loop
static volatile bool running = true;

// FPS tracking variables
static uint32_t frame_count = 0;
static float current_fps = 0.0f;
static bool show_fps = true;
static lv_obj_t *fps_label = NULL;
uint32_t last_tick;

// FPS calculation timer callback
static void fps_timer_cb(lv_timer_t *timer)
{
	uint32_t now = SDL_GetTicks();

	// Update FPS every second
	if (now - last_tick >= 1000) {

		current_fps = (float)frame_count * 1000.0f / (now - last_tick);
		frame_count = 0;
		last_tick = now;
	}
}

// Signal handler for graceful shutdown
static void signal_handler(int sig)
{
	printf("\nReceived signal %d, shutting down gracefully...\n", sig);
	running = false;
}

// Function to create FPS label on screen
static void create_fps_label(void)
{
	if (fps_label) return; // Already created

	// Create FPS label
	fps_label = lv_label_create(lv_scr_act());
	lv_label_set_text(fps_label, "FPS: 0.0");
	lv_obj_set_style_text_color(fps_label, lv_color_hex(0xFFFFFF), 0);
	lv_obj_set_style_text_font(fps_label, &lv_font_montserrat_14, 0);
	lv_obj_set_style_bg_color(fps_label, lv_color_hex(0x000000), 0);
	lv_obj_set_style_bg_opa(fps_label, LV_OPA_80, 0);
	lv_obj_set_style_pad_all(fps_label, 8, 0);
	lv_obj_set_style_radius(fps_label, 4, 0);
	lv_obj_align(fps_label, LV_ALIGN_TOP_RIGHT, -10, 10);
	lv_obj_add_flag(fps_label, LV_OBJ_FLAG_FLOATING); // Float above other content
	lv_obj_add_flag(fps_label, LV_OBJ_FLAG_IGNORE_LAYOUT); // Ignore layout constraints
	lv_obj_clear_flag(fps_label, LV_OBJ_FLAG_CLICKABLE);
	lv_obj_move_foreground(fps_label); // Move to front
}

// Function to update FPS display on screen
static void update_fps_display(void)
{
	if (!show_fps || !fps_label) return;

	char fps_text[32];
	snprintf(fps_text, sizeof(fps_text), "FPS: %.1f", current_fps);
	lv_label_set_text(fps_label, fps_text);
	lv_obj_move_foreground(fps_label); // Ensure it stays on top
}
#define DISP_BUF_SIZE (LVGL_HOR_RES * LVGL_VER_RES)

// Global display dimensions
static uint32_t g_display_width = LVGL_HOR_RES;
static uint32_t g_display_height = LVGL_VER_RES;

// Buffers
static lv_color_t buf_1[DISP_BUF_SIZE];
static lv_color_t buf_2[DISP_BUF_SIZE];

// Display and input device handles
static lv_display_t *disp;
static lv_indev_t *indev;

// SDL variables
static SDL_Window *window;
static SDL_Renderer *renderer;
static SDL_Texture *texture;

// Mouse state for input
static int32_t mouse_x = 0;
static int32_t mouse_y = 0;
static bool mouse_pressed = false;


// Display flush callback + Software Rotation
static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
	frame_count++;

	// Update the entire LVGL portrait texture (480x800)
	// Pitch = width in pixels * sizeof(lv_color_t) = 480 * 2 bytes for RGB565
	if (SDL_UpdateTexture(texture, NULL, px_map, LVGL_HOR_RES * 2) != 0) {

		printf("SDL_UpdateTexture failed: %s\n", SDL_GetError());
	}

	SDL_RenderClear(renderer);

	// Destination rectangle: width and height swapped relative to the texture
	// to account for 90° clockwise rotation
	SDL_Rect destinantion_rectangle = { 0, 0, DISP_VER_RES, DISP_HOR_RES}; // 800x480


	// Need dynamic offsets based on actual texture size vs renderer output size:
	int renderer_output_width, renderer_output_height;

	SDL_GetRendererOutputSize( renderer, &renderer_output_width, &renderer_output_height );

	// Center the texture in the renderer output

	// SDL_RenderCopyEx rotates the texture around the center of destination_rectangle.
	// That means if destinantion_rectangle.x=0 and destinantion_rectangle.y=0,
	// the rotated texture is pushed down and to the right because the center pivot moves
	// the top-left corner. To center it in the renderer, you offset destination_rectangle
	//  so that after rotation, it aligns perfectly:

	destinantion_rectangle.x = ( renderer_output_width - destinantion_rectangle.w ) / 2;
	destinantion_rectangle.y = ( renderer_output_height - destinantion_rectangle.h ) / 2;

	// Rotate the texture 90° clockwise to fill the physical screen
	if( SDL_RenderCopyEx( renderer, texture, NULL, &destinantion_rectangle, 90, NULL, SDL_FLIP_NONE ) != 0 ){

		//  rotation failed
		printf("SDL_RenderCopyEx failed: %s\n", SDL_GetError());
	}

	SDL_RenderPresent(renderer);

	// Tell LVGL we're done flushing
	lv_display_flush_ready( disp );
}

// Input device read callback
static void indev_read(lv_indev_t * indev, lv_indev_data_t * data)
{
	data->point.x = mouse_x;
	data->point.y = mouse_y;
	data->state = mouse_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

// Initialize LVGL for Raspberry Pi
int lvgl_port_init(void)
{

	printf("Initializing LVGL for Raspberry Pi (logical %dx%d -> physical %dx%d)...\n",
		LVGL_HOR_RES, LVGL_VER_RES, DISP_HOR_RES, DISP_VER_RES);

	// Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Failed to initialize SDL: %s\n", SDL_GetError());
		return -1;
	}

	// Create SDL window - MUST match framebuffer dimensions (800x480) for kernel rotation
	window = SDL_CreateWindow("Jeep Sensor Hub - Raspberry Pi",
							  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
							  DISP_HOR_RES, DISP_VER_RES,
							  SDL_WINDOW_SHOWN);
	if (!window) {
		printf("Failed to create SDL window: %s\n", SDL_GetError());
		return -1;
	}

	// Create SDL renderer with maximum performance settings
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	if (!renderer) {
		printf("Failed to create SDL renderer: %s\n", SDL_GetError());
		return -1;
	}

	SDL_SetHint(SDL_HINT_VIDEODRIVER, "KMSDRM");
	SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	// Optimize SDL renderer for maximum performance
	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE); // Disable blending for speed
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // Nearest neighbor scaling for speed
	SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0"); // Disable VSYNC for maximum FPS
	SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl"); // Force OpenGL driver
	SDL_SetHint(SDL_HINT_FRAMEBUFFER_ACCELERATION, "1"); // Enable framebuffer acceleration
	SDL_SetHint(SDL_HINT_RENDER_OPENGL_SHADERS, "1"); // Enable OpenGL shaders

	// Create SDL texture - MUST match framebuffer dimensions (800x480) for kernel rotation
	texture = SDL_CreateTexture(
		renderer, SDL_PIXELFORMAT_RGB565,
		SDL_TEXTUREACCESS_STREAMING,
		DISP_VER_RES, DISP_HOR_RES
	);

	printf("Texture created: %dx%d\n", LVGL_HOR_RES, LVGL_VER_RES);

	if (!texture) {
		printf("Failed to create SDL texture: %s\n", SDL_GetError());
		return -1;
	}

	// Initialize LVGL
	lv_init();

	// Create display
	disp = lv_display_create(LVGL_HOR_RES, LVGL_VER_RES);
	lv_display_set_flush_cb(disp, disp_flush);
	lv_display_set_buffers(disp, buf_1, buf_2, sizeof(buf_1), LV_DISPLAY_RENDER_MODE_FULL);
	lv_display_set_default(disp);

	lv_indev_t *touch = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event7");
	if (touch == NULL) {
		printf("ERROR: Failed to create evdev input device\n");
		return -1;
	}
	printf("Touch input device created successfully\n");
	lv_indev_set_display(touch, disp);

	// Touchscreen is 800x480 in landscape; display is 480x800 portrait.
	// Swap axes and calibrate raw range to display coordinates.
	lv_evdev_set_swap_axes(touch, true);
	// After swapping axes: X comes from raw Y (0..479), Y comes from raw X (0..799)
	// Invert Y to map top-left to (0,0)
	lv_evdev_set_calibration(touch, 0, 799, 479, 0);

	indev = touch;

	lv_indev_enable(indev, true);
	printf("Touch input device enabled\n");


	printf("LVGL initialized successfully (logical %dx%d -> texture %dx%d)\n",
		LVGL_HOR_RES, LVGL_VER_RES, LVGL_HOR_RES, LVGL_VER_RES);
	return 0;
}

// Main render loop
void lvgl_port_main_loop(void)
{
	printf("Starting main loop...\n");

	// Register signal handlers for graceful shutdown
	signal(SIGINT, signal_handler);   // Ctrl+C
	signal(SIGTERM, signal_handler);  // kill command

	// Create FPS calculation timer (runs every 10ms for maximum responsiveness)
	lv_timer_t *fps_timer = lv_timer_create(fps_timer_cb, 10, NULL);
	lv_timer_set_repeat_count(fps_timer, -1);

	// Create FPS label on screen
	create_fps_label();

	printf("FPS HUD enabled! Press 'F' to toggle FPS display, 'ESC' to exit\n");

	while (running) {

		// Calculate elapsed ms since last iteration
		uint32_t now = SDL_GetTicks();
		uint32_t elapsed = now - last_tick;
		last_tick = now;

		// Handle SDL events (including keyboard input)
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				case SDL_QUIT:
					printf("SDL_QUIT event received\n");
					running = false;
					break;
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE) {
						printf("Escape key pressed, exiting...\n");
						running = false;
					} else if (event.key.keysym.sym == SDLK_f) {
						// Toggle FPS display with 'F' key
						show_fps = !show_fps;
						if (fps_label) {
							if (show_fps) {
								lv_obj_clear_flag(fps_label, LV_OBJ_FLAG_HIDDEN);
							} else {
								lv_obj_add_flag(fps_label, LV_OBJ_FLAG_HIDDEN);
							}
						}
						printf("\nFPS display %s\n", show_fps ? "enabled" : "disabled");
					}
					break;
			}
		}

		// Increment LVGL tick by the actual time elapsed
		lv_tick_inc(elapsed);

		// Handle LVGL tasks
		lv_timer_handler();

		// Update FPS display on screen
		update_fps_display();
	}

	printf("Main loop exiting gracefully...\n");
}

// Deinitialize LVGL
void lvgl_port_deinit(void)
{
	if (texture) {
		SDL_DestroyTexture(texture);
		texture = NULL;
	}
	if (renderer) {
		SDL_DestroyRenderer(renderer);
		renderer = NULL;
	}
	if (window) {
		SDL_DestroyWindow(window);
		window = NULL;
	}
	SDL_Quit();
}

// Get display dimensions
void lvgl_port_get_display_size(uint32_t *width, uint32_t *height)
{
	if (width) *width = g_display_width;
	if (height) *height = g_display_height;
}

// Set display dimensions
void lvgl_port_set_display_size(uint32_t width, uint32_t height)
{
	g_display_width = width;
	g_display_height = height;
}

// Get the active display object
lv_display_t* lvgl_port_get_display(void)
{
	return disp;
}

// Force all screens to use the correct dimensions
void lvgl_port_force_screen_dimensions(lv_obj_t *screen)
{
	if (screen) {
		lv_obj_set_size(screen, g_display_width, g_display_height);
	}
}

