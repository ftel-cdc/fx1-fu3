cmake_minimum_required(VERSION 3.21)
project(fu2_aes_known_vector C ASM)

set(FU2_EXAMPLE_SOURCES main.c aes_known_vector.c)
include(../cmake/fpga_example.cmake)
