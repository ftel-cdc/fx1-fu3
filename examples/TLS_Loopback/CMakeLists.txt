cmake_minimum_required(VERSION 3.25)
project(tls_loopback C ASM)
get_filename_component(NEON_MDF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../neon-mdf" ABSOLUTE)
include(${NEON_MDF_PATH}/tools/cmake/project.cmake)
if(NOT NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "FX1")
  message(FATAL_ERROR "TLS_Loopback requires FX1 (cmake --preset FX1)")
endif()
include(${NEON_MDF_PATH}/cmake/freertos.cmake)
include(${NEON_MDF_PATH}/cmake/neon_net.cmake)
include(${NEON_MDF_PATH}/cmake/neon_tls.cmake)
neon_mdf_add_freertos(${NEON_MDF_TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/Inc)
neon_mdf_add_net()
neon_mdf_add_tls()

neon_mdf_add_executable(tls_loopback Src/main.c Src/test_pki.c)
target_include_directories(tls_loopback PRIVATE Inc)
target_link_libraries(tls_loopback PRIVATE neon_mdf::tls neon_mdf::net neon_mdf::freertos)
target_compile_options(tls_loopback PRIVATE -Wall -Wextra)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
include(${NEON_MDF_PATH}/cmake/runners/fx1-vp.cmake)
add_custom_target(run-vp
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_vp.py
    --vp "${NEON_FX1_VP_EXECUTABLE}"
    --elf $<TARGET_FILE:tls_loopback>
    --output ${CMAKE_CURRENT_BINARY_DIR}/vp
  DEPENDS tls_loopback
  USES_TERMINAL VERBATIM
)
