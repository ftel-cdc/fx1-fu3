cmake_minimum_required(VERSION 3.21)
project(fu2_i2c_dma C ASM)

set(FU2_EXAMPLE_SOURCES Src/main.c Src/i2c_dma.c)
include(../cmake/fpga_example.cmake)
