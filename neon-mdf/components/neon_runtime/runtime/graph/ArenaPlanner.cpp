#include "ArenaPlanner.h"
#include <algorithm>

namespace {
struct Block { uint64_t off; uint64_t size; };
struct Live  { uint32_t tensor; uint64_t off; uint64_t size; };

uint64_t alignUp(uint64_t v, uint64_t a) { return (v + a - 1) / a * a; }

void coalesce(std::vector<Block>& fl) {
    std::sort(fl.begin(), fl.end(), [](const Block& a, const Block& b) { return a.off < b.off; });
    std::vector<Block> out;
    for (const Block& b : fl) {
        if (!out.empty() && out.back().off + out.back().size == b.off) out.back().size += b.size;
        else out.push_back(b);
    }
    fl.swap(out);
}
} // namespace

size_t ArenaPlanner::plan(NPUGraph& g, size_t alignment) {
    if (alignment == 0) alignment = 1;
    const uint32_t nf = static_cast<uint32_t>(g.fragments.size());
    // INTERNAL tensors to place at each fragment: those whose (first) writer it is. Without views this
    // is exactly fragments[f].output_tensor; a concat parent of views is placed at its first view's writer.
    std::vector<std::vector<uint32_t>> starts(nf);
    for (uint32_t t = 0; t < g.tensors.size(); ++t)
        if (g.tensors[t].kind == NEON_GRAPH_TENSOR_INTERNAL && g.tensors[t].producer < nf)
            starts[g.tensors[t].producer].push_back(t);

    std::vector<Block> free_list;
    std::vector<Live>  live;
    uint64_t high = 0;

    for (uint32_t f = 0; f < nf; ++f) {
        // Release every block whose last reader ran strictly before f.
        for (size_t i = 0; i < live.size();) {
            if (g.tensors[live[i].tensor].last_consumer < f) {
                free_list.push_back({live[i].off, live[i].size});
                live.erase(live.begin() + i);
            } else {
                ++i;
            }
        }
        coalesce(free_list);

        for (uint32_t t : starts[f]) {
            GraphTensor& gt = g.tensors[t];
            const uint64_t need = alignUp(std::max<uint64_t>(gt.size_bytes, 1), alignment);
            int best = -1;
            for (size_t i = 0; i < free_list.size(); ++i)
                if (free_list[i].size >= need &&
                    (best < 0 || free_list[i].size < free_list[(size_t)best].size))
                    best = (int)i;

            uint64_t off;
            if (best >= 0) {
                Block& b = free_list[(size_t)best];
                off = b.off;
                if (b.size > need) { b.off += need; b.size -= need; }
                else free_list.erase(free_list.begin() + best);
            } else {
                off = high;
                high += need;
            }
            gt.arena_offset = off;
            live.push_back({t, off, need});
        }
    }
    g.arena_bytes = (size_t)high;
    return g.arena_bytes;
}
