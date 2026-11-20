#include "OperandBinder.h"
#include "../../core/NPUUnsupportedOpFallback.h"
#include "../support/NeonLog.h"
#include <algorithm>
#include <cstring>

namespace {

uint64_t addr(const void* p) { return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(p)); }

// Writable view of op.instr_ptr; nullptr unless it lies inside the model's
// own instruction_buffer (never patch caller memory).
uint8_t* mutableInstr(NPUModel& m, const RuntimeOp& op) {
    if (!op.instr_ptr || m.instruction_buffer.empty()) return nullptr;
    uint8_t* base = m.instruction_buffer.data();
    const uint8_t* p = static_cast<const uint8_t*>(op.instr_ptr);
    if (p < base || p >= base + m.instruction_buffer.size()) return nullptr;
    return base + (p - base);
}

template <typename I>
void fillSrcDst(uint8_t* bytes, size_t size, uint8_t* in, uint8_t* out) {
    if (size < sizeof(I)) return;
    I* i = reinterpret_cast<I*>(bytes);
    if (i->src_offset == 0) i->src_offset = addr(in);
    if (i->dst_offset == 0) i->dst_offset = addr(out);
}

} // namespace

uint8_t* OperandBinder::tensorPtr(const NPUModel& m, const FragmentBinding& b, uint32_t idx) {
    if (idx < b.num_inputs && idx < NEON_GRAPH_MAX_FRAG_INPUTS && b.inputs[idx]) return b.inputs[idx];
    if (idx == m.input_tensor_idx && b.num_inputs > 0 && b.inputs[0]) return b.inputs[0];
    if (idx == m.output_tensor_idx) return b.output;
    if (idx < m.tensors.size() && m.tensors[idx].data) return static_cast<uint8_t*>(m.tensors[idx].data);
    return b.output;
}

void OperandBinder::bindTensors(NPUModel& m, const FragmentBinding& b) {
    const size_t need = std::max<size_t>((size_t)m.output_tensor_idx + 1,
                                         std::max<size_t>((size_t)m.input_tensor_idx + 1, b.num_inputs));
    if (m.tensors.size() < need) m.tensors.resize(need);   // value-initialised TensorDesc
    if (b.num_inputs > 0 && b.inputs[0]) m.tensors[m.input_tensor_idx].data = b.inputs[0];
    for (uint32_t k = 0; k < b.num_inputs && k < NEON_GRAPH_MAX_FRAG_INPUTS; ++k)
        if (b.inputs[k]) m.tensors[k].data = b.inputs[k];
    TensorDesc& out = m.tensors[m.output_tensor_idx];
    out.data = b.output;
    if (out.size_bytes == 0) out.size_bytes = b.output_bytes;
}

