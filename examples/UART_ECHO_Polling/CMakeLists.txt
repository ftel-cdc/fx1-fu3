cmake_minimum_required(VERSION 3.21)
project(gt32u2086_uart_echo_polling C ASM)

if(NOT NEON_MDF_PATH)
  if(DEFINED ENV{NEON_MDF_PATH})
    set(NEON_MDF_PATH "$ENV{NEON_MDF_PATH}")
  else()
    get_filename_component(NEON_MDF_PATH "${CMAKE_CURRENT_LIST_DIR}/../../neon-mdf" ABSOLUTE)
  endif()
endif()

include(${NEON_MDF_PATH}/tools/cmake/project.cmake)
if(NOT NEON_MDF_TARGET STREQUAL "GT32U2086")
  message(FATAL_ERROR "This example supports GT32U2086 only")
endif()

include(${CMAKE_BINARY_DIR}/config/sdkconfig.cmake)
if(NOT CONFIG_GT32U2086_EXAMPLE_FIXED_CLOCK_HZ EQUAL 0)
  target_compile_definitions(${NEON_MDF_DEVICE_TARGET} PUBLIC
    GT32U2086_FPGA_FIXED_CLOCK_HZ=${CONFIG_GT32U2086_EXAMPLE_FIXED_CLOCK_HZ}UL)
endif()

neon_mdf_add_executable(${PROJECT_NAME}
  Src/main.c
  Src/uart_echo_polling.c
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Inc)
