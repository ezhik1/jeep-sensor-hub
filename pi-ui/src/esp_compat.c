\
#include "esp_compat.h"
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <stdio.h>

static pthread_mutex_t esp_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t esp_lvgl_mutex = PTHREAD_MUTEX_INITIALIZER;

void esp_log_printf(const char *level, const char *tag, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	pthread_mutex_lock(&esp_log_mutex);
	fprintf(stdout, "[%s] %s: ", level, tag ? tag : "");
	vfprintf(stdout, fmt, ap);
	fprintf(stdout, "\n");
	fflush(stdout);
	pthread_mutex_unlock(&esp_log_mutex);
	va_end(ap);
}

uint64_t esp_timer_get_time(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)(ts.tv_nsec / 1000ULL);
}

struct thread_wrapper {
	void (*fn)(void*);
	void *arg;
};

static void *thread_trampoline(void *arg) {
	struct thread_wrapper *tw = (struct thread_wrapper*)arg;
	if (tw && tw->fn) {
		tw->fn(tw->arg);
	}
	free(tw);
	return NULL;
}

int esp_xTaskCreatePinnedToCore(void (*task_func)(void*), const char *name, unsigned int stack_size, void *param, unsigned int prio, void *task_handle, int core_id) {
	pthread_t thr;
	struct thread_wrapper *tw = malloc(sizeof(*tw));
	tw->fn = task_func;
	tw->arg = param;
	if (pthread_create(&thr, NULL, thread_trampoline, tw) != 0) {
		free(tw);
		return -1;
	}
	pthread_detach(thr);
	(void)name; (void)stack_size; (void)prio; (void)task_handle; (void)core_id;
	return 0;
}

int esp_xTaskCreate(void (*task_func)(void*), const char *name, unsigned int stack_size, void *param, unsigned int prio, void *task_handle) {
	return esp_xTaskCreatePinnedToCore(task_func, name, stack_size, param, prio, task_handle, -1);
}

void esp_vTaskDelete(void *task_handle) {
	(void)task_handle;
	pthread_exit(NULL);
}

void esp_vTaskDelay(unsigned int ms) {
	usleep(ms * 1000);
}

void esp_vTaskDelayUntil(unsigned long *prev_wake_time, unsigned int ms) {
	(void)prev_wake_time;
	esp_vTaskDelay(ms);
}

// Simple queue stub using a malloc'd ring buffer (not thread-safe beyond naive use)
// For complex use, replace with POSIX message queues or proper thread-safe containers.
typedef struct {
	unsigned int len;
	unsigned int item_size;
	unsigned int head, tail;
	unsigned int used;
	char data[];
} simple_queue_t;

void* esp_xQueueCreate(unsigned int uxQueueLength, unsigned int uxItemSize) {
	simple_queue_t *q = malloc(sizeof(simple_queue_t) + uxQueueLength * uxItemSize);
	if (!q) return NULL;
	q->len = uxQueueLength;
	q->item_size = uxItemSize;
	q->head = q->tail = q->used = 0;
	return q;
}

int esp_xQueueSend(void *queue, const void *item, unsigned int ticks_to_wait) {
	simple_queue_t *q = (simple_queue_t*)queue;
	if (!q) return -1;
	if (q->used >= q->len) return -1;
	memcpy(q->data + q->tail * q->item_size, item, q->item_size);
	q->tail = (q->tail + 1) % q->len;
	q->used++;
	(void)ticks_to_wait;
	return 0;
}

int esp_xQueueReceive(void *queue, void *buffer, unsigned int ticks_to_wait) {
	simple_queue_t *q = (simple_queue_t*)queue;
	if (!q || q->used == 0) return -1;
	memcpy(buffer, q->data + q->head * q->item_size, q->item_size);
	q->head = (q->head + 1) % q->len;
	q->used--;
	(void)ticks_to_wait;
	return 0;
}

void esp_portENTER_CRITICAL(void) {
	pthread_mutex_lock(&esp_log_mutex);
}

void esp_portEXIT_CRITICAL(void) {
	pthread_mutex_unlock(&esp_log_mutex);
}

int lvgl_port_lock(int wait_ms) {
	(void)wait_ms;
	pthread_mutex_lock(&esp_lvgl_mutex);
	return 1;
}

