#include "app_data_store.h"
#include "displayModules/power-monitor/power-monitor.h"
#include "data/config.h"
#include "data/mock_data/mock_data.h"
#include "data/lerp_data/lerp_data.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static const char *TAG = "app_data_store";

// Global app data store instance
static app_data_store_t g_app_data_store = {0};
static bool g_initialized = false;

void app_data_store_init(void)
{
	if (g_initialized) {
		printf("[W] %s: Already initialized\n", TAG);
		return;
	}

	printf("[I] %s: Initializing app data store\n", TAG);

	// Allocate power monitor data
	g_app_data_store.power_monitor = calloc(1, sizeof(power_monitor_data_t));
	if (!g_app_data_store.power_monitor) {
		printf("[E] %s: Failed to allocate power monitor data\n", TAG);
		return;
	}

	// Initialize persistent gauge histories
	memset(
		g_app_data_store.power_monitor_gauge_histories, 0,
		sizeof(g_app_data_store.power_monitor_gauge_histories)
	);

	// Initialize other module data here as needed

	g_initialized = true;
	printf("[I] %s: App data store initialized\n", TAG);
}

void app_data_store_update(void)
{
	if (!g_initialized) {
		return;
	}

	// Update power monitor data from mock/real data source
	if (data_config_get_source() == DATA_SOURCE_MOCK) {
		mock_data_write_to_state_objects();
	}

	// Update LERP data system with current power data
	if (g_app_data_store.power_monitor) {
		lerp_data_set_targets(g_app_data_store.power_monitor);
		lerp_data_update();
	}

	// Update other module data here
}

void app_data_store_cleanup(void)
{
	if (!g_initialized) {
		return;
	}

	printf("[I] %s: Cleaning up app data store\n", TAG);

	// Free power monitor data
	if (g_app_data_store.power_monitor) {
		free(g_app_data_store.power_monitor);
		g_app_data_store.power_monitor = NULL;
	}

	// Free other module data here

	memset(&g_app_data_store, 0, sizeof(app_data_store_t));
	g_initialized = false;
	printf("[I] %s: App data store cleanup complete\n", TAG);
}

app_data_store_t* app_data_store_get(void)
{
	return &g_app_data_store;
}


