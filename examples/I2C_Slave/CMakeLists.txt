cmake_minimum_required(VERSION 3.21)
project(fu2_i2c_slave C ASM)

set(FU2_EXAMPLE_SOURCES main.c i2c_slave.c)
include(../cmake/fpga_example.cmake)
