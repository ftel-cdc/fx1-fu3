cmake_minimum_required(VERSION 3.21)
project(fu2_freertos_peripheral_queue C ASM)

set(FU2_EXAMPLE_SOURCES main.c freertos_peripheral_queue.c)
include(../cmake/fpga_example.cmake)
