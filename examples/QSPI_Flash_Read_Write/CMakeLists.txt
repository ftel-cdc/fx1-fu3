cmake_minimum_required(VERSION 3.21)
project(fu2_qspi_flash_read_write C ASM)

if(NOT NEON_MDF_PATH)
  if(DEFINED ENV{NEON_MDF_PATH})
    set(NEON_MDF_PATH "$ENV{NEON_MDF_PATH}")
  else()
    get_filename_component(NEON_MDF_PATH "${CMAKE_CURRENT_LIST_DIR}/../../neon-mdf" ABSOLUTE)
  endif()
endif()

# These apps reconfigure/program external NOR while executing from internal Flash.
if(NEON_MDF_PROFILE STREQUAL "xip")
  message(FATAL_ERROR "This example must use the normal internal-Flash profile")
endif()

include(${NEON_MDF_PATH}/tools/cmake/project.cmake)
if(NOT NEON_MDF_TARGET STREQUAL "FU2")
  message(FATAL_ERROR "This example supports FU2 only")
endif()

include(${CMAKE_BINARY_DIR}/config/sdkconfig.cmake)
if(NOT CONFIG_FU2_EXAMPLE_FIXED_CLOCK_HZ EQUAL 0)
  target_compile_definitions(${NEON_MDF_DEVICE_TARGET} PUBLIC
    FU2_FPGA_FIXED_CLOCK_HZ=${CONFIG_FU2_EXAMPLE_FIXED_CLOCK_HZ}UL)
endif()

neon_mdf_add_executable(${PROJECT_NAME}
  Src/main.c
  Src/qspi_flash_read_write.c
)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Inc)
