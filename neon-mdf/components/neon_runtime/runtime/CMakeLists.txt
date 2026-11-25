add_library(neon_runtime STATIC
    NeonRuntime.cpp
    loader/ModelLoader.cpp
    executor/Executor.cpp
    graph/GraphLoader.cpp
    graph/ArenaPlanner.cpp
    graph/GraphRunner.cpp
    binder/OperandBinder.cpp
    capi/NeonCAPI.cpp
)

target_include_directories(neon_runtime PUBLIC
    "${NEON_INCLUDE_ROOT}"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/dlpack/include"
)

target_link_libraries(neon_runtime PUBLIC
    neon_analyse
)

set_target_properties(neon_runtime PROPERTIES
    CXX_STANDARD 17
    POSITION_INDEPENDENT_CODE ON
)

# ============================================================
# neon_opfallbacks — CPU implementations of ops no NPU realizes
#
# Kept OUT of neon_runtime and in its own library for the same reason the
# device libs are separate: every fallback here self-registers into
# CPUFallbackRegistry via NEON_REGISTER_CPU_FALLBACK, and nothing calls it
# by symbol name — Executor::dispatchCPUFallback() only ever looks it up by
# fallback_id. A plain archive link therefore drops the object and the
# registry stays empty, so consumers must whole-archive link this (see
# neon_link_whole_archive() in the root CMakeLists.txt). Keeping it as its
# own target keeps that forced-load set small and explicit instead of
# dragging all of neon_runtime in.
#
# Links neon_conv2d_kernel because the CONV2D fallback delegates the actual
# math to conv2D_Func() rather than reimplementing it (see
# opfallbacks/Conv2DCPUFallback.h).
# ============================================================
add_library(neon_opfallbacks STATIC
    opfallbacks/Conv2DCPUFallback.cpp
    opfallbacks/DWConvCPUFallback.cpp
    opfallbacks/DWIm2ColCPUFallback.cpp
    opfallbacks/MaxPoolCPUFallback.cpp
    opfallbacks/SoftmaxCPUFallback.cpp
    opfallbacks/TransposeCPUFallback.cpp
    opfallbacks/Im2ColCPUFallback.cpp
    opfallbacks/LutCPUFallback.cpp
    opfallbacks/SliceCPUFallback.cpp
    opfallbacks/StridedSliceCPUFallback.cpp
    opfallbacks/PadCPUFallback.cpp
    opfallbacks/ConcatCPUFallback.cpp
    opfallbacks/ResizeCPUFallback.cpp
    opfallbacks/ConstCopyCPUFallback.cpp
    opfallbacks/LayerNormCPUFallback.cpp
    opfallbacks/EmbeddingCPUFallback.cpp
    opfallbacks/CompiledKernelRegistry.cpp
    opfallbacks/CompiledKernelCPUFallback.cpp
)

target_include_directories(neon_opfallbacks PUBLIC
    "${NEON_INCLUDE_ROOT}"
    "${CMAKE_SOURCE_DIR}"
)

target_link_libraries(neon_opfallbacks PUBLIC
    neon_runtime
    neon_conv2d_kernel
)

set_target_properties(neon_opfallbacks PROPERTIES
    CXX_STANDARD 17
    POSITION_INDEPENDENT_CODE ON
)
