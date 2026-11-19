cmake_minimum_required(VERSION 3.21)
project(printf_log C ASM)

if(NOT NEON_MDF_PATH)
  get_filename_component(NEON_MDF_PATH "../../neon-mdf" ABSOLUTE)
endif()
include(${NEON_MDF_PATH}/tools/cmake/project.cmake)
if(NOT NEON_MDF_TARGET STREQUAL "GT32U2086")
  message(FATAL_ERROR "Printf_Log currently supports GT32U2086 only")
endif()

# This example defaults to the same externally supplied 10-MHz FPGA clock
# as Test_VP. Set 0 to use the SDK's normal SystemInit clock policy.
set(NEON_PRINTF_FIXED_CLOCK_HZ "10000000" CACHE STRING "Fixed FPGA clock Hz; 0 for normal clock setup")
if(NOT NEON_PRINTF_FIXED_CLOCK_HZ STREQUAL "0")
  target_compile_definitions(${NEON_MDF_DEVICE_TARGET} PUBLIC
    GT32U2086_FPGA_FIXED_CLOCK_HZ=${NEON_PRINTF_FIXED_CLOCK_HZ}UL)
endif()
neon_mdf_add_executable(printf_log Src/main.c)
