cmake_minimum_required(VERSION 3.25)
project(freertos_blinky C ASM)

if(NOT NEON_MDF_PATH)
  if(DEFINED ENV{NEON_MDF_PATH})
    set(NEON_MDF_PATH "$ENV{NEON_MDF_PATH}")
  else()
    get_filename_component(NEON_MDF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../neon-mdf" ABSOLUTE)
  endif()
endif()
include(${NEON_MDF_PATH}/tools/cmake/project.cmake)

include(${NEON_MDF_PATH}/cmake/freertos.cmake)
neon_mdf_add_freertos(${NEON_MDF_TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/Inc)

neon_mdf_add_executable(freertos_blinky Src/main.c)
target_link_libraries(freertos_blinky PRIVATE freertos_kernel)
