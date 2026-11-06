/**
 * @file main.c
 * @brief Implementation for fpga terminal.
 * @details
 * UART command terminal and hardware testcases for FU2 FPGA.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include "main.h"

#ifndef CONFIG_NEON_MDF_DRIVER_UART
#error "FPGA_Terminal requires CONFIG_NEON_MDF_DRIVER_UART=y"
#endif

/*==================================================================================================
*                                  STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef void (*terminal_command_handler_t)(void);

typedef struct {
    const char *name;
    const char *description;
    terminal_command_handler_t handler;
} terminal_command_t;

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static void fpga_terminal_command_help(void);
static void fpga_terminal_command_info(void);
static void fpga_terminal_command_clear(void);
static void fpga_terminal_command_uart_echo(void);
static void fpga_terminal_command_gpio_io(void);
static void fpga_terminal_command_gpio_irq(void);
static void fpga_terminal_command_tim_irq(void);
static void fpga_terminal_command_flash_rw(void);
static void fpga_terminal_command_uart_rx_interrupt(void);
static void fpga_terminal_command_uart_dma(void);
static void fpga_terminal_command_pwm_output(void);
static void fpga_terminal_command_i2c_rw(void);
static void fpga_terminal_command_spi_w5500(void);
static void fpga_terminal_command_adc_sampling(void);
static void fpga_terminal_command_dma_memcpy(void);
static void fpga_terminal_command_rtc_calendar(void);
static void fpga_terminal_command_crc_vector(void);
static void fpga_terminal_command_aes_vector(void);
static void fpga_terminal_command_trng_read(void);
static void fpga_terminal_command_qspi_flash(void);
static void fpga_terminal_command_i2c_slave(void);
static void fpga_terminal_command_spi_dma(void);
static void fpga_terminal_command_i2c_dma(void);
static void fpga_terminal_command_freertos_queue(void);
static void fpga_terminal_terminal_prompt(void);
static void fpga_terminal_terminal_dispatch(char *line);
static void fpga_terminal_terminal_reset_line(void);
static void fpga_terminal_terminal_process_character(uint32_t value);

/*==================================================================================================
*                                         LOCAL VARIABLES
==================================================================================================*/
/* help and dispatch share this table. Add future testcase commands here. */
static const terminal_command_t commands[] = {
    {"help", "Show available commands", fpga_terminal_command_help},
    {"info", "Show terminal configuration", fpga_terminal_command_info},
    {"clear", "Clear the terminal screen (ANSI)", fpga_terminal_command_clear},
    {"uart_echo", "UART0 polling echo; Ctrl+C returns to terminal", fpga_terminal_command_uart_echo},
    {"gpio_io", "PB5 button -> PB7 output; Ctrl+C returns to terminal", fpga_terminal_command_gpio_io},
    {"gpio_irq", "PB5 rising-edge IRQ counter; 'i' injects; Ctrl+C returns", fpga_terminal_command_gpio_irq},
    {"tim_irq", "TIM1 500 ms IRQ -> PB7 LED blink 1 Hz; Ctrl+C returns", fpga_terminal_command_tim_irq},
    {"flash_rw", "Erase/program/verify reserved Flash sector at 0x0009FE00", fpga_terminal_command_flash_rw},
    {"uart_rx_irq", "UART0 FIFO RX interrupt + main echo; Ctrl+C returns", fpga_terminal_command_uart_rx_interrupt},
    {"uart_dma", "UART0 DMA: t=TX64, r=RX16, q=return", fpga_terminal_command_uart_dma},
    {"pwm_output", "TIM0_CH0 PWM output; Ctrl+C returns to terminal", fpga_terminal_command_pwm_output},
    {"i2c_rw", "[TODO] I2C register read/write", fpga_terminal_command_i2c_rw},
    {"spi_w5500", "SPI0 W5500 register read/write + burst; Ctrl+C returns", fpga_terminal_command_spi_w5500},
    {"adc_sampling", "[TODO] ADC sampling", fpga_terminal_command_adc_sampling},
    {"dma_memcpy", "[TODO] DMA memory copy", fpga_terminal_command_dma_memcpy},
    {"rtc_calendar", "[TODO] RTC calendar", fpga_terminal_command_rtc_calendar},
    {"crc_vector", "[TODO] CRC known vector", fpga_terminal_command_crc_vector},
    {"aes_vector", "[TODO] AES known vector", fpga_terminal_command_aes_vector},
    {"trng_read", "[TODO] TRNG random data", fpga_terminal_command_trng_read},
    {"qspi_flash", "[TODO] QSPI Flash indirect access", fpga_terminal_command_qspi_flash},
    {"i2c_slave", "[TODO] I2C slave", fpga_terminal_command_i2c_slave},
    {"spi_dma", "[TODO] SPI DMA", fpga_terminal_command_spi_dma},
    {"i2c_dma", "[TODO] I2C DMA", fpga_terminal_command_i2c_dma},
    {"freertos_queue", "[TODO] FreeRTOS peripheral queue", fpga_terminal_command_freertos_queue},
};

