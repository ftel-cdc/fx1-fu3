#include "EmbeddingCPUFallback.h"
#include "CPUFallbackRegistry.h"
#include "../../core/NPU_ISA.h"
#include <cstdint>
#include <cstring>
#include <cmath>

#include "FallbackIO.h"
#include "../support/NeonLog.h"

bool embeddingCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr || op.instr_size < sizeof(NeonCPUFallbackEmbeddingInstr)) {
        uart_puts("[ERROR] embeddingCPUFallback: instruction too small\r\n");
        return false;
    }

    const auto* I = static_cast<const NeonCPUFallbackEmbeddingInstr*>(op.instr_ptr);
    if (I->fallback_id != NEON_CPU_FALLBACK_EMBEDDING) {
        uart_puts("[ERROR] embeddingCPUFallback: incorrect fallback_id\r\n");
        return false;
    }
    if (!model.weight_buffer) {
        uart_puts("[ERROR] embeddingCPUFallback: null weight_buffer\r\n");
        return false;
    }
    if (I->index_bytes != 4 && I->index_bytes != 8) {
        uart_puts("[ERROR] embeddingCPUFallback: invalid index_bytes (must be 4 or 8)\r\n");
        return false;
    }
    if (I->num_indices == 0 || I->vocab_size == 0 || I->embed_dim == 0) {
        uart_puts("[ERROR] embeddingCPUFallback: zero dimension\r\n");
        return false;
    }

    const uint8_t* src = neon_fb::srcPtr<const uint8_t>(op, model, I->src_offset);
    int8_t* dst = neon_fb::dstPtr<int8_t>(op, model, I->dst_offset);
    if (!src || !dst) {
        uart_puts("[ERROR] embeddingCPUFallback: unbound operand\r\n");
        return false;
    }

    const uint8_t* wb = static_cast<const uint8_t*>(model.weight_buffer);
    const int8_t* tab = reinterpret_cast<const int8_t*>(wb + I->table_offset);

    const float* bias = nullptr;
    if (I->has_bias) {
        uintptr_t bias_addr = reinterpret_cast<uintptr_t>(wb + I->bias_offset);
        if ((bias_addr & 0x3u) != 0) {
            uart_puts("[ERROR] embeddingCPUFallback: bias_offset not 4-byte aligned\r\n");
            return false;
        }
        bias = reinterpret_cast<const float*>(wb + I->bias_offset);
    }

    const uint32_t num_indices = I->num_indices;
    const uint32_t vocab_size = I->vocab_size;
    const uint32_t embed_dim = I->embed_dim;
    const uint8_t index_bytes = I->index_bytes;
    const float s_t = I->table_scale;
    const int32_t zp_t = I->table_zero_point;
    const float s_o = I->output_scale;
    const int32_t zp_o = I->output_zero_point;

    for (uint32_t l = 0; l < num_indices; ++l) {
        int64_t id = 0;
        if (index_bytes == 8) {
            std::memcpy(&id, src + static_cast<size_t>(l) * 8, sizeof(int64_t));
        } else {
            int32_t id32 = 0;
            std::memcpy(&id32, src + static_cast<size_t>(l) * 4, sizeof(int32_t));
            id = id32;
        }

        if (id < 0) {
            id += vocab_size;
        }
        if (id < 0 || id >= static_cast<int64_t>(vocab_size)) {
            uart_puts("[ERROR] embeddingCPUFallback: index out of range\r\n");
            return false;
        }

        const size_t tab_row_offset = static_cast<size_t>(id) * embed_dim;
        const size_t out_row_offset = static_cast<size_t>(l) * embed_dim;

        for (uint32_t c = 0; c < embed_dim; ++c) {
            float f = static_cast<float>(static_cast<int32_t>(tab[tab_row_offset + c]) - zp_t) * s_t;
            if (bias) {
                f += bias[out_row_offset + c];
            }
            int32_t q = static_cast<int32_t>(nearbyintf(f / s_o)) + zp_o;
            if (q < -128) {
                q = -128;
            } else if (q > 127) {
                q = 127;
            }
            dst[out_row_offset + c] = static_cast<int8_t>(q);
        }
    }

    return true;
}

NEON_REGISTER_CPU_FALLBACK(NEON_CPU_FALLBACK_EMBEDDING, embeddingCPUFallback)
