#ifndef NEON_FREERTOS_FX1_H
#define NEON_FREERTOS_FX1_H

#include "FreeRTOS.h"
#include "fx1_fw.h"
#include "sdk_irq.h"

/* ISR must deassert the level source before returning: either service/W1C
 * the cause, or mask ALL enabled IP IRQ groups and defer ACK to its worker.
 * Masking preserves sticky status. PLIC complete follows handler return; a
 * worker must rearm/recheck before waiting. Return pdTRUE if a higher priority
 * task was woken through ...FromISR(). Hart 0 only, no nested device handlers. */
typedef BaseType_t (*NEON_FX1_IRQ_Handler)(void *user);

/* Gọi trên hart 0, trước vTaskStartScheduler(), khi MIE đang tắt. */
void NEON_FX1_FreeRTOS_Init(void);
/* Đăng ký từ task/khởi tạo; priority 1..FX1_PLIC_PRIORITY_MAX. */
BaseType_t NEON_FX1_IRQ_Register(uint32_t source, uint32_t priority,
                                NEON_FX1_IRQ_Handler handler, void *user);
/* Task/hart 0 only, IP already masked. Synchronously disable source and remove
 * only this handler/user pair. Pending PLIC state may remain disabled; a later
 * registration must tolerate a stale wakeup. No handler is in service when a
 * task runs on this non-nested hart (complete occurs before port yield). */
BaseType_t NEON_FX1_IRQ_Unregister(uint32_t source,
                                  NEON_FX1_IRQ_Handler handler, void *user);
void NEON_FX1_RTOS_Assert(const char *file, int line) __attribute__((noreturn));

#endif
