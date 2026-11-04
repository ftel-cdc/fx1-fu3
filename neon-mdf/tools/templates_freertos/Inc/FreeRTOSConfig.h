/**
 * @file FreeRTOSConfig.h
 * @brief Public declarations for freertosconfig.
 * @details
 * * FreeRTOSConfig.h - cau hinh mac dinh cho app FreeRTOS moi tao qua "NEON-MDF:
 * Tao App Moi" - dung chung cho CA FU2 (Cortex-M0, port ARM_CM0), WEACT_F412
 * (Cortex-M4F, port ARM_CM4F) VA CH32V003F4P6 (RISC-V QingKe V2A, port
 * GCC_RISC_V_GENERIC + chip extension "RISCV_no_extensions") - xem
 * #if defined(...) duoi, target nao dung macro nao do cmake/freertos.cmake
 * chon FREERTOS_PORT tuong ung. Sua thoai mai theo nhu cau app (stack/heap/
 * priority/feature bat-tat) - xem examples/FreeRTOS_Blinky/Inc/FreeRTOSConfig.h
 * cho 1 vi du that day du hon (2 task) da hardware-verify tren WEACT_F412.
 *
 * File nay thuoc ve APP (khong phai SDK) - moi app tu co FreeRTOSConfig.h
 * rieng. Xem rtos/README.md.
 *
 * Luu y ve port ARM_CM0 (FU2, portable/GCC/ARM_CM0/port.c trong
 * FreeRTOS-Kernel): port nay set priority cua PendSV/SysTick bang hang so
 * portMIN_INTERRUPT_PRIORITY=0xFF (thap nhat co the, bat ke so bit priority
 * NVIC that implement bao nhieu) - nen KHONG can configPRIO_BITS/
 * configKERNEL_INTERRUPT_PRIORITY.
 *
 * Port ARM_CM4F (WEACT_F412, portable/GCC/ARM_CM4F/port.c) thi NGUOC LAI -
 * CAN configMAX_SYSCALL_INTERRUPT_PRIORITY (port.c dung truc tiep trong
 * portmacro.h de tinh BASEPRI) va configPRIO_BITS phai khop
 * __NVIC_PRIO_BITS that (=4, xem WEACT_F412.yaml cpu.nvic_priority_bits) -
 * gia tri chuan theo demo FreeRTOS chinh thuc cho STM32F4 (5 << (8-4) = 0x50,
 * de lai 4 muc priority cao nhat 0-4 cho ISR "khong kernel-aware", con lai
 * 5-15 duoc phep goi FreeRTOS API tu ISR).
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
/* "Indirect Routing" (xem comment o Src/main.c): vector table generic sinh
 * tu mcu-description luon dat ten SVC_Handler/PendSV_Handler/SysTick_Handler
 * (khong phai vPortSVCHandler/xPortPendSVHandler ma port CM4F mong doi) -
 * cac ham do nhay `b` sang dung ham FreeRTOS luc chay, nhung xPortStartScheduler()
 * tu kiem tra TINH (dia chi trong vector table == dia chi vPortSVCHandler)
 * chu khong kiem tra hanh vi - kiem tra nay CHI danh cho "Direct Routing",
 * phai tat bang macro nay neu khong se treo vinh vien tai configASSERT
 * that bai. */
#define configCHECK_HANDLER_INSTALLATION 0
#endif

/* SystemCoreClock cap nhat boi SystemCoreClockUpdate() (xem system_<target>.c) -
 * configTICK_RATE_HZ van dung vi tinh theo SystemCoreClock dong, khong
 * hardcode so Hz. */
extern uint32_t SystemCoreClock;
#define configCPU_CLOCK_HZ (SystemCoreClock)

#define configTICK_RATE_HZ (1000)
#define configTICK_TYPE_WIDTH_IN_BITS TICK_TYPE_WIDTH_32_BITS

#define configUSE_PREEMPTION 1
#define configUSE_TIME_SLICING 1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 0 /* Cortex-M0 khong co lenh CLZ */
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
#define configTOTAL_HEAP_SIZE (8 * 1024) /* trong RAM_USR 64KB, xem FU2.yaml */
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

/* Port ARM_CM0 dung MPU_WRAPPERS_V2 optional - FU2 (Cortex-M0 thuan) khong
 * co MPU nen tat han. */
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
/* QingKe V2A (CH32V003) KHONG co CLINT (Core Local Interrupter chuan cua
 * RISC-V ma FreeRTOS RISC-V port mac dinh gia dinh) - PFIC (Programmable
 * Fast interrupt Controller) rieng cua WCH quan ly ngat thay CLINT. Port
 * RISC-V cua FreeRTOS-Kernel HO TRO SAN cau hinh "khong CLINT" nay (KHONG
 * phai tu improvise) qua bien the chip_specific_extensions
 * "RISCV_no_extensions" - xem cmake/freertos.cmake (chon
 * FREERTOS_PORT=GCC_RISC_V_GENERIC + FREERTOS_RISCV_EXTENSION=RISCV_no_extensions)
 * va Src/main.c (noi setup that: mtvec Direct mode + STK sinh ngat tick).
 *
 * configMTIME_BASE_ADDRESS/configMTIMECMP_BASE_ADDRESS = 0 bao port.c biet
 * KHONG co thanh ghi mtime/mtimecmp that o dia chi nao ca - port se KHONG
 * tu dong cau hinh tick timer (vPortSetupTimerInterrupt() mac dinh la ham
 * rong "weak" - Src/main.c tu ghi de that). */
#define configMTIME_BASE_ADDRESS 0
#define configMTIMECMP_BASE_ADDRESS 0

/* ISR dung stack TINH RIENG (mang static, xem port.c) - tranh phai sua
 * linker script sinh tu mcu-description them 1 symbol rieng
 * (__freertos_irq_stack_top) chi cho rieng FreeRTOS. */
#define configISR_STACK_SIZE_WORDS 32

/* CH32V003F4P6 chi co 2KB RAM tong (xem CH32V003F4P6.yaml) - ngan sach CHAT
 * hon han FU2 (64KB RAM_USR)/WEACT_F412 (256KB) rat nhieu, nen PHAI giam
 * stack/heap va TAT timer service task so voi gia tri dung chung o tren
 * (128 words/8KB, tinh cho RAM lon). Gia tri duoi day DA XAC NHAN THAT qua
 * build (xem examples/FreeRTOS_Blinky, template nay it task hon nen con
 * NHIEU margin hon truong hop do - 1952/2048B RAM voi 3 task, template nay
 * chi co 2 task idle+main) - neu app phinh to hon (them task/queue/mutex),
 * theo doi bao loi that cua linker ("region RAM overflowed") de biet khi
 * nao can giam them, KHONG doan truoc. */
#undef configUSE_TIMERS
#define configUSE_TIMERS 0
#undef configMINIMAL_STACK_SIZE
#define configMINIMAL_STACK_SIZE 64 /* words = 256B/task, thay vi 128 */
#undef configTOTAL_HEAP_SIZE
#define configTOTAL_HEAP_SIZE (1024) /* 1KB, thay vi 8KB */
#endif                               /* CH32V003F4P6 */

#endif /* FREERTOS_CONFIG_H */
