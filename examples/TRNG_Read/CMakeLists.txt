cmake_minimum_required(VERSION 3.21)
project(fu2_trng_read C ASM)

set(FU2_EXAMPLE_SOURCES main.c trng_read.c)
include(../cmake/fpga_example.cmake)
