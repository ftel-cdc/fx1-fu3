/**
 * @file pwm_output.c
 * @brief Implementation for PWM output.
 * @details
 * Implements module behavior and target-specific integration.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "pwm_output.h"
#include <stdio.h>
#include <stdbool.h>
#include "pwm/neon_pwm.h"
#include "sdkconfig.h"

#ifndef NEON_MAX_DELAY

/*==================================================================================================
*                                        DEFINES AND MACROS
==================================================================================================*/
#define NEON_MAX_DELAY 0xFFFFFFFFU
#endif

#ifdef CONFIG_PWM_OUTPUT_TEST_PERIOD
#define TEST_PWM_PERIOD_TICKS ((uint32_t)CONFIG_PWM_OUTPUT_TEST_PERIOD)
#else
#define TEST_PWM_PERIOD_TICKS 1000U
#endif

#ifdef CONFIG_PWM_OUTPUT_TEST_PRESCALER
#define TEST_PWM_PRESCALER ((uint32_t)CONFIG_PWM_OUTPUT_TEST_PRESCALER)
#else
#define TEST_PWM_PRESCALER 15U
#endif

#define TEST_PWM_INSTANCE TIM0
#define TEST_PWM_CHANNEL NEON_PWM_CHANNEL_0

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void pwm_output_print_pwm_menu(void);
static void pwm_output_verify_hardware_readback(void);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/** @copydoc pwm_output_run */
void pwm_output_run(neon_uart_handle_t *uart) {
    neon_status_t status;
    neon_pwm_config_t pwm_config;

    printf("\n[INFO] Starting PWM Output Testcase (TIM0 channel 0)...\n");

    /* 1. Khoi tao PWM */
    neon_pwm_get_default_config(&pwm_config);
    pwm_config.period = TEST_PWM_PERIOD_TICKS;
    pwm_config.pulse = 0U;
    pwm_config.prescaler = TEST_PWM_PRESCALER;
    pwm_config.mode = NEON_PWM_MODE_0;
    pwm_config.counter_mode = NEON_PWM_COUNTERMODE_UP;

    status = neon_pwm_init(TEST_PWM_INSTANCE, TEST_PWM_CHANNEL, &pwm_config);
    if (status != NEON_OK) {
        printf("[ERROR] neon_pwm_init failed (Status = %d)\n", (int)status);
        return;
    }

    status = neon_pwm_start(TEST_PWM_INSTANCE, TEST_PWM_CHANNEL);
    if (status != NEON_OK) {
        printf("[ERROR] neon_pwm_start failed (Status = %d)\n", (int)status);
        neon_pwm_deinit(TEST_PWM_INSTANCE, TEST_PWM_CHANNEL);
        return;
    }

    printf("[SUCCESS] PWM Initialized & Started successfully on TIM0_CH0.\n");
    pwm_output_verify_hardware_readback();

    /* In Menu 1 lan duy nhat khi bat dau */
    pwm_output_print_pwm_menu();

    /* 2. Vong lap tuong tac */
    bool running_test = true;
    while (running_test) {
        printf("Select option > ");

        uint32_t rx_word = 0U;
        status = neon_uart_receive(uart, (uint8_t *)&rx_word, 1U, NEON_MAX_DELAY);
        if (status != NEON_OK) {
            continue;
        }

        uint8_t rx_byte = (uint8_t)(rx_word & 0xFFU);

        /* Bo qua ky tu xuong dong / ve dau dong */
        if (rx_byte == '\r' || rx_byte == '\n') {
            printf("\n");
            continue;
        }

        printf("%c\n", rx_byte);

        switch (rx_byte) {
            case '0':
                status = neon_pwm_set_pulse(TEST_PWM_INSTANCE, TEST_PWM_CHANNEL, 0U);
                if (status == NEON_OK) {
                    printf("[ACTION] Set Duty Cycle -> 0%%\n");
                    pwm_output_verify_hardware_readback();
                }
                break;

            case '1':
                status = neon_pwm_set_pulse(TEST_PWM_INSTANCE, TEST_PWM_CHANNEL, TEST_PWM_PERIOD_TICKS / 4U);
                if (status == NEON_OK) {
                    printf("[ACTION] Set Duty Cycle -> 25%%\n");
                    pwm_output_verify_hardware_readback();
                }
                break;

            case '2':
                status = neon_pwm_set_pulse(TEST_PWM_INSTANCE, TEST_PWM_CHANNEL, TEST_PWM_PERIOD_TICKS / 2U);
                if (status == NEON_OK) {
                    printf("[ACTION] Set Duty Cycle -> 50%%\n");
                    pwm_output_verify_hardware_readback();
                }
                break;

            case '3':
                status = neon_pwm_set_pulse(TEST_PWM_INSTANCE, TEST_PWM_CHANNEL, (TEST_PWM_PERIOD_TICKS * 3U) / 4U);
                if (status == NEON_OK) {
                    printf("[ACTION] Set Duty Cycle -> 75%%\n");
                    pwm_output_verify_hardware_readback();
                }
                break;

            case '4':
                status = neon_pwm_set_pulse(TEST_PWM_INSTANCE, TEST_PWM_CHANNEL, TEST_PWM_PERIOD_TICKS);
                if (status == NEON_OK) {
                    printf("[ACTION] Set Duty Cycle -> 100%%\n");
                    pwm_output_verify_hardware_readback();
                }
                break;

            case 's':
            case 'S':
                status = neon_pwm_stop(TEST_PWM_INSTANCE, TEST_PWM_CHANNEL);
                if (status == NEON_OK) {
                    printf("[ACTION] Stopped PWM Output.\n");
                    pwm_output_verify_hardware_readback();
                }
                break;

            case 'r':
            case 'R':
                status = neon_pwm_start(TEST_PWM_INSTANCE, TEST_PWM_CHANNEL);
                if (status == NEON_OK) {
                    printf("[ACTION] Resumed/Started PWM Output.\n");
                    pwm_output_verify_hardware_readback();
                }
                break;

            case 'q':
            case 'Q':
            case 0x03:
                printf("[ACTION] Exiting PWM Testcase...\n");
                running_test = false;
                break;

            default:
                printf("[WARN] Invalid option '%c'. Please select [0-4, s, r, q].\n\n", rx_byte);
                break;
        }
    }

    /* 3. Cleanup */
    neon_pwm_stop(TEST_PWM_INSTANCE, TEST_PWM_CHANNEL);
    neon_pwm_deinit(TEST_PWM_INSTANCE, TEST_PWM_CHANNEL);
    printf("[INFO] PWM DeInitialized. TIM0_CH0 is released safely.\n\n");
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static void pwm_output_print_pwm_menu(void) {
    printf("\n=================== 5P1 PWM OUTPUT TEST MENU ===================\n");
    printf("  [0] Set Duty 0%%   (pulse = 0, Force LOW)\n");
    printf("  [1] Set Duty 25%%  (pulse = 250)\n");
    printf("  [2] Set Duty 50%%  (pulse = 500)\n");
    printf("  [3] Set Duty 75%%  (pulse = 750)\n");
    printf("  [4] Set Duty 100%% (pulse = 1000, Force HIGH)\n");
    printf("  [s] Stop PWM Output (Check Inactive state)\n");
    printf("  [r] Start/Resume PWM Output\n");
    printf("  [q] Quit Testcase (Stop & DeInit)\n");
    printf("================================================================\n");
}

static void pwm_output_verify_hardware_readback(void) {
    uint32_t raw_chl = TIM0->CHL_CH0;   /* Thanh ghi chu ky period thuc */
    uint32_t raw_ccr1 = TIM0->CCR1_CH0; /* Thanh ghi so sanh pulse thuc */
    uint32_t raw_prsc = TIM0->PRSC_CH0; /* Thanh ghi chia tan prescaler thuc */
    uint32_t raw_cnt = TIM0->CNT_CH0;   /* Bo dem thoi gian thuc đang chay */
    uint32_t raw_ctrl = TIM0->CTRL_CH0; /* Thanh ghi dieu khien CTRL thuc */

    uint32_t mode_bits = raw_ctrl & TIM_CTRL_CH0_TIMER_MODE_Msk;
    bool is_hw_running = (mode_bits == ((uint32_t)NEON_PWM_MODE_0 << TIM_CTRL_CH0_TIMER_MODE_Pos)) ||
                         (mode_bits == ((uint32_t)NEON_PWM_MODE_1 << TIM_CTRL_CH0_TIMER_MODE_Pos));

    uint32_t duty_calc = (raw_chl > 0U) ? ((raw_ccr1 * 100U) / raw_chl) : 0U;

    printf("\n[RAW HW REG AUDIT]\n");
    printf("  -> TIM0->CTRL_CH0  = 0x%08lX (HW mode Bit: %s)\n", (unsigned long)raw_ctrl,
           is_hw_running ? "RUNNING" : "STOPPED");
    printf("  -> TIM0->CHL_CH0   = %lu (RAW period Register)\n", (unsigned long)raw_chl);
    printf("  -> TIM0->CCR1_CH0  = %lu (RAW pulse Register)\n", (unsigned long)raw_ccr1);
    printf("  -> TIM0->PRSC_CH0  = %lu (RAW prescaler Register)\n", (unsigned long)raw_prsc);
    printf("  -> TIM0->CNT_CH0   = %lu (RAW Live Counter)\n", (unsigned long)raw_cnt);
    printf("  -> Physical Duty   = %lu%%\n\n", (unsigned long)duty_calc);
}