/**
 * @file freertos_peripheral_queue.c
 * @brief UART RX ISR / periodic task -> queue -> terminal task -> GPIOB7 and console.
 */

#include "freertos_peripheral_queue.h"
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "sdkconfig.h"
#include "gpio/neon_gpio.h"

#define EVENT_UART_RX 1U
#define EVENT_PERIODIC 2U
#define EVENT_STOP 3U
#define OUTPUT_PIN NEON_GPIO_PIN_7
#define PERIODIC_STACK_WORDS 128U
#define UART_TX_TIMEOUT_POLLS 1000000U

/* Each event owns complete aligned words, as required by FU2 SRAM. The queue
 * copies the value; no pointers to temporary ISR/task buffers are retained. */
typedef struct {
    uint32_t type;
    uint32_t value;
    uint32_t tick;
} peripheral_event_t;

static QueueHandle_t g_event_queue;
static TaskHandle_t g_periodic_task;
static neon_uart_handle_t *g_uart;
static volatile uint32_t g_running;
static volatile uint32_t g_stop_requested;
static volatile uint32_t g_uart_dropped;
static uint32_t g_periodic_dropped;
static uint32_t g_periodic_sequence;
static uint32_t g_processed;
static uint32_t g_output_state;

static void peripheral_queue_periodic(void *context);
static void peripheral_queue_print_stats(void);
static void peripheral_queue_start_run(void);
static void peripheral_queue_stop_run(void);
static void peripheral_queue_handle_command(uint32_t key);

/**
 * @brief Drain UART0 RX without waiting or printing from interrupt context.
 */
