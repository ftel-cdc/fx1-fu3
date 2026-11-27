/* VP/board port probe, independent of profile-mode camera acceptance. Generates
 * only a deliberately invalid DMA configuration, never a successful image.
 * Exercises real IP mask -> PLIC complete -> deferred W1C and handler detach. */
#include "neon_freertos_fx1.h"
#include "task.h"
#include "isp/neon_isp_claim.h"
#include "isp/platform/fx1/fx1_isp_irq.h"
#include "fx1_isp/fx1_isp_csr.h"
#include "fx1/fx1_memory_map.h"
#define R(n) FX1_ISP_##n##_OFFSET
static TaskHandle_t owner;
static fx1_isp_io_t io;
static uint32_t rd(void *u, uint32_t r) { (void)u; return fx1_read32(FX1_ISP_CSR_BASE + r); }
static void wr(void *u, uint32_t r, uint32_t v) { (void)u; fx1_write32(FX1_ISP_CSR_BASE + r, v); }
static void fence(void *u) { (void)u; fx1_mb(); }
static BaseType_t handler(void *u)
{
    BaseType_t woken = pdFALSE;
    fx1_isp_irq_gate(&io); /* The only MMIO performed by top half. */
    vTaskNotifyGiveFromISR((TaskHandle_t)u, &woken);
    return woken;
}
static void check(bool ok, uint32_t code) { if (!ok) fx1_fail(code); }
static void probe(void *u)
{
    (void)u;
    check(neon_isp_device_claim(&io), 0xc101);
    fx1_isp_irq_gate(&io);
    check(NEON_FX1_IRQ_Register(FX1_IRQ_ISP, 2, handler, owner) == pdPASS, 0xc102);
    check(NEON_FX1_IRQ_Register(FX1_IRQ_ISP, 2, handler, owner) == pdFAIL, 0xc103);
    wr(NULL, R(COMMON_FRAME_WIDTH), 64); wr(NULL, R(COMMON_FRAME_HEIGHT), 48);
    wr(NULL, R(IDMA_STRIDE), 1); wr(NULL, R(IDMA_BUF_ADDR_L0), 1);
    wr(NULL, R(IDMA_BUF_ADDR_H0), 0); wr(NULL, R(IDMA_BUF_VALID), 1);
    wr(NULL, R(COMMON_CTRL), 1);
    (void)fx1_isp_irq_rearm(&io, (fx1_isp_irq_bits_t){2,FX1_IRQ_CFG,0});
    wr(NULL, R(DMA_CTRL), 1); fence(NULL);
    check(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100)) != 0, 0xc104);
    fx1_isp_irq_bits_t captured = fx1_isp_irq_read(&io);
    fx1_log_hex("DEFERRED_COMMON", captured.common); fx1_log_hex("DEFERRED_DMA", captured.dma);
    check((captured.dma & FX1_IRQ_CFG) != 0, 0xc105);
    check(rd(NULL, R(COMMON_IRQ_EN)) == 0 && rd(NULL, R(DMA_IRQ_EN)) == 0, 0xc106);
    check(rd(NULL, R(ODMA_BUF_DONE)) == 0, 0xc107);
    /* Re-enable while PLIC disabled to leave a pending request. Mask the IP
     * again without clearing sticky status, then detach the old context. */
    SDK_IRQ_Disable(FX1_IRQ_ISP);
    wr(NULL, R(DMA_IRQ_EN), FX1_IRQ_CFG); fence(NULL);
    vTaskDelay(1);
    fx1_isp_irq_gate(&io);
    check(NEON_FX1_IRQ_Unregister(FX1_IRQ_ISP, handler, &io) == pdFAIL, 0xc108);
    check(NEON_FX1_IRQ_Unregister(FX1_IRQ_ISP, handler, owner) == pdPASS, 0xc109);
    fx1_isp_irq_ack(&io, captured, captured);
    check(NEON_FX1_IRQ_Register(FX1_IRQ_ISP, 2, handler, owner) == pdPASS, 0xc10a);
    /* Stale pending is permitted, never required. If delivered the replacement
     * ISR masks and wakes safely even though there is no new frame status. */
    vTaskDelay(1); (void)ulTaskNotifyTake(pdTRUE, 0);
    check(rd(NULL, R(ODMA_BUF_DONE)) == 0, 0xc10b);
    fx1_isp_irq_gate(&io);
    check(NEON_FX1_IRQ_Unregister(FX1_IRQ_ISP, handler, owner) == pdPASS, 0xc10c);
    uint32_t error = rd(NULL, R(DMA_ERR)) & FX1_ISP_DMA_ERR_W1C_MASK;
    wr(NULL, R(DMA_ERR), error);
    wr(NULL, R(DMA_CTRL), FX1_ISP_DMA_CTRL_SOFT_RESET_MASK); fence(NULL);
    TickType_t start = xTaskGetTickCount();
    while (rd(NULL, R(COMMON_RECOVERY_STATUS)) & FX1_ISP_COMMON_RECOVERY_STATUS_BUSY_MASK) {
        check(xTaskGetTickCount() - start < pdMS_TO_TICKS(100), 0xc10d); vTaskDelay(1);
    }
    wr(NULL, R(DMA_CTRL), 0); wr(NULL, R(COMMON_CTRL), 0); fence(NULL);
    check(neon_isp_device_release(&io), 0xc10e);
    fx1_puts("[PASS] IRQ_PORT_PROBE: deferred status, PLIC complete, detach/re-register\n");
    fx1_pass();
}
int main(void)
{
    NEON_FX1_FreeRTOS_Init();
    io = (fx1_isp_io_t){NULL, rd, wr, fence};
    configASSERT(xTaskCreate(probe, "irq-probe", 1024, NULL, 2, &owner) == pdPASS);
    vTaskStartScheduler(); fx1_fail(0xc100);
}
