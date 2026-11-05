cmake_minimum_required(VERSION 3.25)
project(fx1_isp_image C ASM)
get_filename_component(NEON_MDF_PATH "${CMAKE_CURRENT_LIST_DIR}/../../neon-mdf" ABSOLUTE)
include("${NEON_MDF_PATH}/tools/cmake/project.cmake")
if(NOT NEON_MDF_TARGET STREQUAL "FX1" OR NOT TARGET neon_mdf_ftel_isp)
  message(FATAL_ERROR "Use preset FX1 and CONFIG_NEON_MDF_FTEL_ISP=y")
endif()
neon_mdf_add_executable(fx1_isp_image Src/main.c)
target_link_libraries(fx1_isp_image PRIVATE neon_mdf_ftel_isp)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(FX1_ISP_PROFILE "${CMAKE_CURRENT_SOURCE_DIR}/profile/profile.csrw" CACHE FILEPATH "Neon application tuning profile")
set(_profile_header "${CMAKE_CURRENT_BINARY_DIR}/generated/profile.h")
add_custom_command(OUTPUT "${_profile_header}"
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_profile.py"
    --profile "${FX1_ISP_PROFILE}"
    --output "${_profile_header}"
  DEPENDS "${FX1_ISP_PROFILE}"
          "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_profile.py"
  VERBATIM)
target_sources(fx1_isp_image PRIVATE "${_profile_header}")
target_include_directories(fx1_isp_image PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(FX1_IMAGE_FRAME_COUNT "" CACHE STRING "Override frame count (1..4); empty uses tuning default")
if(NOT FX1_IMAGE_FRAME_COUNT STREQUAL "")
  if(NOT FX1_IMAGE_FRAME_COUNT MATCHES "^[1-4]$")
    message(FATAL_ERROR "FX1_IMAGE_FRAME_COUNT must be 1..4")
  endif()
  target_compile_definitions(fx1_isp_image PRIVATE FRAME_COUNT=${FX1_IMAGE_FRAME_COUNT})
endif()
