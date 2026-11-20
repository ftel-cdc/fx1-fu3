#include "ResizeCPUFallback.h"
#include "../../core/NPU_ISA.h"
#include "core/NPUUnsupportedOpFallback.h"
#include <cstring>
#include <cmath>
#include <cstddef>
#include <algorithm>

#include "FallbackIO.h"
#include "../support/NeonLog.h"

bool resizeCPUFallback(const RuntimeOp& op, const NPUModel& model) {
    if (!op.instr_ptr) {
        uart_puts("[ERROR] resizeCPUFallback: null instr_ptr\r\n");
        return false;
    }

    const auto* instr = reinterpret_cast<const NeonCPUFallbackResizeInstr*>(op.instr_ptr);
    if (instr->opcode != NeonOpcode::CPU_FALLBACK ||
        instr->fallback_id != NEON_CPU_FALLBACK_RESIZE_NN) {
        uart_puts("[ERROR] resizeCPUFallback: invalid opcode or fallback_id\r\n");
        return false;
    }

    const int8_t* src = neon_fb::srcPtr<const int8_t>(op, model, instr->src_offset);
    int8_t* dst = neon_fb::dstPtr<int8_t>(op, model, instr->dst_offset);
    if (!src || !dst) {
        uart_puts("[ERROR] resizeCPUFallback: unbound operand\r\n");
        return false;
    }

    float height_scale = (instr->align_corners && instr->out_h > 1)
                             ? (static_cast<float>(instr->in_h - 1) / static_cast<float>(instr->out_h - 1))
                             : (static_cast<float>(instr->in_h) / static_cast<float>(instr->out_h));
    float width_scale  = (instr->align_corners && instr->out_w > 1)
                             ? (static_cast<float>(instr->in_w - 1) / static_cast<float>(instr->out_w - 1))
                             : (static_cast<float>(instr->in_w) / static_cast<float>(instr->out_w));

    auto srcRow = [&](uint32_t oh) -> int32_t {
        int32_t ih = 0;
        if (instr->half_pixel_centers) {
            ih = static_cast<int32_t>(std::floor((static_cast<float>(oh) + 0.5f) * height_scale));
        } else if (instr->align_corners && instr->out_h > 1) {
            ih = static_cast<int32_t>(std::round(static_cast<float>(oh) * height_scale));
        } else {
            ih = static_cast<int32_t>(std::floor(static_cast<float>(oh) * height_scale));
        }
        if (ih < 0) ih = 0;
        if (ih >= static_cast<int32_t>(instr->in_h)) ih = static_cast<int32_t>(instr->in_h - 1);
        return ih;
    };

    auto srcCol = [&](uint32_t ow) -> int32_t {
        int32_t iw = 0;
        if (instr->half_pixel_centers) {
            iw = static_cast<int32_t>(std::floor((static_cast<float>(ow) + 0.5f) * width_scale));
        } else if (instr->align_corners && instr->out_w > 1) {
            iw = static_cast<int32_t>(std::round(static_cast<float>(ow) * width_scale));
        } else {
            iw = static_cast<int32_t>(std::floor(static_cast<float>(ow) * width_scale));
        }
        if (iw < 0) iw = 0;
        if (iw >= static_cast<int32_t>(instr->in_w)) iw = static_cast<int32_t>(instr->in_w - 1);
        return iw;
    };

    if (instr->layout == NEON_LAYOUT_NCHW) {
        const size_t in_plane = static_cast<size_t>(instr->in_h) * instr->in_w;
        const size_t out_plane = static_cast<size_t>(instr->out_h) * instr->out_w;
        for (uint32_t c = 0; c < instr->in_c; ++c) {
            for (uint32_t oh = 0; oh < instr->out_h; ++oh) {
                const int32_t ih = srcRow(oh);
                for (uint32_t ow = 0; ow < instr->out_w; ++ow) {
                    dst[c * out_plane + static_cast<size_t>(oh) * instr->out_w + ow] =
                        src[c * in_plane + static_cast<size_t>(ih) * instr->in_w + srcCol(ow)];
                }
            }
        }
        uart_puts("[Firmware] RESIZE_NN CPU fallback executed successfully\r\n");
        return true;
    }

    for (uint32_t oh = 0; oh < instr->out_h; ++oh) {
        int32_t ih = srcRow(oh);
        for (uint32_t ow = 0; ow < instr->out_w; ++ow) {
            int32_t iw = srcCol(ow);
            const int8_t* src_pixel = src + (ih * instr->in_w + iw) * instr->in_c;
            int8_t* dst_pixel = dst + (oh * instr->out_w + ow) * instr->in_c;
            std::memcpy(dst_pixel, src_pixel, instr->in_c);
        }
    }

    uart_puts("[Firmware] RESIZE_NN CPU fallback executed successfully\r\n");
    return true;
}
