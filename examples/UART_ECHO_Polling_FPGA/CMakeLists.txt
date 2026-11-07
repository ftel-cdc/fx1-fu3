cmake_minimum_required(VERSION 3.21)
project(fu2_uart_echo_polling C ASM)

set(FU2_EXAMPLE_SOURCES main.c uart_echo_polling.c)
include(../cmake/fpga_example.cmake)
