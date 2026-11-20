cmake_minimum_required(VERSION 3.25)
project(__APP_NAME__ C ASM)

if(NOT NEON_MDF_PATH)
  if(DEFINED ENV{NEON_MDF_PATH})
    set(NEON_MDF_PATH "$ENV{NEON_MDF_PATH}")
  else()
    get_filename_component(NEON_MDF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/__NEON_MDF_RELATIVE_PATH__" ABSOLUTE)
  endif()
endif()
include(${NEON_MDF_PATH}/tools/cmake/project.cmake)

set(NEON_APP_FIXED_CLOCK_HZ "0" CACHE STRING "Actual FPGA clock Hz; 0 uses CRM clock detection")
if(NEON_APP_FIXED_CLOCK_HZ LESS 0 OR NEON_APP_FIXED_CLOCK_HZ GREATER 60000000)
  message(FATAL_ERROR "NEON_APP_FIXED_CLOCK_HZ must be 0..60000000")
endif()
if(NOT NEON_APP_FIXED_CLOCK_HZ EQUAL 0)
  target_compile_definitions(${NEON_MDF_DEVICE_TARGET} PUBLIC
    GT32U2086_FPGA_FIXED_CLOCK_HZ=${NEON_APP_FIXED_CLOCK_HZ}UL)
endif()

neon_mdf_add_executable(__APP_NAME__ Src/main.c)
