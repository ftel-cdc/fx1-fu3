/**
 * @file uart_rx_interrupt.c
 * @brief PC -> UART FIFO -> ISR -> ring buffer -> main -> echo.
 */

#include "uart_rx_interrupt.h"
#include "sdkconfig.h"
#include <stdio.h>

#define RX_BUFFER_SIZE CONFIG_UART_RX_INTERRUPT_RING_BUF_SIZE
#define TX_TIMEOUT CONFIG_UART_RX_INTERRUPT_TX_TIMEOUT_POLLS

/* FU2 lưu mỗi byte trong một word 32-bit. */
static uint32_t g_rx_buffer[RX_BUFFER_SIZE];
static volatile uint32_t g_rx_head;
static volatile uint32_t g_rx_tail;
static volatile uint32_t g_rx_count;
static volatile uint32_t g_stop_requested;
static volatile uint32_t g_main_work;
static neon_uart_handle_t *volatile g_active_uart_handle;

void USART0_IRQHandler(void) {
    uint32_t value = 0U;

    /* Đọc các byte đang có trong FIFO, không chờ byte mới. */
    while (neon_uart_try_receive(g_active_uart_handle, &value) == NEON_OK) {
        if (value == 3U) { /* Ctrl+C vẫn nhận được khi ring đầy. */
            g_stop_requested = 1U;
            continue;
        }

        /* Ring đầy thì bỏ byte mới; vẫn tiếp tục đọc FIFO. */
        if (g_stop_requested == 0U && g_rx_count < RX_BUFFER_SIZE) {
            g_rx_buffer[g_rx_head] = value;
            g_rx_head = (g_rx_head + 1U) % RX_BUFFER_SIZE;
            ++g_rx_count;
        }
    }
    __DSB();
}

void uart_rx_interrupt_run(neon_uart_handle_t *uart, uint32_t skip_initial_lf) {
    uint32_t saved_primask = __get_PRIMASK();
    neon_uart_config_t saved_config;
    neon_uart_config_t config;

    /* 1. UART0 đã được terminal khởi tạo. Bật thêm FIFO và ngắt RX. */
    neon_uart_get_config(uart, &saved_config);
    config = saved_config;
    config.fifo_state = NEON_UART_FIFO_ENABLE;

    neon_uart_wait_tx_complete(uart, TX_TIMEOUT);
    NVIC_DisableIRQ(USART0_IRQn);
    neon_uart_set_config(uart, &config);
    neon_uart_clear_rx_fifo(uart);

    g_rx_head = 0U;
    g_rx_tail = 0U;
    g_rx_count = 0U;
    g_stop_requested = 0U;
    g_main_work = 0U;
    g_active_uart_handle = uart;

    neon_uart_enable_interrupt(uart, NEON_UART_INTERRUPT_RX_READY | NEON_UART_INTERRUPT_AUTO_CLEAR);
    NVIC_ClearPendingIRQ(USART0_IRQn);
    NVIC_EnableIRQ(USART0_IRQn);
    __enable_irq();
    printf("[READY] UART RX interrupt. Type to echo; Ctrl+C returns.\n");

    /* 2. Main làm việc khác rồi lấy từng byte trong ring ra để echo. */
    while (g_stop_requested == 0U || g_rx_count != 0U) {
        ++g_main_work; /* Quan sát trong debugger: vẫn tăng khi chưa gửi UART. */
        uint32_t value = 0U;
        uint32_t has_data = 0U;

        /* ISR và main cùng cập nhật count nên cần bảo vệ đoạn lấy dữ liệu. */
        uint32_t primask = __get_PRIMASK();
        __disable_irq();
        if (g_rx_count != 0U) {
            value = g_rx_buffer[g_rx_tail];
            g_rx_tail = (g_rx_tail + 1U) % RX_BUFFER_SIZE;
            --g_rx_count;
            has_data = 1U;
        }
        __set_PRIMASK(primask);

        if (has_data == 0U) {
            continue;
        }
        if (skip_initial_lf != 0U) {
            skip_initial_lf = 0U;
            if (value == '\n') {
                continue; /* Bỏ LF còn dư của lệnh terminal kết thúc bằng CRLF. */
            }
        }
        /* Log ở main: byte đã đi từ FIFO qua ring, ISR không in log. */
        int display = (value >= 0x20U && value <= 0x7EU) ? (int)value : '.';
        printf("[PC -> FPGA] '%c' (0x%02X)\n", display, (unsigned int)value);
        printf("[FPGA -> PC]  ");
        neon_uart_transmit(uart, (const uint8_t *)&value, 1U, TX_TIMEOUT);
        printf("  (0x%02X)\n", (unsigned int)value);
    }

    /* 3. Ctrl+C: tắt ngắt RX, khôi phục UART rồi trả về terminal. */
    NVIC_DisableIRQ(USART0_IRQn);
    neon_uart_disable_interrupt(uart, NEON_UART_INTERRUPT_RX_READY);
    g_active_uart_handle = NULL;
    neon_uart_wait_tx_complete(uart, TX_TIMEOUT);
    neon_uart_set_config(uart, &saved_config);
    NVIC_ClearPendingIRQ(USART0_IRQn);
    __set_PRIMASK(saved_primask);
    printf("\n[INFO] UART RX interrupt stopped.\n");
}
