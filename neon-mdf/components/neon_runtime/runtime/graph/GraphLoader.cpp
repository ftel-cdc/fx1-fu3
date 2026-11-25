#include "GraphLoader.h"
#include "ArenaPlanner.h"
#include "../loader/ModelLoader.h"
#include "../support/NeonLog.h"
#include <algorithm>
#include <cstring>
#include <memory>

namespace {

bool rangeOk(uint64_t off, uint64_t len, size_t total) {
    return off <= total && len <= (uint64_t)total - off;
}

NPUGraph* fail(std::string* err, const char* msg) {
    if (err) *err = msg;
    uart_puts("[GraphLoader] ERROR: ");
    uart_puts(msg);
    uart_puts("\r\n");
    return nullptr;
}

bool readNsmHeader(const uint8_t* p, size_t n, NeonFileHeader& h) {
    if (!p || n < sizeof(NeonFileHeader)) return false;
    std::memcpy(&h, p, sizeof(h));
    return std::memcmp(h.magic, NEON_MAGIC, 8) == 0;
}

uint64_t memplanSize(const uint8_t* nsm, size_t n, const NeonFileHeader& h, uint32_t idx) {
    if (h.memplan_offset == 0) return 0;
    for (uint32_t i = 0; i < h.num_tensors; ++i) {
        const uint64_t off = h.memplan_offset + (uint64_t)i * sizeof(NeonMemAlloc);
        if (!rangeOk(off, sizeof(NeonMemAlloc), n)) return 0;
        NeonMemAlloc a;
        std::memcpy(&a, nsm + off, sizeof(a));
        if (a.tensor_idx == idx) return a.size_bytes;
    }
    return 0;
}

std::string fixedString(const char* p, size_t n) {
    std::string s(p, n);
    s.resize(std::strlen(s.c_str()));
    return s;
}

NPUGraph* wrapSingle(const uint8_t* buf, size_t size, const DeviceTarget* target,
                     uint32_t n_in, std::string* err) {
    if (!ModelLoader::validateMemory(buf, size, target, err))
        return fail(err, "single-program .nsm failed validation");
    NeonFileHeader h;
    if (!readNsmHeader(buf, size, h)) return fail(err, "bad NEON_MAGIC header");
    if (n_in == 0) n_in = 1;
    if (n_in > NEON_GRAPH_MAX_FRAG_INPUTS) return fail(err, "too many single-program inputs");

    std::unique_ptr<NPUGraph> g(new NPUGraph());
    g->single_program = true;
    g->target_device  = fixedString(h.target_device, sizeof(h.target_device));

    GraphFragment f;
    f.nsm           = buf;
    f.nsm_size      = size;
    f.num_inputs    = n_in;
    f.source_index  = 0;
    f.scratch_bytes = (size_t)h.scratch_buffer_size;
    for (uint32_t k = 0; k < n_in; ++k) {
        GraphTensor t;
        t.size_bytes    = memplanSize(buf, size, h, k == 0 ? h.input_tensor_idx : k);
        t.kind          = NEON_GRAPH_TENSOR_INPUT;
        t.last_consumer = 0;
        g->tensors.push_back(t);
        g->inputs.push_back(k);
        f.input_tensors[k] = k;
    }
    GraphTensor out;
    out.size_bytes    = memplanSize(buf, size, h, h.output_tensor_idx);
    out.producer      = 0;
    out.last_consumer = 1;
    out.kind          = NEON_GRAPH_TENSOR_OUTPUT;
    f.output_tensor   = (uint32_t)g->tensors.size();
    g->outputs.push_back(f.output_tensor);
    g->tensors.push_back(out);
    g->max_scratch_bytes = f.scratch_bytes;
    g->fragments.push_back(f);
    ArenaPlanner::plan(*g);
    return g.release();
}

NPUGraph* parseContainer(const uint8_t* buf, size_t size, const DeviceTarget* target,
                         std::string* err) {
    NeonGraphHeader h;
    if (size < sizeof(h)) return fail(err, "buffer smaller than NeonGraphHeader");
    std::memcpy(&h, buf, sizeof(h));
    if (h.format_version < NEON_GRAPH_FORMAT_VERSION_MIN || h.format_version > NEON_GRAPH_FORMAT_VERSION)
        return fail(err, "unsupported graph format_version");
    if (h.isa_version_major != NEON_ISA_VERSION_MAJOR) return fail(err, "graph ISA major version mismatch");
    if (h.file_size > size) return fail(err, "graph truncated (file_size > buffer)");
    if (h.num_fragments == 0 || h.num_tensors == 0) return fail(err, "empty graph");
    if (!rangeOk(h.tensors_offset, (uint64_t)h.num_tensors * sizeof(NeonGraphTensor), size) ||
        !rangeOk(h.fragments_offset, (uint64_t)h.num_fragments * sizeof(NeonGraphFragment), size) ||
        !rangeOk(h.io_offset, (uint64_t)(h.num_inputs + h.num_outputs) * sizeof(uint32_t), size))
        return fail(err, "graph table out of bounds");

    std::unique_ptr<NPUGraph> g(new NPUGraph());
    g->target_device = fixedString(h.target_device, sizeof(h.target_device));
    if (target && g->target_device != target->getName()) {
        uart_puts("[GraphLoader] WARN: graph compiled for '");
        uart_puts(g->target_device.c_str());
        uart_puts("'\r\n");
    }

    g->tensors.resize(h.num_tensors);
    for (uint32_t t = 0; t < h.num_tensors; ++t) {
        NeonGraphTensor rt;
        std::memcpy(&rt, buf + h.tensors_offset + (uint64_t)t * sizeof(rt), sizeof(rt));
        if (rt.kind > NEON_GRAPH_TENSOR_ALIAS ||
            (rt.kind == NEON_GRAPH_TENSOR_ALIAS && h.format_version < 3u))
            return fail(err, "bad tensor kind");
        GraphTensor& gt  = g->tensors[t];
        gt.size_bytes    = rt.size_bytes;
        gt.producer      = rt.producer;
        gt.last_consumer = rt.last_consumer;
        gt.kind          = rt.kind;
        if (rt.kind == NEON_GRAPH_TENSOR_CONST) {
            if (rt.producer != NEON_GRAPH_NO_TENSOR || rt.data_offset == 0 ||
                rt.data_offset % NEON_GRAPH_ALIGN != 0 || !rangeOk(rt.data_offset, rt.size_bytes, size))
                return fail(err, "const tensor data out of bounds");
            gt.data = buf + rt.data_offset;    // zero-copy: buf must outlive the graph (already required)
        }
        if (rt.kind == NEON_GRAPH_TENSOR_ALIAS) {
            gt.alias_of     = rt.alias_of;
            gt.alias_offset = rt.data_offset;
        }
    }

    // A view lies inside a non-view parent; the parent's lifetime covers each view that is written
    // (the linker extends it, checked here).
    for (uint32_t t = 0; t < h.num_tensors; ++t) {
        const GraphTensor& v = g->tensors[t];
        if (v.kind != NEON_GRAPH_TENSOR_ALIAS) continue;
        if (v.alias_of >= h.num_tensors || g->tensors[v.alias_of].kind == NEON_GRAPH_TENSOR_ALIAS)
            return fail(err, "alias parent out of range or itself an alias");
        const GraphTensor& p = g->tensors[v.alias_of];
        if (v.alias_offset + v.size_bytes > p.size_bytes) return fail(err, "alias outside its parent");
        if (p.kind == NEON_GRAPH_TENSOR_INTERNAL && v.producer != NEON_GRAPH_NO_TENSOR &&
            (v.producer < p.producer || v.last_consumer > p.last_consumer))
            return fail(err, "alias outlives its parent");
    }

    const uint8_t* io = buf + h.io_offset;
    for (uint32_t i = 0; i < h.num_inputs + h.num_outputs; ++i) {
        uint32_t tid;
        std::memcpy(&tid, io + (uint64_t)i * sizeof(uint32_t), sizeof(tid));
        const bool is_in = i < h.num_inputs;
        if (tid >= h.num_tensors ||
            g->tensors[tid].kind != (is_in ? NEON_GRAPH_TENSOR_INPUT : NEON_GRAPH_TENSOR_OUTPUT))
            return fail(err, "graph I/O table names a tensor of the wrong kind");
        (is_in ? g->inputs : g->outputs).push_back(tid);
    }

    for (uint32_t f = 0; f < h.num_fragments; ++f) {
        NeonGraphFragment rf;
        std::memcpy(&rf, buf + h.fragments_offset + (uint64_t)f * sizeof(rf), sizeof(rf));
        if (!rangeOk(rf.nsm_offset, rf.nsm_size, size) || rf.nsm_size == 0)
            return fail(err, "fragment payload out of bounds");
        if (rf.nsm_offset % NEON_GRAPH_ALIGN != 0) return fail(err, "fragment payload misaligned");
        if (rf.num_inputs > NEON_GRAPH_MAX_FRAG_INPUTS) return fail(err, "fragment has too many inputs");
        if (rf.output_tensor >= h.num_tensors || g->tensors[rf.output_tensor].producer != f)
            return fail(err, "fragment output tensor / producer mismatch");
        if (g->tensors[rf.output_tensor].kind == NEON_GRAPH_TENSOR_ALIAS) {
            const uint8_t pk = g->tensors[g->tensors[rf.output_tensor].alias_of].kind;
            if (pk == NEON_GRAPH_TENSOR_INPUT || pk == NEON_GRAPH_TENSOR_CONST)
                return fail(err, "fragment writes into a read-only tensor");
        }
        GraphFragment gf;
        gf.nsm           = buf + rf.nsm_offset;
        gf.nsm_size      = (size_t)rf.nsm_size;
        gf.output_tensor = rf.output_tensor;
        gf.num_inputs    = rf.num_inputs;
        gf.source_index  = rf.source_index;
        for (uint32_t k = 0; k < rf.num_inputs; ++k) {
            const uint32_t tid = rf.input_tensors[k];
            if (tid != NEON_GRAPH_NO_TENSOR) {
                if (tid >= h.num_tensors) return fail(err, "fragment input id out of range");
                const GraphTensor& it = g->tensors[tid];
                const bool timed = it.kind == NEON_GRAPH_TENSOR_INTERNAL ||
                                   (it.kind == NEON_GRAPH_TENSOR_ALIAS && it.producer != NEON_GRAPH_NO_TENSOR);
                if (timed && (it.producer >= f || it.last_consumer < f))
                    return fail(err, "fragment reads a tensor outside its lifetime");
            }
            gf.input_tensors[k] = tid;
        }
        NeonFileHeader fh;
        if (!readNsmHeader(gf.nsm, gf.nsm_size, fh)) return fail(err, "fragment is not a NEON_MAGIC .nsm");
        if (!ModelLoader::validateMemory(gf.nsm, gf.nsm_size, target, nullptr))
            return fail(err, "fragment failed ModelLoader::validateMemory");
        gf.scratch_bytes     = (size_t)fh.scratch_buffer_size;
        g->max_scratch_bytes = std::max(g->max_scratch_bytes, gf.scratch_bytes);
        g->fragments.push_back(gf);
    }

    ArenaPlanner::plan(*g);
    return g.release();
}

} // namespace

bool GraphLoader::isGraphContainer(const uint8_t* buf, size_t size) {
    return buf && size >= 8 && std::memcmp(buf, NEON_GRAPH_MAGIC, 8) == 0;
}

NPUGraph* GraphLoader::loadFromMemory(const uint8_t* buf, size_t size, const DeviceTarget* target,
                                      uint32_t single_program_inputs, std::string* err) {
    if (!buf || size < 8) return fail(err, "empty model buffer");
    if (isGraphContainer(buf, size)) return parseContainer(buf, size, target, err);
    if (std::memcmp(buf, NEON_MAGIC, 8) == 0)
        return wrapSingle(buf, size, target, single_program_inputs, err);
    return fail(err, "unknown model magic (expected NEONGRF or NEONNSM)");
}
