cmake_minimum_required(VERSION 3.21)
project(fpga_terminal C ASM)

if(NOT NEON_MDF_PATH)
  if(DEFINED ENV{NEON_MDF_PATH})
    set(NEON_MDF_PATH "$ENV{NEON_MDF_PATH}")
  else()
    get_filename_component(NEON_MDF_PATH "../../neon-mdf" ABSOLUTE)
  endif()
endif()
include(${NEON_MDF_PATH}/tools/cmake/project.cmake)
if(NOT NEON_MDF_TARGET STREQUAL "FU2")
  message(FATAL_ERROR "FPGA_Terminal currently supports FU2 only")
endif()

include(${CMAKE_BINARY_DIR}/config/sdkconfig.cmake)
if(NOT CONFIG_FPGA_TERMINAL_FIXED_CLOCK_HZ EQUAL 0)
  target_compile_definitions(${NEON_MDF_DEVICE_TARGET} PUBLIC
    FU2_FPGA_FIXED_CLOCK_HZ=${CONFIG_FPGA_TERMINAL_FIXED_CLOCK_HZ}UL)
endif()

neon_mdf_add_executable(fpga_terminal Src/main.c)
target_include_directories(fpga_terminal PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Inc
  ${CMAKE_CURRENT_SOURCE_DIR}/testcase)
add_subdirectory(testcase)