void OperandBinder::fillFallbackOperands(NPUModel& m, const FragmentBinding& b) {
    for (const RuntimeOp& op : m.ops) {
        if (op.opcode != NeonOpcode::CPU_FALLBACK || op.instr_size < sizeof(NeonCPUFallbackInstr))
            continue;
        uint8_t* bytes = mutableInstr(m, op);
        if (!bytes) continue;
        NeonCPUFallbackInstr hdr;
        std::memcpy(&hdr, bytes, sizeof(hdr));
        uint8_t* in  = tensorPtr(m, b, op.input_tensor_idx);
        uint8_t* out = tensorPtr(m, b, op.output_tensor_idx);
        const size_t n = op.instr_size;

        switch (hdr.fallback_id) {
        case NEON_CPU_FALLBACK_TRANSPOSE:  fillSrcDst<NeonCPUFallbackTransposeInstr>(bytes, n, in, out); break;
        case NEON_CPU_FALLBACK_IM2COL:     fillSrcDst<NeonCPUFallbackIm2ColInstr>(bytes, n, in, out);    break;
        case NEON_CPU_FALLBACK_DW_IM2COL:  fillSrcDst<NeonCPUFallbackDWIm2ColInstr>(bytes, n, in, out);  break;
        case NEON_CPU_FALLBACK_DWCONV:     fillSrcDst<NeonCPUFallbackDWConvInstr>(bytes, n, in, out);    break;
        case NEON_CPU_FALLBACK_SLICE:      fillSrcDst<NeonCPUFallbackSliceInstr>(bytes, n, in, out);     break;
        case NEON_CPU_FALLBACK_STRIDED_SLICE: fillSrcDst<NeonCPUFallbackStridedSliceInstr>(bytes, n, in, out); break;
        case NEON_CPU_FALLBACK_PAD:        fillSrcDst<NeonCPUFallbackPadInstr>(bytes, n, in, out);       break;
        case NEON_CPU_FALLBACK_RESIZE_NN:  fillSrcDst<NeonCPUFallbackResizeInstr>(bytes, n, in, out);    break;
        case NEON_CPU_FALLBACK_MAXPOOL:    fillSrcDst<NeonCPUFallbackMaxPoolInstr>(bytes, n, in, out);   break;
        case NEON_CPU_FALLBACK_SOFTMAX:    fillSrcDst<NeonCPUFallbackSoftmaxInstr>(bytes, n, in, out);   break;
        case NEON_CPU_FALLBACK_LAYERNORM:  fillSrcDst<NeonCPUFallbackLayerNormInstr>(bytes, n, in, out); break;
        case NEON_CPU_FALLBACK_EMBEDDING:  fillSrcDst<NeonCPUFallbackEmbeddingInstr>(bytes, n, in, out); break;
        case NEON_CPU_FALLBACK_LUT: {
            if (n < sizeof(NeonCPUFallbackLutInstr)) break;
            auto* i = reinterpret_cast<NeonCPUFallbackLutInstr*>(bytes);
            if (i->src_offset == 0)
                i->src_offset = addr(op.input_tensor_idx != op.output_tensor_idx ? in : out);
            if (i->dst_offset == 0) i->dst_offset = addr(out);
            break;
        }
        case NEON_CPU_FALLBACK_CONST_COPY: {
            if (n < sizeof(NeonCPUFallbackConstCopyInstr)) break;
            auto* i = reinterpret_cast<NeonCPUFallbackConstCopyInstr*>(bytes);
            if (i->dst_offset == 0) i->dst_offset = addr(out);
            break;
        }
        case NEON_CPU_FALLBACK_CONCAT: {
            if (n < sizeof(NeonCPUFallbackConcatInstr)) break;
            auto* cc = reinterpret_cast<NeonCPUFallbackConcatInstr*>(bytes);
            if (cc->dst_offset == 0) cc->dst_offset = addr(out);
            const uint32_t num = cc->num_inputs;
            if (n < sizeof(NeonCPUFallbackConcatInstr) + (size_t)num * sizeof(NeonCPUFallbackConcatInput)) break;
            auto* ins = reinterpret_cast<NeonCPUFallbackConcatInput*>(bytes + sizeof(NeonCPUFallbackConcatInstr));
            const uint32_t dims[4] = {cc->out_n, cc->out_h, cc->out_w, cc->out_c};
            const uint64_t total = (uint64_t)dims[0] * dims[1] * dims[2] * dims[3];
            const uint64_t unit  = (cc->axis < 4 && dims[cc->axis]) ? total / dims[cc->axis] : 0;
            uint64_t cur = addr(in);
            for (uint32_t k = 0; k < num; ++k) {
                if (ins[k].src_offset != 0) continue;            // already absolute (segment parity)
                ins[k].src_offset = (k < b.num_inputs && k < NEON_GRAPH_MAX_FRAG_INPUTS && b.inputs[k])
                                        ? addr(b.inputs[k]) : cur;
                cur += unit * ins[k].channels;
            }
            break;
        }
        default:
            break;  // CONV2D fallback resolves its tensors itself
        }
    }
}

NPUDriverStatus OperandBinder::bind(NPUModel& m, const FragmentBinding& b, NPUDriver* driver) {
    if (b.num_inputs > NEON_GRAPH_MAX_FRAG_INPUTS) return NPUDriverStatus::ERROR_INVALID_ADDR;
    bindTensors(m, b);
    fillFallbackOperands(m, b);
    if (!driver) return NPUDriverStatus::OK;
    for (const RuntimeOp& op : m.ops) {
        uint8_t* bytes = mutableInstr(m, op);
        if (!bytes || op.instr_size == 0) continue;
        OpRelocation r{op.opcode, bytes, op.instr_size, op.input_tensor_idx, op.output_tensor_idx,
                       tensorPtr(m, b, op.input_tensor_idx), tensorPtr(m, b, op.output_tensor_idx)};
        const NPUDriverStatus st = driver->relocateInstruction(r, b);
        if (st != NPUDriverStatus::OK) {
            uart_put_u32("[OperandBinder] ERROR: device refused relocation of op ", op.index, "\r\n");
            return st;
        }
    }
    return NPUDriverStatus::OK;
}
