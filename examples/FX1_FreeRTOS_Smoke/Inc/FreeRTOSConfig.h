#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "fx1/fx1_memory_map.h"

/* Port RISC-V tính increment mtimecmp từ configCPU_CLOCK_HZ: dùng tần số
 * mtime 20 MHz, không dùng tần số thực thi lệnh của CPU. */
#define configMTIME_BASE_ADDRESS (FX1_CLINT_BASE + FX1_CLINT_MTIME_OFF)
#define configMTIMECMP_BASE_ADDRESS (FX1_CLINT_BASE + FX1_CLINT_MTIMECMP_OFF)
#define configCPU_CLOCK_HZ ((unsigned long)FX1_MTIME_HZ)
#define configTICK_RATE_HZ ((TickType_t)1000)
#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0
/* FPU vẫn bật trong CPU và được lưu bằng chip extension 34 word.
 * Giá trị 0 chỉ tắt đường lưu FPU native của kernel để tránh lưu hai lần. */
#define configENABLE_FPU 0
#define configUSE_IDLE_HOOK 1
#define configUSE_TICK_HOOK 1
#define configMAX_PRIORITIES 5
#define configMINIMAL_STACK_SIZE ((unsigned short)256)
#define configISR_STACK_SIZE_WORDS 1024
#define configTOTAL_HEAP_SIZE ((size_t)(128 * 1024))
#define configMAX_TASK_NAME_LEN 16
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 0
#define configUSE_COUNTING_SEMAPHORES 1
#define configUSE_TASK_NOTIFICATIONS 1
#define configQUEUE_REGISTRY_SIZE 0
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configUSE_MALLOC_FAILED_HOOK 1
#define configSUPPORT_STATIC_ALLOCATION 0
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configUSE_TIMERS 0
#define configUSE_CO_ROUTINES 0
#define configUSE_TRACE_FACILITY 0
#define INCLUDE_vTaskDelay 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_vTaskDelete 1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define configENABLE_BACKWARD_COMPATIBILITY 0

#ifndef __ASSEMBLER__
void NEON_FX1_RTOS_Assert(const char *file, int line) __attribute__((noreturn));
#define configASSERT(x) do { if (!(x)) NEON_FX1_RTOS_Assert(__FILE__, __LINE__); } while (0)
#endif

#endif
