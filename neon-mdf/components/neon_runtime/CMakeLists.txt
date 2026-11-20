# Embedded NEON graph runtime for FX1. The host compiler/MLIR pipeline is not
# part of this target; the model is produced before firmware build.
if(NOT CMAKE_CXX_COMPILER_LOADED)
  message(FATAL_ERROR
    "neon_runtime uses C++; set project(... C CXX ASM) before including project.cmake")
endif()

if(NOT NEON_MDF_TARGET STREQUAL "FX1" AND NOT NEON_MDF_TARGET STREQUAL "FX1_VP")
  message(FATAL_ERROR "neon_runtime currently supports only the MDF FX1 targets")
endif()

if(NOT TARGET ${NEON_MDF_DEVICE_TARGET})
  message(FATAL_ERROR "neon_runtime requires NEON_MDF_DEVICE_TARGET from neon-mdf")
endif()

set(_neon_runtime_root "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(neon_mdf_neon_runtime STATIC
  runtime/loader/ModelLoader.cpp
  runtime/executor/Executor.cpp
  runtime/graph/GraphLoader.cpp
  runtime/graph/ArenaPlanner.cpp
  runtime/graph/GraphRunner.cpp
  runtime/binder/OperandBinder.cpp
  runtime/capi/NeonCAPI.cpp
  analyse/StaticAnalyzer.cpp
  analyse/PerfReport.cpp
  analyse/MeasuredProfiler.cpp
  analyse/DMAProfiler.cpp
)
add_library(neon_mdf::neon_runtime ALIAS neon_mdf_neon_runtime)

target_include_directories(neon_mdf_neon_runtime
  PUBLIC
    "${_neon_runtime_root}"
    "${_neon_runtime_root}/runtime/include"
    "${_neon_runtime_root}/third_party/dlpack/include"
  PRIVATE
    "${_neon_runtime_root}/analyse"
)
target_compile_features(neon_mdf_neon_runtime PUBLIC cxx_std_17)
target_compile_definitions(neon_mdf_neon_runtime PRIVATE NEON_RUNTIME_NO_FILESYSTEM=1)
target_compile_options(neon_mdf_neon_runtime PRIVATE
  $<$<COMPILE_LANGUAGE:CXX>:-O2>
  $<$<COMPILE_LANGUAGE:CXX>:-ffunction-sections>
  $<$<COMPILE_LANGUAGE:CXX>:-fdata-sections>
)
target_link_libraries(neon_mdf_neon_runtime PUBLIC
  ${NEON_MDF_DEVICE_TARGET}
  stdc++
  c
  m
)
set_target_properties(neon_mdf_neon_runtime PROPERTIES
  CXX_STANDARD 17
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS NO
)

# CPU fallbacks are kept in a separate library, selected by the application.
# The explicit registration entry point references all functions so the
# archive linker retains them and avoids startup-time static constructors.
# Conv2D CPU fallback and compiler-generated native-kernel dispatch are not
# implemented by this runtime target; those ops must be emitted as supported
# NPU instructions or use a registered generic fallback.
add_library(neon_mdf_neon_fallbacks STATIC
  runtime/opfallbacks/NeonMDFCpuFallbacks.cpp
  runtime/opfallbacks/TransposeCPUFallback.cpp
  runtime/opfallbacks/DWConvCPUFallback.cpp
  runtime/opfallbacks/DWIm2ColCPUFallback.cpp
  runtime/opfallbacks/Im2ColCPUFallback.cpp
  runtime/opfallbacks/LutCPUFallback.cpp
  runtime/opfallbacks/SliceCPUFallback.cpp
  runtime/opfallbacks/StridedSliceCPUFallback.cpp
  runtime/opfallbacks/PadCPUFallback.cpp
  runtime/opfallbacks/ConcatCPUFallback.cpp
  runtime/opfallbacks/ResizeCPUFallback.cpp
  runtime/opfallbacks/MaxPoolCPUFallback.cpp
  runtime/opfallbacks/SoftmaxCPUFallback.cpp
  runtime/opfallbacks/LayerNormCPUFallback.cpp
  runtime/opfallbacks/ConstCopyCPUFallback.cpp
  runtime/opfallbacks/EmbeddingCPUFallback.cpp
)
add_library(neon_mdf::neon_fallbacks ALIAS neon_mdf_neon_fallbacks)
target_include_directories(neon_mdf_neon_fallbacks PRIVATE
  "${_neon_runtime_root}"
  "${_neon_runtime_root}/runtime/include"
  "${_neon_runtime_root}/third_party/dlpack/include"
)
target_compile_features(neon_mdf_neon_fallbacks PRIVATE cxx_std_17)
target_compile_definitions(neon_mdf_neon_fallbacks PRIVATE
  NEON_RUNTIME_NO_FILESYSTEM=1
  NEON_RUNTIME_EXPLICIT_CPU_FALLBACK_REGISTRATION=1
)
target_compile_options(neon_mdf_neon_fallbacks PRIVATE
  -O2
  -ffunction-sections
  -fdata-sections
)
target_link_libraries(neon_mdf_neon_fallbacks PUBLIC
  ${NEON_MDF_DEVICE_TARGET}
  stdc++
  c
  m
)

# The graph adapter for VP_FX1_V2.0 is compiled by the standalone
# examples/RUNTIME_NPU_APP/Makefile with its matching BSP and VP ABI. The SDK's
# normal FX1/FX1_VP CMake target describes V3.0 and does not select that backend.
