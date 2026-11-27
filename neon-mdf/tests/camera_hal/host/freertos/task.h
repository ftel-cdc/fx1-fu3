#include "FreeRTOS.h"
BaseType_t xTaskCreate(void (*)(void *), const char *, uint32_t, void *, uint32_t, TaskHandle_t *);
void vTaskDelay(TickType_t);
TickType_t xTaskGetTickCount(void);
void vTaskNotifyGiveFromISR(TaskHandle_t, BaseType_t *);
void xTaskNotifyGive(TaskHandle_t);
uint32_t ulTaskNotifyTake(BaseType_t, TickType_t);
