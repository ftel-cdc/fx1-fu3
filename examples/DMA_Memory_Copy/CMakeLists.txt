cmake_minimum_required(VERSION 3.21)
project(fu2_dma_memory_copy C ASM)

set(FU2_EXAMPLE_SOURCES Src/main.c Src/dma_memory_copy.c)
include(../cmake/fpga_example.cmake)
