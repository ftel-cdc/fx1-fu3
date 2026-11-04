/**
 * @file main.c
 * @brief Implementation for freertos blinky.
 * @details
 * * FreeRTOS_Blinky - 2 task toggle 2 pin GPIO khac chu ky, chung minh
 * scheduler preemptive - chay duoc tren CA 3 TARGET: FU2 (Cortex-M0, port
 * GCC_ARM_CM0), WEACT_F412 (Cortex-M4F, port GCC_ARM_CM4F) VA CH32V003F4P6
 * (RISC-V QingKe V2A, port GCC_RISC_V_GENERIC - them 2026-08-10, target
 * RISC-V dau tien co FreeRTOS) - xem cmake/freertos.cmake cho cach chon
 * port theo arch.
 *
 * FU2 (port ARM_CM0): PendSV_Handler/SysTick_Handler manh (strong) trong
 * portasm.c/port.c cua FreeRTOS DUNG DUNG TEN voi weak handler trong
 * startup_FU2.s - tu ghi de luc link, khong can glue code gi them.
 *
 * WEACT_F412 (port ARM_CM4F): NGUOC LAI - port nay dung "Direct Routing",
 * dat ten ham la vPortSVCHandler/xPortPendSVHandler/xPortSysTickHandler
 * (KHAC ten voi vector table sinh chung SVC_Handler/PendSV_Handler/
 * SysTick_Handler trong startup_WEACT_F412.s) - weak-override KHONG tu xay
 * ra vi ten khac nhau. Can alias tuong minh (xem duoi) - thieu buoc nay,
 * xPortStartScheduler() se treo vinh vien tai configASSERT kiem tra vector
 * table (pxVectorTable[SVC] == vPortSVCHandler) that bai - da xac nhan that
 * bang cach doc PC qua SWD dung tai xPortStartScheduler luc debug.
 *
 * WEACT_F412: LED1_PIN (PB3) khong noi ra ngoai (khong co LED that tren
 * board) - van du de chung minh preemption (2 task chay dong thoi,
 * neon_gpio_toggle_pin tren 2 pin khac nhau voi chu ky khac nhau), chi khong
 * quan sat duoc bang mat nhu PB2 (LED that).
 *
 * CH32V003F4P6 (port GCC_RISC_V_GENERIC): QingKe V2A KHONG co CLINT (Core
 * Local Interrupter chuan cua RISC-V ma port nay mac dinh gia dinh) - dung
 * PFIC (Programmable Fast interrupt Controller) rieng cua WCH, da xac nhan
 * THAT qua QingKeV2 Processor Manual V1.0 (WCH chinh thuc, tai truc tiep tu
 * ch405-labs.com luc lam viec nay - KHONG doan):
 *   - mtvec[1:0]: bit0=0 la "Direct mode" (TAT CA exception/interrupt deu
 *     nhay ve 1 dia chi duy nhat = freertos_risc_v_trap_handler cua
 *     portASM.S, phan biet nguyen nhan qua mcause) - KHAC voi "address
 *     mode" (mtvec[1:0]=3, vector table rieng tung IRQ) ma
 *     startup_CH32V003F4P6.s (dung chung cho MOI app tren target nay, ke ca
 *     Blinky/bootloader) cau hinh SAN luc reset. App FreeRTOS nay TU GHI
 *     DE mtvec sang Direct mode trong vPortSetupTimerInterrupt() duoi day
 *     (goi 1 lan, TRUOC xPortStartFirstTask()/truoc khi ngat duoc bat) -
 *     KHONG dung toi/sua startup.s dung chung.
 *   - mcause[30:0] khi la ngat (mcause[31]=1) CHINH LA so IRQ PFIC that (vd
 *     12 = SysTick) - xac nhan qua manual (Table 2-1 "V2 microprocessor
 *     exception codes"): "12 - Precise asynchronous - SysTick interrupts",
 *     "11 - Synchronous - Environment call in Machine mode" (khop CHINH XAC
 *     voi `li t0, 11` co san trong portASM.S cho ecall/portYIELD() - KHONG
 *     can sua portASM.S).
 *   - Tick timer: STK (SysTick THAT cua QingKe, dia chi/bit KHAC HAN CLINT
 *     mtime - xem Chuong 4 manual) tu cau hinh + bat ngat qua PFIC IENR
 *     (SDK_IRQ_Enable(SysTick_IRQn)) trong vPortSetupTimerInterrupt() (ham
 *     "weak" cua port.c, app ghi de). freertos_risc_v_application_interrupt_handler()
 *     (ham "weak" khac cua portASM.S) tu doc mcause de nhan dien dung
 *     SysTick roi goi xTaskIncrementTick()/vTaskSwitchContext() THAY vi
 *     dua vao logic portasmHAS_MTIME cua port.c (khong dung duoc, portasmHAS_MTIME=0
 *     vi khong co CLINT that - xem FreeRTOSConfig.h).
 *   - portYIELD() (ecall, dung boi vTaskDelay/... bat cu luc nao can
 *     chuyen task NGOAI ngat) hoat dong HOAN TOAN DOC LAP voi PFIC/CLINT
 *     (la synchronous exception chuan RISC-V, khong phai interrupt) - KHONG
 *     can code gi them.
 *   - CHUA verify that tren board (board RISC-V dat shopee chua ve, xem
 *     memory FU2 hardware status) - moi thu tren la THIET KE dua tren tai
 *     lieu chinh thuc THAT (khong doan), se xac nhan lai o Phase 4/5 (xem
 *     .claude/plans/proud-sniffing-teacup.md) khi co board.
 *
 * OTA (WEACT_F412 CHI): app nay gio duoc BUILD 2 LAN tu CUNG 1 source (xem
 * CMakeLists.txt) - "freertos_blinky" (factory, NEON_OTA_APP_VERSION=1) va
 * "freertos_blinky_ota" (slot ota, NEON_OTA_APP_VERSION=2) - xem
 * examples/OTA_Demo/ cho vi du OTA day du hon. App nay KHONG co UART/printf
 * nen dung chu ky vLed1Task LAM DAU HIEU phan biet version (do duoc qua doc
 * thanh ghi IDR qua SWD, giong cach da lam de xac nhan preemption that tren
 * phan cung WEACT_F412 truoc do). Sau ~4s chay on dinh (20 vong vLed0Task,
 * chu ky 200ms), tu goi neon_ota_confirm_valid().
 *
 * FU2 CHUA co bootloader/OTA (giu nguyen build don). CH32V003F4P6 (them
 * 2026-08-10) CO bootloader THAT (verify header/CRC32 luc boot, nhan
 * firmware moi qua UART) NHUNG KHONG co slot OTA/neon_ota_confirm_valid() -
 * FreeRTOS-Kernel toi thieu da chiem ~8.5KB flash/~2KB RAM (do THAT tu
 * build, sau khi da cat het muc co the: heap 1.25KB, stack/task 256B,
 * TAT timer service task), khong the fit 2 ban image (factory+ota) +
 * bootloader trong 16KB flash/2KB RAM that cua chip - xem
 * CMakeLists.txt/flashmap.WEACT_F412.csv va .claude/plans/proud-sniffing-teacup.md.
 * NEON_OTA_APP_VERSION=1 (dinh nghia trong CMakeLists.txt) tren
 * CH32V003F4P6 CHI dung de tinh period_ms cua vLed1Task (khong mang y
 * nghia "phien ban OTA" nhu WEACT_F412).
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "FreeRTOS.h"
#include "task.h"

#include "gpio/neon_gpio.h"
#include "mcu_device.h"

#if defined(WEACT_F412)
/* neon_ota.h: CH32V003F4P6 KHONG dung (khong co slot OTA - xem
 * CMakeLists.txt/flashmap.WEACT_F412.csv cho ly do day du: FreeRTOS-Kernel
 * toi thieu da ~8.5KB flash, khong the fit 2 ban image trong 16KB that cua
 * chip). CH32V003F4P6 van co bootloader THAT (verify header/CRC32), chi
 * KHONG co khai niem "OTA slot" de goi neon_ota_confirm_valid(). */
