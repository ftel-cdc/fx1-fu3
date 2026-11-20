#include "NeonMDFCpuFallbacks.h"

#include "CPUFallbackRegistry.h"
#include "ConcatCPUFallback.h"
#include "ConstCopyCPUFallback.h"
#include "DWConvCPUFallback.h"
#include "DWIm2ColCPUFallback.h"
#include "EmbeddingCPUFallback.h"
#include "Im2ColCPUFallback.h"
#include "LayerNormCPUFallback.h"
#include "LutCPUFallback.h"
#include "MaxPoolCPUFallback.h"
#include "PadCPUFallback.h"
#include "ResizeCPUFallback.h"
#include "SliceCPUFallback.h"
#include "SoftmaxCPUFallback.h"
#include "StridedSliceCPUFallback.h"
#include "TransposeCPUFallback.h"

extern "C" void NeonMDFRegisterEmbeddedCpuFallbacks(void)
{
    CPUFallbackRegistry& registry = CPUFallbackRegistry::get();
    registry.registerFallback(NEON_CPU_FALLBACK_TRANSPOSE, transposeCPUFallback);
    registry.registerFallback(NEON_CPU_FALLBACK_IM2COL, im2colCPUFallback);
    registry.registerFallback(NEON_CPU_FALLBACK_DW_IM2COL, dwIm2colCPUFallback);
    registry.registerFallback(NEON_CPU_FALLBACK_LUT, lutCPUFallback);
    registry.registerFallback(NEON_CPU_FALLBACK_DWCONV, dwConvCPUFallback);
    registry.registerFallback(NEON_CPU_FALLBACK_SLICE, sliceCPUFallback);
    registry.registerFallback(NEON_CPU_FALLBACK_STRIDED_SLICE, stridedSliceCPUFallback);
    registry.registerFallback(NEON_CPU_FALLBACK_PAD, padCPUFallback);
    registry.registerFallback(NEON_CPU_FALLBACK_CONCAT, concatCPUFallback);
    registry.registerFallback(NEON_CPU_FALLBACK_RESIZE_NN, resizeCPUFallback);
    registry.registerFallback(NEON_CPU_FALLBACK_MAXPOOL, maxPoolCPUFallback);
    registry.registerFallback(NEON_CPU_FALLBACK_SOFTMAX, softmaxCPUFallback);
    registry.registerFallback(NEON_CPU_FALLBACK_LAYERNORM, layerNormCPUFallback);
    registry.registerFallback(NEON_CPU_FALLBACK_CONST_COPY, constCopyCPUFallback);
    registry.registerFallback(NEON_CPU_FALLBACK_EMBEDDING, embeddingCPUFallback);
}
