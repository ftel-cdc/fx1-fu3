/**
 * @file main.c
 * @brief Implementation for templates freertos.
 * @details
 * * Diem bat dau TOI GIAN cho app FreeRTOS: 1 task duy nhat, khong dung
 * GPIO/LED gi ca, chi in ten app lap lai qua printf() - proof-of-life de
 * xac nhan pipeline mcu-description -> CMSIS/RISC-V header/linker/startup
 * -> FreeRTOS-Kernel -> firmware chay duoc end-to-end (scheduler that su
 * chay, khong phai superloop nhu template thuong - xem
 * tools/templates/Src/main.c neu khong can FreeRTOS). Xoa/sua/them code
 * tuy y - day chi la diem bat dau, khong phai code bat buoc giu lai.
 *
 * WEACT_F412 khong con retarget printf() sang UART; cac lenh printf()
 * tren target nay dung stub newlib/nosys. CH32V003F4P6 (RISC-V) KHONG dung duoc
 * printf() (toolchain bare-metal khong co libc that, xem
 * arch/RISCV/README.md) - #include <stdio.h> PHAI bao dieu kien (khong chi
 * cac lenh goi ham) neu khong se loi ngay o buoc preprocess.
 *
 * GLUE ARCH-SPECIFIC BAT BUOC (xoa se lam scheduler treo/khong chay that su,
 * du VAN BUILD duoc - xem examples/FreeRTOS_Blinky/Src/main.c cho giai
 * thich day du + nguon tra cuu):
 *   - WEACT_F412 (port ARM_CM4F "Direct Routing"): can SVC_Handler/
 *     PendSV_Handler/SysTick_Handler nhay tuong minh sang vPortSVCHandler/
 *     xPortPendSVHandler/xPortSysTickHandler cua FreeRTOS - vector table
 *     sinh chung KHONG tu dat dung ten ham nay.
 *   - CH32V003F4P6 (port GCC_RISC_V_GENERIC, "no CLINT"): can tu cau hinh
 *     STK (SysTick that cua QingKe) sinh ngat tick + doi mtvec sang Direct
 *     mode - port RISC-V mac dinh gia dinh co CLINT (thanh ghi mtime that)
 *     ma chip nay khong co.
 *   - FU2 (port ARM_CM0): KHONG can gi them - weak-override ten ham chung
 *     (PendSV_Handler/SysTick_Handler) hoat dong tu nhien.
 *
 * CH32V003F4P6: bootloader la THAT (verify header/CRC32 luc boot, nhan
 * firmware moi qua UART) nhung KHONG co OTA (FreeRTOS-Kernel toi thieu da
 * chiem phan lon 16KB flash/2KB RAM that cua chip nay, khong con cho cho 2
 * ban image - xem examples/FreeRTOS_Blinky/Src/main.c cho so lieu that day
 * du + huong dan neu can tinh chinh Inc/FreeRTOSConfig.h khi app phinh to
 * hon template nay).
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "FreeRTOS.h"
#include "task.h"
#include "mcu_device.h"

#if defined(FU2) || defined(WEACT_F412)
#include <stdio.h>
#endif

#if defined(CH32V003F4P6)
#include "riscv_csr.h"
#include "sdk_irq.h"
#endif

#if defined(WEACT_F412)
/* Xem comment dau file - port ARM_CM4F dung "Direct Routing", can noi tuong
 * minh sang ten ham FreeRTOS thay vi dua vao weak-override ten chung.
 * KHONG dung __attribute__((alias(...))) - GCC doi ham duoc alias PHAI co
 * dinh nghia (than ham) trong CUNG 1 translation unit, ma vPortSVCHandler/
 * xPortPendSVHandler/xPortSysTickHandler nam trong port.c (thu vien
 * freertos_kernel_port khac) - loi that luc build: "aliased to undefined
 * symbol". vPortSVCHandler/xPortPendSVHandler la ham "naked" (tu quan ly
 * EXC_RETURN/stack de chuyen task - xem port.c) - PHAI nhay thang bang lenh
 * `b` (khong phai goi ham C binh thuong `bl`+tra ve) de giu nguyen ngu canh
 * exception nhu the duoc NVIC goi truc tiep. xPortSysTickHandler KHONG
 * naked - goi ham C binh thuong la du. */
void vPortSVCHandler(void);
void xPortPendSVHandler(void);
void xPortSysTickHandler(void);

/**
 * @brief Handle the SVC Handler interrupt.
 */
__attribute__((naked)) void SVC_Handler(void) {
    __asm volatile("b vPortSVCHandler");
}

/**
 * @brief Handle the PendSV Handler interrupt.
 */
__attribute__((naked)) void PendSV_Handler(void) {
    __asm volatile("b xPortPendSVHandler");
}

/**
 * @brief Handle the SysTick Handler interrupt.
 */
void SysTick_Handler(void) {
    xPortSysTickHandler();
}
#endif

