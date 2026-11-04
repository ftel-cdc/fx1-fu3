cmake_minimum_required(VERSION 3.21)
project(uart_echo_polling C ASM)

if(NOT NEON_MDF_PATH)
  if(DEFINED ENV{NEON_MDF_PATH})
    set(NEON_MDF_PATH "$ENV{NEON_MDF_PATH}")
  else()
    get_filename_component(NEON_MDF_PATH "../../neon-mdf" ABSOLUTE)
  endif()
endif()
include(${NEON_MDF_PATH}/tools/cmake/project.cmake)
if(NOT NEON_MDF_TARGET STREQUAL "FU2")
  message(FATAL_ERROR "UART_ECHO_Polling currently supports FU2 only")
endif()

# Kconfig is processed inside the SDK subdirectory. Load its generated values
# into the app scope so the clock option also configures SDK startup/drivers.
include(${CMAKE_BINARY_DIR}/config/sdkconfig.cmake)
if(NOT CONFIG_UART_ECHO_FIXED_CLOCK_HZ EQUAL 0)
  target_compile_definitions(${NEON_MDF_DEVICE_TARGET} PUBLIC
    FU2_FPGA_FIXED_CLOCK_HZ=${CONFIG_UART_ECHO_FIXED_CLOCK_HZ}UL)
endif()

neon_mdf_add_executable(uart_echo_polling Src/main.c)