static neon_uart_handle_t *terminal_uart;

/* Own complete aligned words, including the last byte and NUL terminator. */
static uint32_t line_storage[(CONFIG_FPGA_TERMINAL_LINE_SIZE + 3U) / 4U];
static uint32_t line_length;
static uint32_t discard_line;
static uint32_t skip_lf;
static uint32_t escape_state;

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief Initialize the application and run its main loop.
 * @return The application normally runs indefinitely.
 */
int main(void) {
    /* printf initializes the default UART0; reuse that handle for input. */
    printf("\nFU2 FPGA terminal\n");
    terminal_uart = neon_console_get_uart();
    if (terminal_uart == NULL) {
        for (;;) {
            __WFI();
        }
    }

    printf("Type 'help' to list commands.\n");
    fpga_terminal_terminal_prompt();

    for (;;) {
        uint32_t receive_word = 0U;
        neon_status_t status =
            neon_uart_receive(terminal_uart, (uint8_t *)&receive_word, 1U, CONFIG_FPGA_TERMINAL_RX_TIMEOUT_POLLS);
        if (status == NEON_TIMEOUT) {
            continue;
        }
        if (status != NEON_OK) {
            /* Lost/corrupted bytes must not turn the rest of a line into a command. */
            if (discard_line == 0U) {
                printf("\n[ERROR] UART RX status=%lu error=0x%08lX; line discarded.\n", (unsigned long)status,
                       (unsigned long)neon_uart_get_error(terminal_uart));
            }
            discard_line = 1U;
            neon_uart_clear_error(terminal_uart);
            continue;
        }
        fpga_terminal_terminal_process_character(receive_word & 0xFFU);
    }
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

static void fpga_terminal_terminal_prompt(void) {
    printf("fpga> ");
}

static void fpga_terminal_command_help(void) {
    printf("Available commands:\n");
    for (uint32_t index = 0U; index < sizeof(commands) / sizeof(commands[0]); ++index) {
        printf("  %-18s %s\n", commands[index].name, commands[index].description);
    }
}

static void fpga_terminal_command_info(void) {
    neon_uart_config_t config;
    if (neon_uart_get_config(terminal_uart, &config) != NEON_OK) {
        printf("[ERROR] Cannot read terminal UART configuration.\n");
        return;
    }
    printf("Target: FU2 FPGA\n");
    printf("Core clock: %lu Hz\n", (unsigned long)SystemCoreClock);
    printf("Terminal: UART0, %lu baud, 8N1, polling\n", (unsigned long)config.baud_rate);
    printf("Command limit: %lu characters\n", (unsigned long)(CONFIG_FPGA_TERMINAL_LINE_SIZE - 1U));
}

static void fpga_terminal_command_clear(void) {
    printf("\033[2J\033[H");
}

static void fpga_terminal_command_uart_echo(void) {
    /* The testcase consumes the command's pending LF when Enter sent CRLF. */
    uint32_t skip_initial_lf = skip_lf;
    skip_lf = 0U;
    uart_echo_polling_run(terminal_uart, skip_initial_lf);
}

static void fpga_terminal_command_gpio_io(void) {
    /* This testcase ignores ordinary input, including the command's pending LF. */
    skip_lf = 0U;
    gpio_input_output_run(terminal_uart);
}

static void fpga_terminal_command_gpio_irq(void) {
    skip_lf = 0U;
    gpio_interrupt_run(terminal_uart);
}

static void fpga_terminal_command_tim_irq(void) {
    skip_lf = 0U;
    tim_periodic_interrupt_run(terminal_uart);
}

static void fpga_terminal_command_flash_rw(void) {
    flash_read_write_run();
}

static void fpga_terminal_command_uart_rx_interrupt(void) {
    uint32_t skip_initial_lf = skip_lf;
    skip_lf = 0U;
    uart_rx_interrupt_run(terminal_uart, skip_initial_lf);
}

static void fpga_terminal_command_pwm_output(void) {
    skip_lf = 0U;
    pwm_output_run(terminal_uart);
}

static void fpga_terminal_command_uart_dma(void) {
    skip_lf = 0U;
    uart_dma_run(terminal_uart);
}

static void fpga_terminal_command_i2c_rw(void) {
    i2c_register_read_write_run();
}

static void fpga_terminal_command_spi_w5500(void) {
    skip_lf = 0U;
    spi_w5500_run(terminal_uart);
}

static void fpga_terminal_command_adc_sampling(void) {
    adc_sampling_run();
}

static void fpga_terminal_command_dma_memcpy(void) {
    dma_memory_copy_run();
}

static void fpga_terminal_command_rtc_calendar(void) {
    rtc_calendar_run();
}

static void fpga_terminal_command_crc_vector(void) {
    crc_known_vector_run();
}

static void fpga_terminal_command_aes_vector(void) {
    aes_known_vector_run();
}

static void fpga_terminal_command_trng_read(void) {
    trng_read_run();
}

static void fpga_terminal_command_qspi_flash(void) {
    qspi_flash_indirect_run();
}

static void fpga_terminal_command_i2c_slave(void) {
    i2c_slave_run();
}

static void fpga_terminal_command_spi_dma(void) {
    spi_dma_run();
}

static void fpga_terminal_command_i2c_dma(void) {
    i2c_dma_run();
}

static void fpga_terminal_command_freertos_queue(void) {
    free_rtos_peripheral_queue_run();
}

static void fpga_terminal_terminal_dispatch(char *line) {
    line += strspn(line, " \t");
    size_t command_length = strcspn(line, " \t");
    if (command_length == 0U) {
        return;
    }
    /* Match the first word and ignore any text after the command name. */
    memset(line + command_length, 0, 1U);

    for (uint32_t index = 0U; index < sizeof(commands) / sizeof(commands[0]); ++index) {
        if (strcmp(line, commands[index].name) == 0) {
            commands[index].handler();
            return;
        }
    }
    printf("Unknown command: %s. Type 'help'.\n", line);
}

static void fpga_terminal_terminal_reset_line(void) {
    line_length = 0U;
    discard_line = 0U;
    escape_state = 0U;
}

static void fpga_terminal_terminal_process_character(uint32_t value) {
    char *line = (char *)line_storage;

    /* CR, LF and CRLF each submit exactly one command line. */
    if (value == '\n' && skip_lf != 0U) {
        skip_lf = 0U;
        return;
    }
    skip_lf = (value == '\r') ? 1U : 0U;
    if (value == '\r' || value == '\n') {
        printf("\n");
        if (discard_line == 0U) {
            memset(line + line_length, 0, 1U);
            fpga_terminal_terminal_dispatch(line);
        }
        fpga_terminal_terminal_reset_line();
        fpga_terminal_terminal_prompt();
        return;
    }

    /* Ctrl+C cancels the current line, including a line discarded on error. */
    if (value == 3U) {
        fpga_terminal_terminal_reset_line();
        printf("^C\n");
        fpga_terminal_terminal_prompt();
        return;
    }
    if (discard_line != 0U) {
        return;
    }

    /* Consume cursor/function-key sequences; history/edit-in-place are not used. */
    if (value == 27U) {
        escape_state = 1U;
        return;
    }
    if (escape_state == 1U) {
        escape_state = (value == '[' || value == 'O') ? 2U : 0U;
        return;
    }
    if (escape_state == 2U) {
        if (value >= 0x40U && value <= 0x7EU) {
            escape_state = 0U;
        }
        return;
    }

    if (value == '\b' || value == 127U) {
        if (line_length != 0U) {
            --line_length;
            printf("\b \b");
        }
        return;
    }
    if (value != '\t' && (value < 0x20U || value > 0x7EU)) {
        return;
    }

    if (line_length >= CONFIG_FPGA_TERMINAL_LINE_SIZE - 1U) {
        discard_line = 1U;
        printf("\n[ERROR] Line too long; discarded. Press Enter or Ctrl+C.\n");
        return;
    }
    /* A tab is one space on screen and in the command buffer. */
    if (value == '\t') {
        value = ' ';
    }
    memcpy(line + line_length, &value, 1U);
    ++line_length;
    printf("%c", (int)value);
}
