#include "StridedSliceCPUFallback.h"
#include "CPUFallbackRegistry.h"
#include "../../core/NPU_ISA.h"
#include "FallbackIO.h"
#include "../support/NeonLog.h"
#include <cstdint>
#include <cstring>

bool stridedSliceCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr || (op.instr_size && op.instr_size < sizeof(NeonCPUFallbackStridedSliceInstr))) {
        uart_puts("[ERROR] stridedSliceCPUFallback: missing or short instruction\r\n");
        return false;
    }
    // Packed struct: copy it out once so every field read below is aligned.
    NeonCPUFallbackStridedSliceInstr I;
    std::memcpy(&I, op.instr_ptr, sizeof(I));
    if (I.opcode != NeonOpcode::CPU_FALLBACK ||
        I.fallback_id != NEON_CPU_FALLBACK_STRIDED_SLICE) {
        uart_puts("[ERROR] stridedSliceCPUFallback: invalid opcode or fallback_id\r\n");
        return false;
    }
    const int64_t es = I.elem_size;
    if (es <= 0) {
        uart_puts("[ERROR] stridedSliceCPUFallback: elem_size == 0\r\n");
        return false;
    }

    // Element strides of the dense row-major input, and a bounds check of the
    // first and last index each axis touches (the compiler already guarantees
    // both; a bad .nsm must fail here, not scribble over the arena).
    int64_t inStride[4];
    inStride[3] = 1;
    for (int d = 2; d >= 0; --d) inStride[d] = inStride[d + 1] * I.in_dims[d + 1];
    for (int d = 0; d < 4; ++d) {
        if (I.in_dims[d] == 0 || I.out_dims[d] == 0 || I.stride[d] == 0) {
            uart_puts("[ERROR] stridedSliceCPUFallback: zero dim or zero stride\r\n");
            return false;
        }
        const int64_t first = I.begin[d];
        const int64_t last  = first + (int64_t)(I.out_dims[d] - 1) * I.stride[d];
        if (first < 0 || first >= (int64_t)I.in_dims[d] || last < 0 || last >= (int64_t)I.in_dims[d]) {
            uart_puts("[ERROR] stridedSliceCPUFallback: index out of input range\r\n");
            return false;
        }
    }

    const uint8_t* src = neon_fb::srcPtr<const uint8_t>(op, model, I.src_offset);
    uint8_t* dst = neon_fb::dstPtr<uint8_t>(op, model, I.dst_offset);
    if (!src || !dst) {
        uart_puts("[ERROR] stridedSliceCPUFallback: unbound operand\r\n");
        return false;
    }

    const size_t rowBytes = (size_t)I.out_dims[3] * (size_t)es;
    const bool denseRow = (I.stride[3] == 1);
    for (uint32_t n = 0; n < I.out_dims[0]; ++n) {
        const int64_t offN = (I.begin[0] + (int64_t)n * I.stride[0]) * inStride[0];
        for (uint32_t h = 0; h < I.out_dims[1]; ++h) {
            const int64_t offH = offN + (I.begin[1] + (int64_t)h * I.stride[1]) * inStride[1];
            for (uint32_t w = 0; w < I.out_dims[2]; ++w) {
                const int64_t offW = offH + (I.begin[2] + (int64_t)w * I.stride[2]) * inStride[2] + I.begin[3];
                const uint8_t* s = src + offW * es;
                if (denseRow) {
                    std::memcpy(dst, s, rowBytes);
                    dst += rowBytes;
                } else {
                    for (uint32_t c = 0; c < I.out_dims[3]; ++c) {
                        std::memcpy(dst, s + (int64_t)c * I.stride[3] * es, (size_t)es);
                        dst += es;
                    }
                }
            }
        }
    }
    return true;
}

NEON_REGISTER_CPU_FALLBACK(NEON_CPU_FALLBACK_STRIDED_SLICE, stridedSliceCPUFallback)

