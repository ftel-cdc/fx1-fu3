cmake_minimum_required(VERSION 3.21)
project(fx1_freertos_smoke C ASM)

if(NOT DEFINED NEON_MDF_PATH)
  get_filename_component(NEON_MDF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../neon-mdf" ABSOLUTE)
endif()
if(NOT NEON_MDF_TARGET STREQUAL "FX1_VP")
  message(FATAL_ERROR "Vi du nay dung target FX1_VP; chay cmake --preset FX1_VP")
endif()
include(${NEON_MDF_PATH}/tools/cmake/project.cmake)
include(${NEON_MDF_PATH}/cmake/freertos.cmake)
neon_mdf_add_freertos(${NEON_MDF_TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/Inc)

neon_mdf_add_executable(fx1_freertos_smoke Src/main.c Src/fpu_probe.S)
target_link_libraries(fx1_freertos_smoke PRIVATE neon_mdf::freertos)
set_target_properties(fx1_freertos_smoke PROPERTIES SUFFIX ".elf")

find_package(Python3 REQUIRED COMPONENTS Interpreter)
# Runner VP tùy chọn; configure/build ELF không cần repo/binary VP.
include(${NEON_MDF_PATH}/cmake/runners/fx1-vp.cmake)
add_custom_target(run-vp
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/run_vp.py
    --vp ${NEON_FX1_VP_EXECUTABLE}
    --elf $<TARGET_FILE:fx1_freertos_smoke>
    --readelf ${CMAKE_READELF}
    --output ${CMAKE_CURRENT_BINARY_DIR}/vp
  DEPENDS fx1_freertos_smoke
  USES_TERMINAL VERBATIM)
