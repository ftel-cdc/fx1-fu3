cmake_minimum_required(VERSION 3.21)
project(fu2_rtc_calendar C ASM)

set(FU2_EXAMPLE_SOURCES Src/main.c Src/rtc_calendar.c)
include(../cmake/fpga_example.cmake)
