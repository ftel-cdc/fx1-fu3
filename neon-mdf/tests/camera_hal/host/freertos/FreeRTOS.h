#ifndef TEST_FREERTOS_H
#define TEST_FREERTOS_H
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
typedef int BaseType_t;
typedef uint32_t TickType_t;
typedef void *TaskHandle_t;
typedef void *QueueHandle_t;
#define pdTRUE 1
#define pdFALSE 0
#define pdPASS 1
#define pdFAIL 0
#define portMAX_DELAY UINT32_MAX
#define configMAX_PRIORITIES 5u
#define configUSE_TASK_NOTIFICATIONS 1
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configASSERT(x) assert(x)
#define taskENTER_CRITICAL() fake_enter()
#define taskEXIT_CRITICAL() fake_exit()
void fake_enter(void);
void fake_exit(void);
#endif
