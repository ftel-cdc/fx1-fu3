cmake_minimum_required(VERSION 3.21)
project(fu2_i2c_slave C ASM)

set(FU2_EXAMPLE_SOURCES Src/main.c Src/i2c_slave.c)
include(../cmake/fpga_example.cmake)
