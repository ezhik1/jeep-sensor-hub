#include "device_state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

// Global device state
static cJSON *g_root = NULL;

// State file path - use persistent location
// Try XDG_DATA_HOME first, then fall back to home directory
static char state_file_path[256] = {0};

static const char* get_state_file_path(void) {
	if (state_file_path[0] == '\0') {
		const char* xdg_data_home = getenv("XDG_DATA_HOME");
		if (xdg_data_home && strlen(xdg_data_home) > 0) {
			snprintf(state_file_path, sizeof(state_file_path), "%s/jeep_sensor_hub_state.json", xdg_data_home);
		} else {
			const char* home = getenv("HOME");
			if (home && strlen(home) > 0) {
				snprintf(state_file_path, sizeof(state_file_path), "%s/.local/share/jeep_sensor_hub_state.json", home);
			} else {
				// Fallback to current directory
				snprintf(state_file_path, sizeof(state_file_path), "./jeep_sensor_hub_state.json");
			}
		}
	}
	return state_file_path;
}

// Ensure the directory for the state file exists
static void ensure_state_directory_exists(void) {
	const char* path = get_state_file_path();
	char* dir_path = strdup(path);
	if (!dir_path) return;

	// Find the last '/' and truncate to get directory path
	char* last_slash = strrchr(dir_path, '/');
	if (last_slash) {
		*last_slash = '\0';

		// Create directory with 0755 permissions (rwxr-xr-x)
		if (mkdir(dir_path, 0755) != 0 && errno != EEXIST) {
			printf("[W] device_state: Failed to create directory %s: %s\n", dir_path, strerror(errno));
		}
	}

	free(dir_path);
}

// Initialize device state with default values
void device_state_init(void) {
	printf("[I] device_state: Initializing device state\n");

	// Create root JSON object
	g_root = cJSON_CreateObject();
	if (!g_root) {
		printf("[E] device_state: Failed to create root JSON object\n");
		return;
	}

	// Device state should only contain generic structure
	// Module-specific defaults belong in their respective modules

	// Initialize global settings
	cJSON *global = cJSON_CreateObject();
	if (global) {
		cJSON_AddNumberToObject(global, "brightness_level", 100);
		cJSON_AddBoolToObject(global, "auto_save_enabled", true);
		cJSON_AddNumberToObject(global, "auto_save_interval_ms", 5000);
		cJSON_AddItemToObject(g_root, "global", global);
	}

	// Initialize system settings
	cJSON *system = cJSON_CreateObject();
	if (system) {
		cJSON_AddBoolToObject(system, "initialized", true);
		cJSON_AddNumberToObject(system, "last_save_timestamp", (uint32_t)time(NULL));
		cJSON_AddItemToObject(g_root, "system", system);
	}

	// Load existing state from file
	device_state_load();
}

// Cleanup device state
void device_state_cleanup(void) {
	printf("[I] device_state: Cleaning up JSON device state\n");
	if (g_root) {
		cJSON_Delete(g_root);
		g_root = NULL;
	}
}

// Generic JSON path parser - handles any namespace structure
static cJSON* get_object_by_path(const char* path, bool create_if_missing) {
	if (!g_root || !path) return NULL;

	char path_copy[256];
	strncpy(path_copy, path, sizeof(path_copy) - 1);
	path_copy[sizeof(path_copy) - 1] = '\0';

	cJSON* current = g_root;
	char* token = strtok(path_copy, ".");

	while (token != NULL) {
		// Check if this token has array indexing [index]
		char* bracket = strchr(token, '[');
		if (bracket) {
			*bracket = '\0'; // Null-terminate the array name
			int index = atoi(bracket + 1);

			cJSON* array = cJSON_GetObjectItemCaseSensitive(current, token);
			if (!array) {
				if (create_if_missing) {
					array = cJSON_AddArrayToObject(current, token);
					if (!array) return NULL;
				} else {
					return NULL;
				}
			}

			if (!cJSON_IsArray(array)) return NULL;

			// Ensure array has enough elements only if creating
			if (create_if_missing) {
				while (cJSON_GetArraySize(array) <= index) {
					cJSON_AddItemToArray(array, cJSON_CreateObject());
				}
			} else {
				// If not creating, check if index exists
				if (cJSON_GetArraySize(array) <= index) {
					return NULL;
				}
			}

			current = cJSON_GetArrayItem(array, index);
		} else {
			// Regular object property
			cJSON* next = cJSON_GetObjectItemCaseSensitive(current, token);
			if (!next) {
				if (create_if_missing) {
					next = cJSON_AddObjectToObject(current, token);
					if (!next) return NULL;
				} else {
					return NULL;
				}
			}
			current = next;
		}

		token = strtok(NULL, ".");
	}

	return current;
}

// Generic get/set implementation
int device_state_get_int(const char* path) {
	cJSON* obj = get_object_by_path(path, false);
	if (obj && cJSON_IsNumber(obj)) {
		return obj->valueint;
	}
	return 0;
}

float device_state_get_float(const char* path) {
	cJSON* obj = get_object_by_path(path, false);
	if (obj && cJSON_IsNumber(obj)) {
		return (float)obj->valuedouble;
	}
	return 0.0f;
}

bool device_state_get_bool(const char* path) {
	cJSON* obj = get_object_by_path(path, false);
	if (obj && cJSON_IsBool(obj)) {
		return cJSON_IsTrue(obj);
	}
	return false;
}

void device_state_set_int(const char* path, int value) {
	cJSON* obj = get_object_by_path(path, true);
	if (obj) {
		// Directly set the value
		obj->valueint = value;
		obj->valuedouble = (double)value;
		obj->type = cJSON_Number;
		// Save immediately
		device_state_save();
	}
}

