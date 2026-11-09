cmake_minimum_required(VERSION 3.21)
project(fu2_gpio_interrupt C ASM)

set(FU2_EXAMPLE_SOURCES Src/main.c Src/gpio_interrupt.c)
include(../cmake/fpga_example.cmake)
