/**
 * @file FreeRTOSConfig.h
 * @brief GT32U2086 FreeRTOS application configuration.
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>


/* SystemInit updates SystemCoreClock for the selected hardware clock. */
extern uint32_t SystemCoreClock;
#define configCPU_CLOCK_HZ (SystemCoreClock)

#define configTICK_RATE_HZ (1000)
#define configTICK_TYPE_WIDTH_IN_BITS TICK_TYPE_WIDTH_32_BITS

#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0 /* Cortex-M0 has no CLZ instruction */
#define configUSE_TICKLESS_IDLE 0

#define configMAX_PRIORITIES 4
#define configMINIMAL_STACK_SIZE 128 /* words */
#define configMAX_TASK_NAME_LEN 16
#define configIDLE_SHOULD_YIELD 1

#define configUSE_TIMERS 1
#define configTIMER_TASK_PRIORITY (configMAX_PRIORITIES - 1)
#define configTIMER_TASK_STACK_DEPTH configMINIMAL_STACK_SIZE
#define configTIMER_QUEUE_LENGTH 10

#define configUSE_EVENT_GROUPS 1
#define configUSE_STREAM_BUFFERS 1
#define configUSE_MUTEXES 1
#define configUSE_RECURSIVE_MUTEXES 1
#define configUSE_COUNTING_SEMAPHORES 1
#define configUSE_TASK_NOTIFICATIONS 1
#define configUSE_QUEUE_SETS 0
#define configUSE_APPLICATION_TASK_TAG 0
#define configUSE_CO_ROUTINES 0

#define configSUPPORT_STATIC_ALLOCATION 0
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configTOTAL_HEAP_SIZE (8 * 1024) /* GT32U2086 application RAM: 124KB, see GT32U2086.yaml */
#define configAPPLICATION_ALLOCATED_HEAP 0

#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configUSE_MALLOC_FAILED_HOOK 0
#define configCHECK_FOR_STACK_OVERFLOW 2

#define configGENERATE_RUN_TIME_STATS 0
#define configUSE_TRACE_FACILITY 0
#define configUSE_STATS_FORMATTING_FUNCTIONS 0
#define configQUEUE_REGISTRY_SIZE 0
#define configENABLE_BACKWARD_COMPATIBILITY 0
#define configNUM_THREAD_LOCAL_STORAGE_POINTERS 0
#define configUSE_MINI_LIST_ITEM 1
#define configSTACK_DEPTH_TYPE uint32_t
#define configMESSAGE_BUFFER_LENGTH_TYPE size_t
#define configHEAP_CLEAR_MEMORY_ON_FREE 1
#define configUSE_NEWLIB_REENTRANT 0

/* The ARM_CM0 port optionally supports MPU_WRAPPERS_V2. GT32U2086 has no MPU, so disable it. */
#define configENABLE_MPU 0

/* BootROM relays exceptions; Cortex-M0 has no VTOR to inspect. */
#define configCHECK_HANDLER_INSTALLATION 0

#define configASSERT(x)                                                                                                \
    if ((x) == 0) {                                                                                                    \
        taskDISABLE_INTERRUPTS();                                                                                      \
        for (;;)                                                                                                       \
            ;                                                                                                          \
    }

#define INCLUDE_vTaskPrioritySet 1
#define INCLUDE_uxTaskPriorityGet 1
#define INCLUDE_vTaskDelete 1
#define INCLUDE_vTaskSuspend 1
#define INCLUDE_xTaskDelayUntil 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskGetSchedulerState 1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define INCLUDE_uxTaskGetStackHighWaterMark 0
#define INCLUDE_xTaskGetIdleTaskHandle 0
#define INCLUDE_eTaskGetState 0
#define INCLUDE_xTimerPendFunctionCall 0
#define INCLUDE_xTaskAbortDelay 0
#define INCLUDE_xTaskGetHandle 0
#define INCLUDE_xTaskResumeFromISR 1


#endif /* FREERTOS_CONFIG_H */
