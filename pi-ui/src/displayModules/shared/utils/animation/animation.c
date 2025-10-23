#include "animation.h"
#include <stdlib.h>
#include <string.h>

// Animation timer callback
static void animation_timer_cb(lv_timer_t* timer)
{
	animation_manager_t* manager = (animation_manager_t*)lv_timer_get_user_data(timer);
	if (!manager) return;

	uint32_t current_time = lv_tick_get();
	bool any_animating = false;

	// Update all animating values
	for (int i = 0; i < manager->state_count; i++) {
		if (manager->states[i].is_animating) {
			uint32_t elapsed = current_time - manager->states[i].start_time;
			float progress = (float)elapsed / (manager->config.duration * 1000.0f);

			if (progress >= 1.0f) {
				// Animation complete
				manager->states[i].current_value = manager->states[i].target_value;
				manager->states[i].is_animating = false;
			} else {
				// Lerp between start and target values
				manager->states[i].current_value = manager->states[i].start_value +
					(manager->states[i].target_value - manager->states[i].start_value) * progress;
			}

			// Call user callback
			if (manager->on_value_changed) {
				manager->on_value_changed(i, manager->states[i].current_value, manager->user_data);
			}

			any_animating = true;
		}
	}

	// Stop timer if no values are animating
	if (!any_animating) {
		lv_timer_del(timer);
		manager->timer = NULL;
	}
}

// Create animation manager
animation_manager_t* animation_manager_create(int state_count, const animation_config_t* config,
											  void (*on_value_changed)(int index, float value, void* user_data),
											  void* user_data)
{
	if (state_count <= 0 || !config) return NULL;

	animation_manager_t* manager = malloc(sizeof(animation_manager_t));
	if (!manager) return NULL;

	memset(manager, 0, sizeof(animation_manager_t));

	// Allocate animation states
	manager->states = calloc(state_count, sizeof(animation_state_t));
	if (!manager->states) {
		free(manager);
		return NULL;
	}

	// Initialize
	manager->state_count = state_count;
	manager->config = *config;
	manager->on_value_changed = on_value_changed;
	manager->user_data = user_data;

	// Initialize all states
	for (int i = 0; i < state_count; i++) {
		manager->states[i].current_value = 0.0f;
		manager->states[i].target_value = 0.0f;
		manager->states[i].start_value = 0.0f;
		manager->states[i].start_time = 0;
		manager->states[i].is_animating = false;
	}

	return manager;
}

// Destroy animation manager
void animation_manager_destroy(animation_manager_t* manager)
{
	if (!manager) return;

	// Stop and delete timer
	if (manager->timer) {
		lv_timer_del(manager->timer);
	}

	// Free states
	if (manager->states) {
		free(manager->states);
	}

	// Free manager
	free(manager);
}

// Animate a value to target
void animation_manager_animate_to(animation_manager_t* manager, int index, float target_value)
{
	if (!manager || index < 0 || index >= manager->state_count) return;

	// Set up animation
	manager->states[index].start_value = manager->states[index].current_value;
	manager->states[index].target_value = target_value;
	manager->states[index].start_time = lv_tick_get();
	manager->states[index].is_animating = true;

	// Start timer if not already running
	if (!manager->timer) {
		manager->timer = lv_timer_create(animation_timer_cb, manager->config.frame_rate, manager);
	}
}

// Set value immediately
void animation_manager_set_value(animation_manager_t* manager, int index, float value)
{
	if (!manager || index < 0 || index >= manager->state_count) return;

	manager->states[index].current_value = value;
	manager->states[index].target_value = value;
	manager->states[index].is_animating = false;

	// Call user callback
	if (manager->on_value_changed) {
		manager->on_value_changed(index, value, manager->user_data);
	}
}

// Get current value
float animation_manager_get_value(animation_manager_t* manager, int index)
{
	if (!manager || index < 0 || index >= manager->state_count) return 0.0f;
	return manager->states[index].current_value;
}

// Check if any values are animating
bool animation_manager_is_animating(animation_manager_t* manager)
{
	if (!manager) return false;

	for (int i = 0; i < manager->state_count; i++) {
		if (manager->states[i].is_animating) {
			return true;
		}
	}
	return false;
}

// Stop all animations
void animation_manager_stop_all(animation_manager_t* manager)
{
	if (!manager) return;

	for (int i = 0; i < manager->state_count; i++) {
		manager->states[i].is_animating = false;
	}

	// Stop timer
	if (manager->timer) {
		lv_timer_del(manager->timer);
		manager->timer = NULL;
	}
}
