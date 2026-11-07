cmake_minimum_required(VERSION 3.21)
project(fu2_spi_w5500 C ASM)

set(FU2_EXAMPLE_SOURCES main.c spi_w5500.c)
include(../cmake/fpga_example.cmake)
