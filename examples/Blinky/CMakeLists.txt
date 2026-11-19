cmake_minimum_required(VERSION 3.25)
project(blinky C ASM)

if(NOT NEON_MDF_PATH)
  if(DEFINED ENV{NEON_MDF_PATH})
    set(NEON_MDF_PATH "$ENV{NEON_MDF_PATH}")
  else()
    get_filename_component(NEON_MDF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../neon-mdf" ABSOLUTE)
  endif()
endif()
include(${NEON_MDF_PATH}/tools/cmake/project.cmake)

neon_mdf_add_executable(blinky Src/main.c)
