/**
 * @file main.c
 * @brief GT32U2086 application example.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "FreeRTOS.h"
#include "task.h"
#include "mcu_device.h"

#include <stdio.h>



/*==================================================================================================
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
 * configCHECK_FOR_STACK_OVERFLOW == 2 (see Inc/FreeRTOSConfig.h) forces the app to be of this level.
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



static void templates_freertos_v_main_task(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
        printf("__APP_NAME__ is running\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
