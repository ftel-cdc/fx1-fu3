cmake_minimum_required(VERSION 3.21)
project(fu2_adc_sampling C ASM)

set(FU2_EXAMPLE_SOURCES main.c adc_sampling.c)
include(../cmake/fpga_example.cmake)