void USART0_IRQHandler(void) {
    BaseType_t higher_priority_task_woken = pdFALSE;
    uint32_t value = 0U;

    while (neon_uart_try_receive(g_uart, &value) == NEON_OK) {
        if (g_stop_requested != 0U) {
            continue; /* Discard the rest of a burst until the run stops. */
        }

        peripheral_event_t event = {
            .type = EVENT_UART_RX,
            .value = value,
            .tick = (uint32_t)xTaskGetTickCountFromISR(),
        };
        if (value == 3U) {
            /* The flag cannot be lost when the queue is full. A stop event
             * wakes a blocked receiver; a full queue already makes it ready. */
            g_stop_requested = 1U;
            event.type = EVENT_STOP;
            (void)xQueueSendFromISR(g_event_queue, &event, &higher_priority_task_woken);
        } else if (xQueueSendFromISR(g_event_queue, &event, &higher_priority_task_woken) != pdPASS) {
            ++g_uart_dropped;
        }
    }
    __DSB();
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

/** @copydoc free_rtos_peripheral_queue_init */
neon_status_t free_rtos_peripheral_queue_init(neon_uart_handle_t *uart) {
    if (uart == NULL || neon_uart_get_instance(uart) != NEON_UART0) {
        printf("[FAIL] An initialized UART0 handle is required.\n");
        return NEON_ERROR;
    }
    g_uart = uart;

    neon_status_t status = neon_gpio_clock_enable(GPIOB);
    if (status == NEON_OK) {
        neon_gpio_config_t config;
        neon_gpio_get_default_config(&config);
        config.pin = OUTPUT_PIN;
        config.mode = NEON_GPIO_MODE_OUTPUT;
        config.initial_state = NEON_GPIO_PIN_RESET;
        status = neon_gpio_init(GPIOB, &config);
    }
    if (status != NEON_OK) {
        printf("[FAIL] GPIOB7 initialization: status=%lu\n", (unsigned long)status);
        return NEON_ERROR;
    }

    NVIC_DisableIRQ(USART0_IRQn);
    status = neon_uart_disable_interrupt(g_uart, NEON_UART_INTERRUPT_ALL);
    /* Follow UART_RX_Interrupt: preserve the default printf UART settings and
     * enable its FIFO before receiving through interrupts. Startup is serialized:
     * no tasks or RX IRQs are running, and no printf is called during reconfiguration. */
    neon_uart_config_t uart_config;
    if (status == NEON_OK) {
        status = neon_uart_get_config(g_uart, &uart_config);
    }
    if (status == NEON_OK && uart_config.fifo_state != NEON_UART_FIFO_ENABLE) {
        status = neon_uart_wait_tx_complete(g_uart, UART_TX_TIMEOUT_POLLS);
        if (status == NEON_OK) {
            uart_config.fifo_state = NEON_UART_FIFO_ENABLE;
            status = neon_uart_set_config(g_uart, &uart_config);
        }
    }
    if (status == NEON_OK) {
        status = neon_uart_clear_rx_fifo(g_uart);
    }
    if (status != NEON_OK) {
        printf("[FAIL] UART RX initialization: status=%lu\n", (unsigned long)status);
        return NEON_ERROR;
    }

    g_event_queue = xQueueCreate(CONFIG_FREERTOS_PERIPHERAL_QUEUE_LENGTH, sizeof(peripheral_event_t));
    if (g_event_queue == NULL ||
        xTaskCreate(peripheral_queue_periodic, "queue_periodic", PERIODIC_STACK_WORDS, NULL,
                    tskIDLE_PRIORITY + 2U, &g_periodic_task) != pdPASS) {
        printf("[FAIL] Cannot allocate the event queue/tasks.\n");
        return NEON_ERROR;
    }

    return NEON_OK;
}

/* Fault hooks avoid console access: the failure may interrupt printf or leave
 * too little stack to format a message. Inspect the hook in the debugger. */
void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name) {
    (void)task;
    (void)task_name;
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

void vApplicationMallocFailedHook(void) {
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}

static void peripheral_queue_print_stats(void) {
    uint32_t uart_dropped;
    uint32_t periodic_dropped;
    taskENTER_CRITICAL();
    uart_dropped = g_uart_dropped;
    periodic_dropped = g_periodic_dropped;
    taskEXIT_CRITICAL();

    printf("[STATS] processed=%lu dropped=%lu (uart=%lu periodic=%lu) queued=%lu PB7=%lu\n",
           (unsigned long)g_processed, (unsigned long)(uart_dropped + periodic_dropped),
           (unsigned long)uart_dropped, (unsigned long)periodic_dropped,
           (unsigned long)uxQueueMessagesWaiting(g_event_queue), (unsigned long)g_output_state);
    printf("[RTOS] free_heap=%lu stack_free_words: terminal=%lu periodic=%lu\n",
           (unsigned long)xPortGetFreeHeapSize(), (unsigned long)uxTaskGetStackHighWaterMark(NULL),
           (unsigned long)uxTaskGetStackHighWaterMark(g_periodic_task));
}

static void peripheral_queue_start_run(void) {
    g_processed = 0U;
    g_output_state = 0U;
    neon_gpio_write_pin(GPIOB, OUTPUT_PIN, NEON_GPIO_PIN_RESET);
    taskENTER_CRITICAL();
    g_uart_dropped = 0U;
    g_periodic_dropped = 0U;
    g_periodic_sequence = 0U;
    g_running = 1U;
    taskEXIT_CRITICAL();

    printf("[READY] queue=%u events, period=%u ms, output=GPIOB7=0\n",
           (unsigned int)CONFIG_FREERTOS_PERIPHERAL_QUEUE_LENGTH,
           (unsigned int)CONFIG_FREERTOS_PERIPHERAL_QUEUE_PERIOD_MS);
    printf("1=high, 0=low, t=toggle, s=stats, Ctrl+C=stop\n");
}

static void peripheral_queue_stop_run(void) {
    NVIC_DisableIRQ(USART0_IRQn);
    (void)neon_uart_disable_interrupt(g_uart, NEON_UART_INTERRUPT_ALL);
    /* The producer suspends scheduling around its check/send, so it cannot
     * retain an unfinished send across this transition and queue reset. */
    g_running = 0U;
    (void)xQueueReset(g_event_queue);
    g_output_state = 0U;
    neon_gpio_write_pin(GPIOB, OUTPUT_PIN, NEON_GPIO_PIN_RESET);
    printf("\n[STOP] GPIOB7=0; pending events discarded.\n");
    peripheral_queue_print_stats();
    /* Return RX ownership to the polling terminal after discarding stop-time input. */
    (void)neon_uart_clear_rx_fifo(g_uart);
    NVIC_ClearPendingIRQ(USART0_IRQn);
    g_stop_requested = 0U;
}

static void peripheral_queue_handle_command(uint32_t key) {
    switch (key) {
        case '1':
            g_output_state = 1U;
            break;
        case '0':
            g_output_state = 0U;
            break;
        case 't':
            g_output_state ^= 1U;
            break;
        case 's':
            peripheral_queue_print_stats();
            return;
        case '\r':
        case '\n':
            return;
        default:
            printf("[HELP] 1=high, 0=low, t=toggle, s=stats, Ctrl+C=stop\n");
            return;
    }
    neon_gpio_write_pin(GPIOB, OUTPUT_PIN, g_output_state != 0U ? NEON_GPIO_PIN_SET : NEON_GPIO_PIN_RESET);
    printf("[UART] key='%c' -> PB7=%lu\n", (int)key, (unsigned long)g_output_state);
}

/** @copydoc free_rtos_peripheral_queue_run */
void free_rtos_peripheral_queue_run(neon_uart_handle_t *uart) {
    if (uart == NULL || uart != g_uart || g_event_queue == NULL) {
        printf("[FAIL] Initialize the example before running it.\n");
        return;
    }

    /* Called by the terminal task after r. It stops polling UART while this
     * function owns RX through the ISR and consumes events from the queue. */
    (void)xQueueReset(g_event_queue);
    g_stop_requested = 0U;
    neon_status_t status = neon_uart_enable_interrupt(g_uart,
        NEON_UART_INTERRUPT_RX_READY | NEON_UART_INTERRUPT_AUTO_CLEAR);
    if (status != NEON_OK) {
        printf("[FAIL] UART RX interrupt: status=%lu\n", (unsigned long)status);
        return;
    }
    peripheral_queue_start_run();
    NVIC_SetPriority(USART0_IRQn, (1U << __NVIC_PRIO_BITS) - 1U);
    NVIC_ClearPendingIRQ(USART0_IRQn);
    NVIC_EnableIRQ(USART0_IRQn);

    for (;;) {
        peripheral_event_t event;
        if (xQueueReceive(g_event_queue, &event, portMAX_DELAY) != pdPASS) {
            continue;
        }
        if (g_stop_requested != 0U) {
            peripheral_queue_stop_run();
            return;
        }
        ++g_processed;
        if (event.type == EVENT_PERIODIC) {
            printf("[PERIODIC] seq=%lu tick=%lu PB7=%lu\n", (unsigned long)event.value,
                   (unsigned long)event.tick, (unsigned long)g_output_state);
        } else if (event.type == EVENT_UART_RX) {
            peripheral_queue_handle_command(event.value);
        }
    }
}

static void peripheral_queue_periodic(void *context) {
    (void)context;
    TickType_t last_wake = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(CONFIG_FREERTOS_PERIPHERAL_QUEUE_PERIOD_MS);

    for (;;) {
        vTaskDelayUntil(&last_wake, period);
        /* Keep the run check, sequence update and zero-wait send together
         * relative to the terminal's start/stop. IRQs remain enabled. No blocking
         * APIs or peripheral operations are used while scheduling is suspended. */
        vTaskSuspendAll();
        if (g_running != 0U && g_stop_requested == 0U) {
            peripheral_event_t event = {
                .type = EVENT_PERIODIC,
                .value = ++g_periodic_sequence,
                .tick = (uint32_t)xTaskGetTickCount(),
            };
            if (xQueueSend(g_event_queue, &event, 0U) != pdPASS) {
                ++g_periodic_dropped;
            }
        }
        (void)xTaskResumeAll();
    }
}
