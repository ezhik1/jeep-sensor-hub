\
#ifndef ESP_COMPAT_H
#define ESP_COMPAT_H
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Error codes
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_NO_MEM -2
#define ESP_ERR_INVALID_ARG -3
#define ESP_ERR_INVALID_STATE -4
#define ESP_ERR_TIMEOUT -5

// Logging macros (map to printf/syslog)
void esp_log_printf(const char *level, const char *tag, const char *fmt, ...);
#define ESP_LOGI(tag, fmt, ...) esp_log_printf("I", tag, fmt, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) esp_log_printf("W", tag, fmt, ##__VA_ARGS__)
#define ESP_LOGE(tag, fmt, ...) esp_log_printf("E", tag, fmt, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) esp_log_printf("D", tag, fmt, ##__VA_ARGS__)

// Timer (microseconds since some epoch)
uint64_t esp_timer_get_time(void);
uint32_t xTaskGetTickCount(void);
const char* esp_err_to_name(int err);
int esp_timer_init(void);
int esp_timer_deinit(void);

// Task/Thread helpers
int esp_xTaskCreatePinnedToCore(void (*task_func)(void*), const char *name, unsigned int stack_size, void *param, unsigned int prio, void *task_handle, int core_id);
int esp_xTaskCreate(void (*task_func)(void*), const char *name, unsigned int stack_size, void *param, unsigned int prio, void *task_handle);
void esp_vTaskDelete(void *task_handle);
void esp_vTaskDelay(unsigned int ms);
void esp_vTaskDelayUntil(unsigned long *prev_wake_time, unsigned int ms);

// Semaphore/Queue stubs (basic)
void* esp_xQueueCreate(unsigned int uxQueueLength, unsigned int uxItemSize);
int esp_xQueueSend(void *queue, const void *item, unsigned int ticks_to_wait);
int esp_xQueueReceive(void *queue, void *buffer, unsigned int ticks_to_wait);

// Critical section stubs
void esp_portENTER_CRITICAL(void);
void esp_portEXIT_CRITICAL(void);

// LVGL port lock/unlock (mutex)
int lvgl_port_lock(int wait_ms);
void lvgl_port_unlock(void);

// NVS (Non-Volatile Storage) stubs for Pi
typedef void* nvs_handle_t;
typedef void* TimerHandle_t;
typedef void* TaskHandle_t;

#define NVS_READONLY 0
#define NVS_READWRITE 1
#define ESP_ERR_NVS_NO_FREE_PAGES -6
#define ESP_ERR_NVS_NEW_VERSION_FOUND -7
#define pdTRUE 1
#define pdFALSE 0
#define tskNO_AFFINITY -1

int nvs_flash_init(void);
int nvs_flash_erase(void);
int nvs_open(const char* name, int open_mode, nvs_handle_t* handle);
int nvs_set_i32(nvs_handle_t handle, const char* key, int32_t value);
int nvs_set_u8(nvs_handle_t handle, const char* key, uint8_t value);
int nvs_set_u32(nvs_handle_t handle, const char* key, uint32_t value);
int nvs_get_i32(nvs_handle_t handle, const char* key, int32_t* value);
int nvs_get_u8(nvs_handle_t handle, const char* key, uint8_t* value);
int nvs_get_u32(nvs_handle_t handle, const char* key, uint32_t* value);
int nvs_commit(nvs_handle_t handle);
void nvs_close(nvs_handle_t handle);

// Timer stubs
TimerHandle_t xTimerCreate(const char* name, int period, int auto_reload, void* id, void (*callback)(TimerHandle_t));
int xTimerStart(TimerHandle_t timer, int block_time);
int xTimerStop(TimerHandle_t timer, int block_time);
int xTimerChangePeriod(TimerHandle_t timer, int new_period, int block_time);

// Task notification stubs
uint32_t ulTaskNotifyTake(int clear_on_exit, uint32_t timeout);
void xTaskNotifyGive(void *task_handle);

#ifdef __cplusplus
}
#endif
#endif
