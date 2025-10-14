#ifndef CURRENT_VIEW_MANAGER_H
#define CURRENT_VIEW_MANAGER_H

#include "lvgl.h"
#include "../../../state/device_state.h"

// Shared current view management for all modules
// This provides a clean, safe interface for managing current view state

// Initialize the current view system for a module
void current_view_manager_init(int available_views_count);

// View lifecycle management (separated from state management)
int current_view_manager_get_index(void);
int current_view_manager_get_count(void);
bool current_view_manager_is_visible(void);
void current_view_manager_set_visible(bool visible);

// State transition management (delegated to global state)
void current_view_manager_cycle_to_next(void);
bool current_view_manager_is_cycling_in_progress(void);
void current_view_manager_set_cycling_in_progress(bool in_progress);

// Clean up current view resources
void current_view_manager_cleanup(void);

// Missing function declarations - these need to be implemented
void current_view_initialize(int available_views_count);
void current_view_cleanup(void);
int module_screen_view_get_view_index(const char* module_name);
void module_screen_view_set_view_index(const char* module_name, int view_index);
void module_screen_view_cycle_to_next(const char* module_name);
bool module_screen_view_is_cycling_in_progress(const char* module_name);
void module_screen_view_set_cycling_in_progress(const char* module_name, bool in_progress);
int module_screen_view_get_views_count(const char* module_name);
bool module_screen_view_is_visible(const char* module_name);
void module_screen_view_set_visible(const char* module_name, bool visible);
void view_state_set_cycling_in_progress(bool in_progress);

#endif // CURRENT_VIEW_MANAGER_H
