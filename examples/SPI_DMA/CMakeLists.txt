cmake_minimum_required(VERSION 3.21)
project(fu2_spi_dma C ASM)

set(FU2_EXAMPLE_SOURCES main.c spi_dma.c)
include(../cmake/fpga_example.cmake)
