/**
 * @file main.c
 * @brief GT32U2086 application example.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "FreeRTOS.h"
#include "task.h"

#include "gpio/neon_gpio.h"
#include "mcu_device.h"




/* The former GPIO0 no longer exists. GT32U2086.yaml declares the four actual GPIOA-D instances from
 * inc_gt32u2086/gt32u2086.h, with eight pins per port. GPIOA pins 0/1 are provisional choices pending hardware
 * and pin_map.md verification, not confirmed LED pins. */
#define LED_INSTANCE GPIOA
#define LED0_PIN (1U << 0)
#define LED1_PIN (1U << 1)



/*==================================================================================================
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

    /* Reached only if creating the idle/timer task exhausts the heap; not expected during normal
   * operation. */
    for (;;) {
    }
}

/**
 * @brief V application stack overflow hook.
 * @param[in] xTask X task.
 * @param[in,out] pcTaskName Pc task name.
 * @details
 * configCHECK_FOR_STACK_OVERFLOW == 2 (FreeRTOSConfig.h) requires the application to provide this
 * hook.
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



/**
 * @brief Run the first periodic LED task.
 * @param[in] pvParameters Unused task context.
 */
static void freertos_blinky_v_led0_task(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
        neon_gpio_toggle_pin(LED_INSTANCE, LED0_PIN);
        vTaskDelay(pdMS_TO_TICKS(200));

    }
}

/**
 * @brief Run the second periodic LED task.
 * @param[in] pvParameters Unused task context.
 */
static void freertos_blinky_v_led1_task(void *pvParameters) {
    (void)pvParameters;
    const TickType_t period_ms = 500U;
    for (;;) {
        neon_gpio_toggle_pin(LED_INSTANCE, LED1_PIN);
        vTaskDelay(pdMS_TO_TICKS(period_ms));
    }
}
