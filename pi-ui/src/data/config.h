#ifndef DATA_CONFIG_H
#define DATA_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

// Data source configuration
typedef enum {
	DATA_SOURCE_MOCK = 0,
	DATA_SOURCE_REAL,
	DATA_SOURCE_COUNT
} data_source_t;

// Global configuration
extern data_source_t g_data_source;

// Configuration functions
void data_config_init(void);
void data_config_set_source(data_source_t source);
data_source_t data_config_get_source(void);
const char* data_config_get_source_name(data_source_t source);

#ifdef __cplusplus
}
#endif

#endif // DATA_CONFIG_H
