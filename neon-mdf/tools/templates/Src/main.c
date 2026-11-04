/**
 * @file main.c
 * @brief Implementation for templates.
 * @details
 * * Diem bat dau TOI GIAN: khong dung GPIO/LED gi ca, chi in ten app lap lai
 * qua printf() - proof-of-life de xac nhan pipeline mcu-description ->
 * CMSIS/RISC-V header/linker/startup -> firmware chay duoc end-to-end.
 * Xoa/sua/them code tuy y - day chi la diem bat dau, khong phai code bat
 * buoc giu lai.
 *
 * WEACT_F412 khong con retarget printf() sang UART; cac lenh printf()
 * tren target nay dung stub newlib/nosys. CH32V003F4P6 (RISC-V) KHONG dung duoc
 * printf() (toolchain bare-metal khong co libc that, xem
 * arch/RISCV/README.md) - #include <stdio.h> PHAI bao dieu kien (khong chi
 * cac lenh goi ham) neu khong se loi ngay o buoc preprocess.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include <stdint.h>

#if defined(FU2) || defined(WEACT_F412)
#include <stdio.h>
#endif

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void templates_delay_busy(volatile uint32_t count);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize the application and run its main loop.
 * @return The application normally runs indefinitely.
 */
int main(void) {
    for (;;) {
#if defined(FU2) || defined(WEACT_F412)
        printf("__APP_NAME__ dang chay\n");
#endif
        templates_delay_busy(2000000U);
    }
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static void templates_delay_busy(volatile uint32_t count) {
    while (count--) {
    }
}