#include "neon_ota.h"
#endif

#if defined(CH32V003F4P6)
#include "riscv_csr.h"
#include "sdk_irq.h"
#endif

#ifndef NEON_OTA_APP_VERSION

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define NEON_OTA_APP_VERSION 1
#endif

#if defined(FU2)
/* GPIO0 (bản cũ) không còn tồn tại - FU2.yaml giờ khai đúng 4 instance thật
 * GPIOA-D (theo inc_fu2/fu2.h), mỗi port 8 pin. Chưa có board thật/pin_map.md
 * nên chọn tạm GPIOA pin 0/1 (arbitrary, không phải LED thật nào đã xác nhận). */
#define LED_INSTANCE GPIOA
#define LED0_PIN (1U << 0)
#define LED1_PIN (1U << 1)
#elif defined(WEACT_F412)
#define LED_INSTANCE GPIOB
#define LED0_PIN (1U << 2) /* PB2 = LED that onboard */
#define LED1_PIN (1U << 3) /* PB3 = khong noi LED, chi de chung minh preemption */
#elif defined(CH32V003F4P6)
#define LED_INSTANCE GPIOD
#define LED0_PIN (1U << 1) /* PD1 = LED that (xem Blinky/Src/main.c) */
#define LED1_PIN                                                                                                       \
    (1U                                                                                                                \
     << 0) /* PD0 = khong noi LED, chi de chung minh preemption - CHUA xac nhan pin nay ra chan that tren board (chua ve) */
