#ifndef ANIMATION_H
#define ANIMATION_H

#include <lvgl.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Animation configuration
typedef struct {
	float duration;          // Animation duration in seconds
	uint32_t frame_rate;     // Target frame rate (ms per frame)
} animation_config_t;

// Animation state for a single value
typedef struct {
	float current_value;     // Current animated value
	float target_value;      // Target value to animate to
	float start_value;       // Starting value for current animation
	uint32_t start_time;     // Animation start timestamp
	bool is_animating;       // Whether this value is currently animating
} animation_state_t;

// Animation manager
typedef struct {
	lv_timer_t* timer;       // Animation timer
	animation_state_t* states; // Array of animation states
	int state_count;         // Number of animation states
	animation_config_t config; // Animation configuration
	void* user_data;         // User data passed to callback
	void (*on_value_changed)(int index, float value, void* user_data); // Callback for value changes
} animation_manager_t;

/**
 * @brief Create a new animation manager
 * @param state_count Number of values to animate
 * @param config Animation configuration
 * @param on_value_changed Callback function called when values change
 * @param user_data User data passed to callback
 * @return Animation manager instance or NULL on failure
 */
animation_manager_t* animation_manager_create(int state_count, const animation_config_t* config,
											  void (*on_value_changed)(int index, float value, void* user_data),
											  void* user_data);

/**
 * @brief Destroy an animation manager
 * @param manager Animation manager to destroy
 */
void animation_manager_destroy(animation_manager_t* manager);

/**
 * @brief Animate a value to a target
 * @param manager Animation manager
 * @param index Index of the value to animate
 * @param target_value Target value to animate to
 */
void animation_manager_animate_to(animation_manager_t* manager, int index, float target_value);

/**
 * @brief Set a value immediately without animation
 * @param manager Animation manager
 * @param index Index of the value to set
 * @param value Value to set
 */
void animation_manager_set_value(animation_manager_t* manager, int index, float value);

/**
 * @brief Get current value
 * @param manager Animation manager
 * @param index Index of the value to get
 * @return Current value
 */
float animation_manager_get_value(animation_manager_t* manager, int index);

/**
 * @brief Check if any values are currently animating
 * @param manager Animation manager
 * @return True if any values are animating
 */
bool animation_manager_is_animating(animation_manager_t* manager);

/**
 * @brief Stop all animations
 * @param manager Animation manager
 */
void animation_manager_stop_all(animation_manager_t* manager);

#ifdef __cplusplus
}
#endif

#endif // ANIMATION_H