void lvgl_port_unlock(void) {
	pthread_mutex_unlock(&esp_lvgl_mutex);
}

uint32_t xTaskGetTickCount(void) {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint32_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}

const char* esp_err_to_name(int err) {
	switch (err) {
		case ESP_OK: return "ESP_OK";
		case ESP_FAIL: return "ESP_FAIL";
		case ESP_ERR_NO_MEM: return "ESP_ERR_NO_MEM";
		case ESP_ERR_INVALID_ARG: return "ESP_ERR_INVALID_ARG";
		case ESP_ERR_INVALID_STATE: return "ESP_ERR_INVALID_STATE";
		case ESP_ERR_TIMEOUT: return "ESP_ERR_TIMEOUT";
		default: return "ESP_UNKNOWN_ERROR";
	}
}

// NVS stub implementations (using simple file-based storage)
static char nvs_file_path[256] = "/tmp/jeep_sensor_hub_nvs";

int nvs_flash_init(void) {
	// Create NVS directory if it doesn't exist
	return ESP_OK;
}

int nvs_flash_erase(void) {
	// Remove NVS file
	unlink(nvs_file_path);
	return ESP_OK;
}

int nvs_open(const char* name, int open_mode, nvs_handle_t* handle) {
	// Simple file-based NVS implementation
	*handle = (nvs_handle_t)malloc(1);
	return ESP_OK;
}

int nvs_set_i32(nvs_handle_t handle, const char* key, int32_t value) {
	// Write to file (simplified)
	FILE* f = fopen(nvs_file_path, "a");
	if (f) {
		fprintf(f, "%s=%d\n", key, value);
		fclose(f);
	}
	return ESP_OK;
}

int nvs_set_u8(nvs_handle_t handle, const char* key, uint8_t value) {
	return nvs_set_i32(handle, key, value);
}

int nvs_set_u32(nvs_handle_t handle, const char* key, uint32_t value) {
	return nvs_set_i32(handle, key, value);
}

int nvs_get_i32(nvs_handle_t handle, const char* key, int32_t* value) {
	// Read from file (simplified)
	FILE* f = fopen(nvs_file_path, "r");
	if (f) {
		char line[256];
		while (fgets(line, sizeof(line), f)) {
			if (strncmp(line, key, strlen(key)) == 0 && line[strlen(key)] == '=') {
				*value = atoi(line + strlen(key) + 1);
				fclose(f);
				return ESP_OK;
			}
		}
		fclose(f);
	}
	return ESP_FAIL;
}

int nvs_get_u8(nvs_handle_t handle, const char* key, uint8_t* value) {
	int32_t val;
	int ret = nvs_get_i32(handle, key, &val);
	if (ret == ESP_OK) *value = (uint8_t)val;
	return ret;
}

int nvs_get_u32(nvs_handle_t handle, const char* key, uint32_t* value) {
	int32_t val;
	int ret = nvs_get_i32(handle, key, &val);
	if (ret == ESP_OK) *value = (uint32_t)val;
	return ret;
}

int nvs_commit(nvs_handle_t handle) {
	return ESP_OK;
}

void nvs_close(nvs_handle_t handle) {
	free(handle);
}

// Timer stub implementations
TimerHandle_t xTimerCreate(const char* name, int period, int auto_reload, void* id, void (*callback)(TimerHandle_t)) {
	// Simple timer stub
	return (TimerHandle_t)malloc(1);
}

int xTimerStart(TimerHandle_t timer, int block_time) {
	return ESP_OK;
}

int xTimerStop(TimerHandle_t timer, int block_time) {
	return ESP_OK;
}

int xTimerChangePeriod(TimerHandle_t timer, int new_period, int block_time) {
	return ESP_OK;
}

uint32_t ulTaskNotifyTake(int clear_on_exit, uint32_t timeout) {
	// Simple stub - just return 0
	return 0;
}

void xTaskNotifyGive(void *task_handle) {
	// Simple stub - do nothing
	(void)task_handle;
}

int esp_timer_init(void) {
	// Initialize timer system (stub)
	return ESP_OK;
}

int esp_timer_deinit(void) {
	// Deinitialize timer system (stub)
	return ESP_OK;
}
