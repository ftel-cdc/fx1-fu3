# FX1 V2.0 NPU backend. This component is deliberately not selected by the
# generic driver list: FX1 V2.0 uses its own VP/BSP ABI. Applications that use
# this backend must add this component explicitly and provide the matching BSP.
if(NOT CMAKE_CXX_COMPILER_LOADED OR NOT CMAKE_C_COMPILER_LOADED)
  message(FATAL_ERROR "runtime_npu requires C and C++ compilers")
endif()

set(_neon_npu_root "${CMAKE_CURRENT_LIST_DIR}")
set(_neon_npu_fx1_v2 "${_neon_npu_root}/platform/FX1_V2.0")
get_filename_component(_neon_npu_runtime
  "${_neon_npu_root}/../../neon_runtime" ABSOLUTE)

get_filename_component(_neon_npu_repo_root "${_neon_npu_root}/../../../.." ABSOLUTE)
if(NOT NEON_NPU_FX1_V2_BSP_ROOT)
  set(NEON_NPU_FX1_V2_BSP_ROOT
    "${_neon_npu_repo_root}/examples/RUNTIME_NPU_APP/firmware_support/FX1_V2.0")
endif()
set(_neon_npu_freertos "${NEON_NPU_FX1_V2_BSP_ROOT}/third_party/FreeRTOS-Kernel")
set(_neon_npu_port "${_neon_npu_freertos}/portable/GCC/RISC-V")
set(_neon_npu_chip_ext
  "${_neon_npu_port}/chip_specific_extensions/RISCV_MTIME_CLINT_no_extensions")

foreach(_required_path
    "${NEON_NPU_FX1_V2_BSP_ROOT}/FreeRTOSConfig.h"
    "${NEON_NPU_FX1_V2_BSP_ROOT}/include/uart.h"
    "${NEON_NPU_FX1_V2_BSP_ROOT}/common/include/soc/soc_memory_map.h"
    "${_neon_npu_freertos}/include/task.h")
  if(NOT EXISTS "${_required_path}")
    message(FATAL_ERROR
      "FX1 V2.0 BSP file not found: ${_required_path}. Set NEON_NPU_FX1_V2_BSP_ROOT.")
  endif()
endforeach()

add_library(neon_mdf_runtime_npu STATIC
  "${_neon_npu_fx1_v2}/NPUV4Runtime.cpp"
  "${_neon_npu_fx1_v2}/NPUV4Relocator.cpp"
  "${_neon_npu_fx1_v2}/SoftwareRequantFallback.cpp"
  "${_neon_npu_fx1_v2}/bsp_ext/npu_v4_native_cfg.c"
  "${_neon_npu_fx1_v2}/bsp_ext/npu_v4_native_driver.c"
)
add_library(neon_mdf::runtime_npu ALIAS neon_mdf_runtime_npu)

target_include_directories(neon_mdf_runtime_npu
  PUBLIC
    "${_neon_npu_root}"
    "${_neon_npu_fx1_v2}"
    "${_neon_npu_runtime}"
    "${_neon_npu_runtime}/runtime/include"
    "${_neon_npu_runtime}/third_party/dlpack/include"
  PRIVATE
    "${_neon_npu_fx1_v2}/bsp_ext"
    "${_neon_npu_fx1_v2}/hal"
    "${NEON_NPU_FX1_V2_BSP_ROOT}"
    "${NEON_NPU_FX1_V2_BSP_ROOT}/include"
    "${NEON_NPU_FX1_V2_BSP_ROOT}/common/include"
    "${_neon_npu_freertos}/include"
    "${_neon_npu_port}"
    "${_neon_npu_chip_ext}"
)
target_compile_features(neon_mdf_runtime_npu PUBLIC cxx_std_17)
target_compile_definitions(neon_mdf_runtime_npu PRIVATE
  NEON_RUNTIME_NO_FILESYSTEM=1
  NEON_RUNTIME_EXPLICIT_CPU_FALLBACK_REGISTRATION=1
)

# The standalone runtime application links these when the SDK runtime targets
# are enabled in the same CMake project. The Makefile application compiles the
# same source set directly with the same FX1 V2.0 BSP include paths.
if(TARGET neon_mdf_neon_runtime)
  target_link_libraries(neon_mdf_runtime_npu PUBLIC neon_mdf_neon_runtime)
endif()
if(TARGET neon_mdf_neon_fallbacks)
  target_link_libraries(neon_mdf_runtime_npu PUBLIC neon_mdf_neon_fallbacks)
endif()
