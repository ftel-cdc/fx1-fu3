cmake_minimum_required(VERSION 3.21)
project(fu2_uart_rx_interrupt C ASM)

set(FU2_EXAMPLE_SOURCES Src/main.c Src/uart_rx_interrupt.c)
include(../cmake/fpga_example.cmake)