#else
#error "FreeRTOS_Blinky: chua dinh nghia LED_INSTANCE/LED0_PIN/LED1_PIN cho target nay"
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
/* STK (SysTick THAT cua QingKe V2A, dia chi/bit KHAC HAN CLINT mtime) - dia
 * chi/ten bit lay THAT tu QingKeV2 Processor Manual V1.0 chuong 4, DUNG
 * DUNG voi platform/ch32v003/neon_bootloader_arch.c (khong tai su dung macro tu do
 * vi la file bootloader-internal, xem comment dau file do - o day tu khai
 * bao lai, cung 1 nguon so lieu). */
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
        SysTick_IRQn); /* PFIC IENR - "STIE" chi bat ngat O STK, PFIC van phai duoc bat rieng (xem Table 3-1 manual: SysTick la "Programmable", khong phai "Fixed") */
    STK_CTLR = STK_CTLR_STE | STK_CTLR_STCLK | STK_CTLR_STRE | STK_CTLR_STIE;
}

/**
 * @brief Freertos risc v application interrupt handler.
 * @details
 * freertos_risc_v_application_interrupt_handler() - ham "weak" khac cua
 * portASM.S, duoc goi cho MOI asynchronous interrupt khi portasmHAS_MTIME=0
 * (xem FreeRTOSConfig.h) - port.c KHONG con tu nhan dien "day co phai tick
 * timer khong" giup minh nua (logic do chi ton tai trong nhanh
 * portasmHAS_MTIME!=0), nen phai tu lam O DAY: doc mcause (Table 2-1 manual:
 * IRQ so 12 = SysTick), neu dung thi clear co pending + tang tick +
 * chuyen task giong het cach freertos_risc_v_mtimer_interrupt_handler goc
 * lam cho chip co CLINT.
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
    /* IRQ khac (chua dung trong demo nay) - bo qua, khong loop vo han nhu
   * default handler (`j .` trong portASM.S) de khong "treo" oan neu co ngat
   * la tu peripheral khac bi bat nham. */
}
#endif /*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void freertos_blinky_v_led0_task(void *pvParameters);
static void freertos_blinky_v_led1_task(void *pvParameters);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize the application and run its main loop.
 * @return The application normally runs indefinitely.
 */
int main(void) {
    neon_gpio_config_t leds_init = {
        .pin = LED0_PIN | LED1_PIN,
        .mode = NEON_GPIO_MODE_OUTPUT,
    };
    neon_gpio_clock_enable(LED_INSTANCE);
    neon_gpio_init(LED_INSTANCE, &leds_init);

    xTaskCreate(freertos_blinky_v_led0_task, "led0", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(freertos_blinky_v_led1_task, "led1", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

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
 * configCHECK_FOR_STACK_OVERFLOW == 2 (xem FreeRTOSConfig.h) bat buoc app
 * phai cung cap ham nay.
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

static void freertos_blinky_v_led0_task(void *pvParameters) {
    (void)pvParameters;
#if defined(WEACT_F412)
    uint32_t loop_count = 0;
#endif
    for (;;) {
        neon_gpio_toggle_pin(LED_INSTANCE, LED0_PIN);
        vTaskDelay(pdMS_TO_TICKS(200));

#if defined(WEACT_F412)
        loop_count++;
        if (loop_count == 20U) { /* ~4s da chay on dinh */
            neon_ota_confirm_valid();
        }
#endif
    }
}

static void freertos_blinky_v_led1_task(void *pvParameters) {
    (void)pvParameters;
    /* Chu ky LED1 lam dau hieu phan biet version (khong co UART/printf o app
   * nay) - v1=500ms, v2=250ms - do duoc qua SWD (mdw GPIOB->IDR nhieu lan
   * lien tiep, xem plan file). */
    const TickType_t period_ms = (NEON_OTA_APP_VERSION == 1) ? 500U : 250U;
    for (;;) {
        neon_gpio_toggle_pin(LED_INSTANCE, LED1_PIN);
        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
}
