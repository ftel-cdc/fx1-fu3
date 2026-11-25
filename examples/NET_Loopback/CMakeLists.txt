cmake_minimum_required(VERSION 3.25)
project(net_loopback C ASM)
get_filename_component(NEON_MDF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../neon-mdf" ABSOLUTE)
include(${NEON_MDF_PATH}/tools/cmake/project.cmake)
if(NOT NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "FX1")
  message(FATAL_ERROR "NET_Loopback requires FX1 (cmake --preset FX1)")
endif()
include(${NEON_MDF_PATH}/cmake/freertos.cmake)
include(${NEON_MDF_PATH}/cmake/neon_net.cmake)
neon_mdf_add_freertos(${NEON_MDF_TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/Inc)
neon_mdf_add_net()

neon_mdf_add_executable(net_loopback Src/main.c)
target_link_libraries(net_loopback PRIVATE neon_mdf::net neon_mdf::freertos)
target_compile_options(net_loopback PRIVATE -Wall -Wextra)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
include(${NEON_MDF_PATH}/cmake/runners/fx1-vp.cmake)
add_custom_target(run-vp
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_vp.py
    --vp "${NEON_FX1_VP_EXECUTABLE}"
    --elf $<TARGET_FILE:net_loopback>
    --output ${CMAKE_CURRENT_BINARY_DIR}/vp
  DEPENDS net_loopback
  USES_TERMINAL VERBATIM
)
