cmake_minimum_required(VERSION 3.21)
project(fu2_i2c_register_read_write C ASM)

set(FU2_EXAMPLE_SOURCES Src/main.c Src/i2c_register_read_write.c)
include(../cmake/fpga_example.cmake)
