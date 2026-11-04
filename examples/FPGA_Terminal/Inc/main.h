/**
 * @file main.h
 * @brief Public declarations for main.
 * @details
 * Declares module interfaces.
 */

#ifndef FPGA_TERMINAL_MAIN_H
#define FPGA_TERMINAL_MAIN_H

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"
#include "console/neon_console.h"
#include "uart/neon_uart.h"
#include "GPIO_Input_Output/gpio_input_output.h"
#include "GPIO_Interrupt/gpio_interrupt.h"
#include "TIM_Periodic_Interrupt/tim_periodic_interrupt.h"
#include "Flash_Read_Write/flash_read_write.h"
#include "UART_ECHO_Polling/uart_echo_polling.h"
#include "UART_RX_Interrupt/uart_rx_interrupt.h"
#include "PWM_Output/pwm_output.h"
#include "I2C_Register_Read_Write/i2c_register_read_write.h"
#include "SPI_W5500/spi_w5500.h"
#include "ADC_Sampling/adc_sampling.h"
#include "DMA_Memory_Copy/dma_memory_copy.h"
#include "RTC_Calendar/rtc_calendar.h"
#include "CRC_Known_Vector/crc_known_vector.h"
#include "AES_Known_Vector/aes_known_vector.h"
#include "TRNG_Read/trng_read.h"
#include "QSPI_Flash_Indirect/qspi_flash_indirect.h"
#include "I2C_Slave/i2c_slave.h"
#include "SPI_DMA/spi_dma.h"
#include "I2C_DMA/i2c_dma.h"
#include "FreeRTOS_Peripheral_Queue/freertos_peripheral_queue.h"

#endif /* FPGA_TERMINAL_MAIN_H */
