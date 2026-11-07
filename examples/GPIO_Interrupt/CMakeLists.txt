cmake_minimum_required(VERSION 3.21)
project(fu2_gpio_interrupt C ASM)

set(FU2_EXAMPLE_SOURCES main.c gpio_interrupt.c)
include(../cmake/fpga_example.cmake)
