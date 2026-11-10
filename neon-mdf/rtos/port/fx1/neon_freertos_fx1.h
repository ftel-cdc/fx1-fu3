#ifndef NEON_FREERTOS_FX1_H
#define NEON_FREERTOS_FX1_H

#include "FreeRTOS.h"
#include "fx1_fw.h"
#include "sdk_irq.h"

/* ISR phải xóa nguyên nhân tại IP trước khi trả về. Trả pdTRUE nếu đã
 * đánh thức task ưu tiên cao hơn bằng API ...FromISR(). */
typedef BaseType_t (*NEON_FX1_IRQ_Handler)(void *user);

/* Gọi trên hart 0, trước vTaskStartScheduler(), khi MIE đang tắt. */
void NEON_FX1_FreeRTOS_Init(void);
/* Đăng ký từ task/khởi tạo; priority 1..FX1_PLIC_PRIORITY_MAX. */
BaseType_t NEON_FX1_IRQ_Register(uint32_t source, uint32_t priority,
                                NEON_FX1_IRQ_Handler handler, void *user);
void NEON_FX1_RTOS_Assert(const char *file, int line) __attribute__((noreturn));

#endif
