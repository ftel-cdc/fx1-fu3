/**
 * @file FreeRTOSConfig.h
 * @brief FU2 application configuration, based on FreeRTOS_Blinky.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include <stdint.h>

extern uint32_t SystemCoreClock;
#define configCPU_CLOCK_HZ (SystemCoreClock)
#define configTICK_RATE_HZ 1000
#define configTICK_TYPE_WIDTH_IN_BITS TICK_TYPE_WIDTH_32_BITS

#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0 /* Cortex-M0 has no CLZ instruction. */
#define configUSE_TICKLESS_IDLE 0
#define configMAX_PRIORITIES 3
#define configMINIMAL_STACK_SIZE 128 /* Stack depths are in words. */
#define configMAX_TASK_NAME_LEN 16
#define configIDLE_SHOULD_YIELD 1

#define configUSE_TIMERS 0
#define configUSE_EVENT_GROUPS 0
#define configUSE_STREAM_BUFFERS 0
#define configUSE_MUTEXES 0
#define configUSE_RECURSIVE_MUTEXES 0
#define configUSE_COUNTING_SEMAPHORES 0
#define configUSE_TASK_NOTIFICATIONS 0
#define configUSE_QUEUE_SETS 0
#define configUSE_CO_ROUTINES 0

#define configSUPPORT_STATIC_ALLOCATION 0
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configTOTAL_HEAP_SIZE (8 * 1024)
#define configAPPLICATION_ALLOCATED_HEAP 0
#define configHEAP_CLEAR_MEMORY_ON_FREE 1
#define configUSE_NEWLIB_REENTRANT 0 /* Only the terminal task writes to the FU2 console. */

#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configUSE_MALLOC_FAILED_HOOK 1
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configGENERATE_RUN_TIME_STATS 0
#define configUSE_TRACE_FACILITY 0
#define configUSE_STATS_FORMATTING_FUNCTIONS 0
#define configQUEUE_REGISTRY_SIZE 0
#define configENABLE_BACKWARD_COMPATIBILITY 0
#define configSTACK_DEPTH_TYPE uint32_t
#define configMESSAGE_BUFFER_LENGTH_TYPE size_t
#define configENABLE_MPU 0
/* FU2 has no VTOR. BootROM v5 forwards exceptions to application vectors.
 * The default port check reads VTOR (0xe000ed08), which is invalid on FU2.
 * Disable that direct-routing check while retaining configASSERT. */
#define configCHECK_HANDLER_INSTALLATION 0

/* The ARM_CM0 port supplies PendSV/SysTick handlers and masks interrupts with
 * PRIMASK; it does not use the Cortex-M4 BASEPRI priority configuration. */
#define configASSERT(condition)                                                                                         \
    do {                                                                                                               \
        if ((condition) == 0) {                                                                                         \
            taskDISABLE_INTERRUPTS();                                                                                   \
            for (;;) {                                                                                                 \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskDelayUntil 1
#define INCLUDE_vTaskSuspend 1 /* Allow portMAX_DELAY to block indefinitely. */
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1

#endif /* FREERTOS_CONFIG_H */
