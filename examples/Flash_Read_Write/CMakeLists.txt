cmake_minimum_required(VERSION 3.21)
project(fu2_flash_read_write C ASM)

set(FU2_EXAMPLE_SOURCES Src/main.c Src/flash_read_write.c)
include(../cmake/fpga_example.cmake)

# Reserve the scratch sector before the SDK linker script is processed.
target_link_options(${PROJECT_NAME} BEFORE PRIVATE
  -T${CMAKE_CURRENT_SOURCE_DIR}/flash_read_write.ld)
set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY LINK_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/flash_read_write.ld)
