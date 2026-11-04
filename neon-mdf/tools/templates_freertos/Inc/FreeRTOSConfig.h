/**
 * @file FreeRTOSConfig.h
 * @brief Public declarations for freertosconfig.
 * @details
 * * FreeRTOSConfig.h - default configuration for FreeRTOS app each created via "NEON-MDF: create new
 * app" - shared with both FU2 (Cortex-M0, port ARM_CM0), WEACT_F412
 * (Cortex-M4F, ARM_CM4F) and CH32V003F4P6 (RISC-V QingKe V2A, port
 * GCC_RISC_V_GENERIC + chip extension "RISCV_no_extensions") - view
 * #if defined(...) below. cmake/freertos.cmake selects FREERTOS_PORT for each target
 * compatible. edit later according to cau app (stack/heap/ priority/feature on-off) - see
 * examples/FreeRTOS_Blinky/Inc/FreeRTOSConfig.h for a more complete task (2 tasks) hardware-verify
 * on WEACT_F412.
 *
 * This file belongs to APP (not SDK) - each app from has its own FreeRTOSConfig.h. See rtos/README.md.
 *
 * Note about the port ARM_CM0 (FU2, portable/GCC/ARM_CM0/port.c in FreeRTOS-Kernel): this port sets
 * the priority of PendSV/SysTick with the number portMIN_INTERRUPT_PRIORITY=0xFF (as far as
 * possible, enable multiple implementation NVIC priority bits) - so no configPRIO_BITS/ is needed
 * configKERNEL_INTERRUPT_PRIORITY.
 *
 * Port ARM_CM4F (WEACT_F412, portable/GCC/ARM_CM4F/port.c) then NGUOC again - needs
 * configMAX_SYSCALL_INTERRUPT_PRIORITY (port.c used directly in portmacro.h to compute BASEPRI) and
 * configPRIO_BITS must match real __NVIC_PRIO_BITS (=4, see WEACT_F412.yaml cpu.nvic_priority_bits)
 * - the value of the true FreeRTOS demo for STM32F4 (5 << (8-4) = 0x50, leaving 4 highest priority
 * entries 0-4 for "non-kernel-aware" ISRs, the 5-15 children are allowed to call the FreeRTOS API
 * from ISR).
 */

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>

#if defined(WEACT_F412)

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define configPRIO_BITS 4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY 0xF
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
/* "Indirect Routing" (see comment in Src/main.c): generic table vector created from mcu-description
 * always named SVC_Handler/PendSV_Handler/SysTick_Handler (not vPortSVCHandler/xPortPendSVHandler
 * but CM4F port expected to change) - these functions jump `b` to use FreeRTOS at runtime, but
 * xPortStartScheduler() manually checks the validity (dia only in vector table == dia only
 * vPortSVCHandler) text does not check behavior - this check specifies "Direct Routing", must be
 * disabled with this macro or it will hang permanently at configASSERT failure. */
#define configCHECK_HANDLER_INSTALLATION 0
#endif

/* SystemCoreClock first level by SystemCoreClockUpdate() (see system_<target>.c) - configTICK_RATE_HZ
 * still uses computers under dynamic SystemCoreClock, not
 * hardcode so Hz. */
extern uint32_t SystemCoreClock;
#define configCPU_CLOCK_HZ (SystemCoreClock)

#define configTICK_RATE_HZ (1000)
#define configTICK_TYPE_WIDTH_IN_BITS TICK_TYPE_WIDTH_32_BITS

#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0 /* Cortex-M0 without CLZ command */
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
#define configTOTAL_HEAP_SIZE (8 * 1024) /* in RAM_USR 64KB, see FU2.yaml */
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

/* Port ARM_CM0 uses MPU_WRAPPERS_V2 optional - FU2 (Cortex-M0 thuan) without MPU, so disable. */
#define configENABLE_MPU 0

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

#if defined(CH32V003F4P6)
/* QingKe V2A (CH32V003) has no standard RISC-V CLINT (Core Local Interrupter), which FreeRTOS
 * RISC-V port default assumes) - WCH's own PFIC (Programmable Fast interrupt Controller) interrupt
 * controls interrupts instead of CLINT. FreeRTOS-Kernel already supports this no-CLINT configuration
 * (not from improvise) via the chip_specific_extensions variant "RISCV_no_extensions" - see
 * cmake/freertos.cmake (select
 * FREERTOS_PORT=GCC_RISC_V_GENERIC + FREERTOS_RISCV_EXTENSION=RISCV_no_extensions)
 * and Src/main.c (where real setup: mtvec Direct mode + STK create interrupt tick).
 *
 * configMTIME_BASE_ADDRESS/configMTIMECMP_BASE_ADDRESS = 0 tells port.c that there is no real
 * mtime/mtimecmp register at the port-only dia, and does not automatically configure a tick timer
 * (the default vPortSetupTimerInterrupt() is a "weak" empty function - Src/main.c writes over the
 * actual register). */
#define configMTIME_BASE_ADDRESS 0
#define configMTIMECMP_BASE_ADDRESS 0

/* The ISR uses its own statically allocated stack (see port.c), avoiding a FreeRTOS-only
 * __freertos_irq_stack_top symbol in the linker script generated by mcu-description. */
#define configISR_STACK_SIZE_WORDS 32

/* CH32V003F4P6 has only 2KB of total RAM (see CH32V003F4P6.yaml) - the budget is much tighter than FU2
 * (64KB RAM_USR)/WEACT_F412 (256KB) so we have to reduce stack/heap and turn off timer service task
 * compared with the common defaults above (128 words/8KB for larger RAM). The values below were
 * actually confirmed through build (see examples/FreeRTOS_Blinky, this template it tasks more so
 * you have more margin than that case - 1952/2048B RAM with 3 tasks, this template only has 2 tasks
 * idle+main) - if the app is louder (add task/queue/mutex), according to the real error message of
 * the linker ("region RAM overflowed") to know when to reduce further, unpredictable. */
#undef configUSE_TIMERS
#define configUSE_TIMERS 0
#undef configMINIMAL_STACK_SIZE
#define configMINIMAL_STACK_SIZE 64 /* words = 256B/task, instead of 128 */
#undef configTOTAL_HEAP_SIZE
#define configTOTAL_HEAP_SIZE (1024) /* 1KB instead of 8KB */
#endif                               /* CH32V003F4P6 */

#endif /* FREERTOS_CONFIG_H */
