cmake_minimum_required(VERSION 3.21)
project(fu2_uart_dma C ASM)

set(FU2_EXAMPLE_SOURCES main.c uart_dma.c)
include(../cmake/fpga_example.cmake)
