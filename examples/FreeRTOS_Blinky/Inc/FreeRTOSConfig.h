/**
 * @file FreeRTOSConfig.h
 * @brief Public declarations for freertosconfig.
 * @details
 * * FreeRTOSConfig.h configures FreeRTOS_Blinky for FU2 (Cortex-M0, ARM_CM0), WEACT_F412 (Cortex-M4F,
 * ARM_CM4F) and
 * CH32V003F4P6 (RISC-V QingKe V2A, port GCC_RISC_V_GENERIC + chip extension
 * "RISCV_no_extensions", added 2026-08-10). See the #if defined(...) branches below;
 * cmake/freertos.cmake selects FREERTOS_PORT for each target.
 *
 * This configuration belongs to the example application. Each app has its own FreeRTOSConfig.h and
 * chooses its stack, heap and priority settings. See
 * rtos/README.md.
 *
 * The ARM_CM0 port (FU2, portable/GCC/ARM_CM0/port.c in FreeRTOS-Kernel) sets the PendSV/SysTick
 * priority to portMIN_INTERRUPT_PRIORITY=0xFF, the lowest priority regardless of the implemented
 * NVIC priority width. It therefore does not require configPRIO_BITS or
 * configKERNEL_INTERRUPT_PRIORITY. Verified by reading the upstream port.c
 * vendor.
 *
 * The ARM_CM4F port (WEACT_F412, portable/GCC/ARM_CM4F/port.c) requires
 * configMAX_SYSCALL_INTERRUPT_PRIORITY, which portmacro.h uses to calculate BASEPRI.
 * configPRIO_BITS must match __NVIC_PRIO_BITS (=4; see WEACT_F412.yaml cpu.nvic_priority_bits).
 * These settings follow the official STM32F4 FreeRTOS demo: 5 << (8-4) = 0x50. Priorities 0-4 are
 * reserved for ISRs that do not call the kernel; priorities 5-15 may call FreeRTOS APIs from an
 * ISR.
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
/* "Indirect Routing" (see Src/main.c): mcu-description generates
 * SVC_Handler/PendSV_Handler/SysTick_Handler names rather than the
 * vPortSVCHandler/xPortPendSVHandler names expected by the CM4F port. Wrappers branch with `b` to
 * the FreeRTOS handlers at runtime. However, xPortStartScheduler() checks static vector addresses
 * (the SVC vector must equal vPortSVCHandler), not runtime behavior. This check supports "Direct
 * Routing" only and must be disabled with this macro (port.c lines 322-325), or configASSERT fails
 * and hangs permanently. Confirmed with SWD debugging. */
#define configCHECK_HANDLER_INSTALLATION 0
#endif

/* SystemCoreClock is updated by SystemCoreClockUpdate(); see system_FU2.c. The clock currently remains
 * at its reset frequency because CRM setup is unfinished. configTICK_RATE_HZ remains correct
 * because it uses the current SystemCoreClock rather than a hardcoded frequency. */
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
#define configTOTAL_HEAP_SIZE (8 * 1024) /* FU2 application RAM: 124KB, see FU2.yaml */
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

/* The ARM_CM0 port optionally supports MPU_WRAPPERS_V2. FU2 has no MPU, so disable it. */
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
/* QingKe V2A (CH32V003) has no CLINT (the standard RISC-V Core Local Interrupter assumed by the
 * default FreeRTOS RISC-V port). Verified against the official QingKeV2 Processor Manual V1.0 from
 * ch405-labs.com, including MIE, mcause and mtvec[1:0] direct mode. WCH uses its own PFIC
 * (Programmable Fast Interrupt Controller). FreeRTOS-Kernel already supports this configuration
 * through chip_specific_extensions "RISCV_no_extensions"; see cmake/freertos.cmake
 * (FREERTOS_PORT=GCC_RISC_V_GENERIC +
 * FREERTOS_RISCV_EXTENSION=RISCV_no_extensions) and Src/main.c (which implements the
 * implementation: mtvec Direct mode and QingKe STK generate the tick interrupt. STK addresses/bits
 * differ from CLINT mtime; PFIC IENR enables the IRQ through SDK_IRQ_Enable()).
 *
 * configMTIME_BASE_ADDRESS/configMTIMECMP_BASE_ADDRESS = 0 tells port.c that no mtime/mtimecmp
 * registers exist. The port therefore does not configure the tick timer automatically. Its default
 * vPortSetupTimerInterrupt() is an empty weak function, overridden by the application in
 * Src/main.c. */
#define configMTIME_BASE_ADDRESS 0
#define configMTIMECMP_BASE_ADDRESS 0

/* The ISR uses its own statically allocated stack (see port.c), avoiding a FreeRTOS-only
 * __freertos_irq_stack_top symbol in the linker script generated by mcu-description. */
#define configISR_STACK_SIZE_WORDS 32

/* CH32V003F4P6 has only 2KB total RAM (CH32V003F4P6.yaml), much less than FU2 (64KB RAM_USR) or
 * WEACT_F412 (256KB). Reduce the stack and heap and disable the unused timer service task. The
 * common defaults above are 128 words per stack and an 8KB heap. Initial sizing includes three
 * tasks (idle+led0+led1, no timer task), each with a roughly 60-90B TCB plus
 * configMINIMAL_STACK_SIZE*4B, and the ISR stack. Hardware verification was pending when this was
 * written. Adjust these estimates using actual build/link size reports; the linker reports an error
 * if RAM exceeds the 2KB region. */
#undef configUSE_TIMERS
#define configUSE_TIMERS 0
#undef configMINIMAL_STACK_SIZE
#define configMINIMAL_STACK_SIZE 64 /* words = 256B/task, instead of 128 */
#undef configTOTAL_HEAP_SIZE
/* The initial 1280B (1.25KB) heap overflowed physical RAM by 160 bytes, confirmed by the linker error
 * "region RAM overflowed by 160 bytes" for CH32V003F4P6. Reduce it to 1024B to leave additional
 * margin while hardware stack/heap verification remains pending; see above. */
#define configTOTAL_HEAP_SIZE (1024) /* 1KB instead of 8KB */
#endif                               /* CH32V003F4P6 */

#endif /* FREERTOS_CONFIG_H */
