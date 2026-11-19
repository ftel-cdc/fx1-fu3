# NEON-MDF build of the vendored FreeRTOS kernel, restricted to GT32U2086.
cmake_minimum_required(VERSION 3.15)
if(NOT TARGET freertos_config)
  message(FATAL_ERROR "Application must provide freertos_config")
endif()
if(NOT FREERTOS_PORT STREQUAL "GCC_ARM_CM0" OR NOT FREERTOS_HEAP STREQUAL "4")
  message(FATAL_ERROR "This SDK includes only GCC_ARM_CM0 and heap_4")
endif()
add_library(freertos_kernel STATIC
  croutine.c event_groups.c list.c queue.c stream_buffer.c tasks.c timers.c
  portable/MemMang/heap_4.c)
add_subdirectory(include)
add_subdirectory(portable)
target_link_libraries(freertos_kernel
  PUBLIC freertos_kernel_include freertos_kernel_port_headers
  PRIVATE freertos_kernel_port)
