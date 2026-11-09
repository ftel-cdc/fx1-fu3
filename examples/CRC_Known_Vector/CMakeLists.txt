cmake_minimum_required(VERSION 3.21)
project(fu2_crc_known_vector C ASM)

set(FU2_EXAMPLE_SOURCES Src/main.c Src/crc_known_vector.c)
include(../cmake/fpga_example.cmake)