void device_state_set_float(const char* path, float value) {
	cJSON* obj = get_object_by_path(path, true);
	if (obj) {
		cJSON_SetNumberValue(obj, value);
		// Save immediately
		device_state_save();
	}
}

void device_state_set_bool(const char* path, bool value) {
	cJSON* obj = get_object_by_path(path, true);
	if (obj) {
		cJSON_SetBoolValue(obj, value);
		// Save immediately
		device_state_save();
	}
}

// Check if a path exists without triggering saves
bool device_state_path_exists(const char* path) {
	cJSON* obj = get_object_by_path(path, false);
	return (obj != NULL);
}

// Generic setter that handles both int and float automatically
void device_state_set_value(const char* path, double value) {
	cJSON* obj = get_object_by_path(path, true);
	if (obj) {
		// Set the value directly
		obj->valuedouble = value;
		obj->valueint = (int)value;
		obj->type = cJSON_Number;
		// Save immediately since this is the main setter
	device_state_save();
	}
}

void device_state_set_float_array(const char* path, const float* values, int count, bool save_now) {
	if (!path || !values || count < 0) return;
	// Split path into parent path and final key (no array index handling needed here)
	char parent_path[256] = {0};
	char leaf_key[128] = {0};
	size_t len = strlen(path);
	const char* last_dot = strrchr(path, '.');
	if (last_dot) {
		size_t parent_len = (size_t)(last_dot - path);
		if (parent_len >= sizeof(parent_path)) parent_len = sizeof(parent_path) - 1;
		memcpy(parent_path, path, parent_len);
		parent_path[parent_len] = '\0';
		strncpy(leaf_key, last_dot + 1, sizeof(leaf_key) - 1);
	} else {
		// No dot, place array at root with this key
		parent_path[0] = '\0';
		strncpy(leaf_key, path, sizeof(leaf_key) - 1);
	}

	cJSON* parent = NULL;
	if (parent_path[0] == '\0') {
		parent = g_root;
	} else {
		parent = get_object_by_path(parent_path, true);
	}
	if (!parent || !leaf_key[0]) return;

	// Remove old value at leaf_key if exists, then set array
	cJSON_DeleteItemFromObjectCaseSensitive(parent, leaf_key);
	cJSON* arr = cJSON_CreateArray();
	if (!arr) return;
	for (int i = 0; i < count; i++) {
		cJSON_AddItemToArray(arr, cJSON_CreateNumber(values[i]));
	}
	cJSON_AddItemToObject(parent, leaf_key, arr);
	if (save_now) device_state_save();
}

int device_state_get_float_array(const char* path, float* out_values, int max_count) {
	cJSON* obj = get_object_by_path(path, false);
	if (!obj || !cJSON_IsArray(obj)) return 0;
	int n = cJSON_GetArraySize(obj);
	if (n > max_count) n = max_count;
	for (int i = 0; i < n; i++) {
		cJSON* it = cJSON_GetArrayItem(obj, i);
		out_values[i] = (float)(cJSON_IsNumber(it) ? it->valuedouble : 0.0);
	}
	return n;
}

// Save device state to JSON file
void device_state_save(void) {
	if (!g_root) {
		printf("[W] device_state: Cannot save - not initialized\n");
		return;
	}

	// Ensure directory exists
	ensure_state_directory_exists();

	// Update last save timestamp (directly to avoid recursive save calls)
	cJSON* system_obj = cJSON_GetObjectItemCaseSensitive(g_root, "system");
	if (system_obj && cJSON_IsObject(system_obj)) {
		cJSON* timestamp_obj = cJSON_GetObjectItemCaseSensitive(system_obj, "last_save_timestamp");
		if (timestamp_obj && cJSON_IsNumber(timestamp_obj)) {
			cJSON_SetNumberValue(timestamp_obj, (double)time(NULL));
		}
	}

	// Write to file
	char *json_string = cJSON_Print(g_root);
	if (!json_string) {
		printf("[E] device_state: Failed to convert JSON to string\n");
		return;
	}

	FILE *file = fopen(get_state_file_path(), "w");
	if (!file) {
		printf("[E] device_state: Failed to open state file for writing: %s\n", get_state_file_path());
		free(json_string);
		return;
	}

	fwrite(json_string, strlen(json_string), 1, file);
	fclose(file);

	free(json_string);
}

// Load device state from JSON file
void device_state_load(void) {
	if (!g_root) {
		printf("[W] device_state: Cannot load - not initialized\n");
		return;
	}

	printf("[I] device_state: Loading device state from JSON file\n");

	FILE *file = fopen(get_state_file_path(), "r");
	if (!file) {
		printf("[W] device_state: State file not found, using defaults: %s\n", get_state_file_path());
		return;
	}

	// Get file size
	fseek(file, 0, SEEK_END);
	long file_size = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (file_size <= 0) {
		printf("[W] device_state: State file is empty, using defaults\n");
		fclose(file);
		return;
	}

	// Read file content
	char *json_string = malloc(file_size + 1);
	if (!json_string) {
		printf("[E] device_state: Failed to allocate memory for JSON string\n");
		fclose(file);
		return;
	}

	fread(json_string, 1, file_size, file);
	json_string[file_size] = '\0';
	fclose(file);

	// Parse JSON
	cJSON *loaded_root = cJSON_Parse(json_string);
	free(json_string);

	if (!loaded_root) {
		printf("[E] device_state: Failed to parse JSON: %s\n", cJSON_GetErrorPtr());
		return;
	}

	// Replace our root with the loaded data
	cJSON_Delete(g_root);
	g_root = loaded_root;

	printf("[I] device_state: Device state loaded successfully\n");
}


