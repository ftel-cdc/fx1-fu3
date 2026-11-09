cmake_minimum_required(VERSION 3.21)
project(fu2_tim_periodic_interrupt C ASM)

set(FU2_EXAMPLE_SOURCES Src/main.c Src/tim_periodic_interrupt.c)
include(../cmake/fpga_example.cmake)