#if defined(CH32V003F4P6)

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
/* STK (SysTick THAT cua QingKe V2A, dia chi/bit KHAC HAN CLINT mtime) - dia
 * chi/ten bit lay THAT tu QingKeV2 Processor Manual V1.0 chuong 4, DUNG
 * DUNG voi platform/ch32v003/neon_bootloader_arch.c (khong tai su dung macro tu do
 * vi la file bootloader-internal - o day tu khai bao lai, cung 1 nguon so
 * lieu). */
#define STK_CTLR (*(volatile uint32_t *)0xE000F000UL)
#define STK_SR (*(volatile uint32_t *)0xE000F004UL)
#define STK_CNTR (*(volatile uint32_t *)0xE000F008UL)
#define STK_CMPR (*(volatile uint32_t *)0xE000F010UL)

#define STK_CTLR_STE (1U << 0)   /* System counter enable */
#define STK_CTLR_STIE (1U << 1)  /* Counter interrupt enable */
#define STK_CTLR_STCLK (1U << 2) /* 1 = HCLK lam time base (khong chia 8) */
#define STK_CTLR_STRE (1U << 3)  /* Auto-reload (dem lai tu 0 khi toi CMPR) */

/**
 * @brief V port setup timer interrupt.
 * @details
 * vPortSetupTimerInterrupt() - ham "weak" cua port.c (RISC-V), app TU GHI
 * DE vi khong co CLINT that (xem comment dau file). Goi 1 LAN duy nhat, tu
 * xPortStartScheduler() TRUOC xPortStartFirstTask() - an toan de doi mtvec
 * o day (chua co ngat nao duoc bat).
 */
void vPortSetupTimerInterrupt(void) {
    extern void freertos_risc_v_trap_handler(void);

    /* mtvec Direct mode (bit[1:0]=00) - dia chi freertos_risc_v_trap_handler
   * da align 8 (`.align 8` trong portASM.S) nen 2 bit thap san = 0, khong
   * can OR them gi. TU DAY tro di, MOI exception/interrupt (ke ca ecall
   * cua portYIELD()) deu di qua ham nay thay vi vector table address-mode
   * cua startup_CH32V003F4P6.s. */
    RISCV_CSR_WRITE(mtvec, (uint32_t)freertos_risc_v_trap_handler);

    STK_CTLR = 0U; /* tat truoc khi cau hinh, giong pattern bootloader */
    STK_CNTR = 0U;
    STK_SR = 0U;
    STK_CMPR = (SystemCoreClock / configTICK_RATE_HZ) - 1U;
    SDK_IRQ_Enable(
        SysTick_IRQn); /* PFIC IENR - "STIE" chi bat ngat O STK, PFIC van phai duoc bat rieng (SysTick la "Programmable", khong phai "Fixed") */
    STK_CTLR = STK_CTLR_STE | STK_CTLR_STCLK | STK_CTLR_STRE | STK_CTLR_STIE;
}

/**
 * @brief Freertos risc v application interrupt handler.
 * @details
 * freertos_risc_v_application_interrupt_handler() - ham "weak" khac cua
 * portASM.S, duoc goi cho MOI asynchronous interrupt khi portasmHAS_MTIME=0
 * (xem Inc/FreeRTOSConfig.h) - port.c KHONG con tu nhan dien "day co phai
 * tick timer khong" giup minh nua, nen phai tu lam O DAY: doc mcause (IRQ
 * so 12 = SysTick), neu dung thi clear co pending + tang tick + chuyen task
 * giong het cach freertos_risc_v_mtimer_interrupt_handler goc lam cho chip
 * co CLINT.
 */
void freertos_risc_v_application_interrupt_handler(void) {
    uint32_t mcause = RISCV_CSR_READ(mcause);
    uint32_t code = mcause & 0x7FFFFFFFU; /* bit31 (interrupt) da chac =1 luc vao day */

    if (code == (uint32_t)SysTick_IRQn) {
        STK_SR = 0U; /* clear CNTIF (RM: ghi 0 = clear, ghi 1 = khong tac dung) */
        if (xTaskIncrementTick() != pdFALSE) {
            vTaskSwitchContext();
        }
    }
    /* IRQ khac (chua dung trong template nay) - bo qua, khong loop vo han nhu
   * default handler de khong "treo" oan neu co ngat tu peripheral khac. */
}
#endif /*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void templates_freertos_v_main_task(void *pvParameters);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize the application and run its main loop.
 * @return The application normally runs indefinitely.
 */
int main(void) {
    xTaskCreate(templates_freertos_v_main_task, "main", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    vTaskStartScheduler();

    /* Chi toi day neu het heap luc tao idle/timer task - khong bao gio xay ra
   * trong dieu kien binh thuong. */
    for (;;) {
    }
}

/**
 * @brief V application stack overflow hook.
 * @param[in] xTask X task.
 * @param[in,out] pcTaskName Pc task name.
 * @details
 * configCHECK_FOR_STACK_OVERFLOW == 2 (xem Inc/FreeRTOSConfig.h) bat buoc
 * app phai cung cap ham nay.
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName) {
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

/* CH32V003F4P6 */

static void templates_freertos_v_main_task(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
#if defined(FU2) || defined(WEACT_F412)
        printf("__APP_NAME__ dang chay\n");
#endif
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
