#include "config.h"
#include "esp_compat.h"
#include <string.h>

static const char *TAG = "data_config";

// Global configuration
data_source_t g_data_source = DATA_SOURCE_MOCK; // Default to mock data

void data_config_init(void)
{
	ESP_LOGI(TAG, "Initializing data configuration");
	ESP_LOGI(TAG, "Data source: %s", data_config_get_source_name(g_data_source));
}

void data_config_set_source(data_source_t source)
{
	if (source >= DATA_SOURCE_COUNT) {
		ESP_LOGW(TAG, "Invalid data source: %d, keeping current: %s",
				 source, data_config_get_source_name(g_data_source));
		return;
	}

	data_source_t old_source = g_data_source;
	g_data_source = source;

	ESP_LOGI(TAG, "Data source changed from %s to %s",
			 data_config_get_source_name(old_source),
			 data_config_get_source_name(g_data_source));
}

data_source_t data_config_get_source(void)
{
	return g_data_source;
}

const char* data_config_get_source_name(data_source_t source)
{
	switch (source) {
		case DATA_SOURCE_MOCK:
			return "MOCK";
		case DATA_SOURCE_REAL:
			return "REAL";
		default:
			return "UNKNOWN";
	}
}
