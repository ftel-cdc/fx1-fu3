#pragma once

// ============================================================
// NEON STUDIO — PROFILE SELECTOR  (Rev3)
//
// Bridges BatchAccumulator → Executor for batched inference:
//
//   1. select()         — pick the best BatchProfile for N requests
//   2. pack_inputs()    — build one contiguous input buffer [N × input]
//   3. unpack_outputs() — distribute results back to callers
//
// Profile selection policy:
//   - Find the smallest BatchProfile with batch_size >= n_actual.
//   - If n_actual > largest profile, use the largest profile and
//     split into multiple Executor calls (see run_with_overflow()).
//   - Dummy inputs (zeros) pad slots [n_actual .. batch_size-1].
//   - Dummy output slots are silently discarded.
//
// Input/output buffer layout (all profiles):
//   [item_0][item_1]...[item_{n_actual-1}][pad_0]...[pad_{B-1}]
//   where B = selected_profile.batch_size
//   Each item is single_input_size / single_output_size bytes.
//
// LLM note: for continuous batching (decode phase), requests at
// different decode steps can be grouped into one batch because
// the weight matrix load is amortized. The per-item scratch region
// handles separate KV-cache state per sequence.
// ============================================================

#include "BatchAccumulator.h"
#include "../loader/NPUModel.h"

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <vector>
#include <cassert>

#ifdef __cplusplus
extern "C++" {

class ProfileSelector {
public:
    // ── Profile selection ──────────────────────────────────────────────────

    // Find the best BatchProfile for n_actual requests.
    //
    // Returns the smallest profile with batch_size >= n_actual.
    // If n_actual > all profiles, returns the largest profile (caller
    // must handle overflow — see padding_needed / overflow_count below).
    // Returns nullptr if model has no batch profiles.
    static const BatchProfile* select(const NPUModel& model,
                                      uint32_t         n_actual) {
        return model.getProfile(n_actual);   // NPUModel::getProfile() saturates
    }

    // How many real items fit in this profile (min of n_actual, batch_size).
    static uint32_t items_in_profile(const BatchProfile& profile,
                                     uint32_t             n_actual) {
        return std::min(n_actual, profile.batch_size);
    }

    // How many dummy (zero-padded) slots are needed.
    static uint32_t padding_needed(const BatchProfile& profile,
                                   uint32_t             n_actual) {
        return profile.batch_size > n_actual
               ? profile.batch_size - n_actual : 0u;
    }

    // How many items exceed the profile and need a follow-up call.
    // Normally 0; only non-zero if n_actual > largest profile.
    static uint32_t overflow_count(const BatchProfile& profile,
                                   uint32_t             n_actual) {
        return n_actual > profile.batch_size
               ? n_actual - profile.batch_size : 0u;
    }

    // ── Input packing ──────────────────────────────────────────────────────

    // Build a packed input buffer for the Executor:
    //   slots 0 .. n_fit-1  : real inputs from requests[]
    //   slots n_fit .. B-1  : zero-padded dummies (B = profile.batch_size)
    //
    // Returns the packed buffer (size = profile.batch_size * single_input_size).
    // The Executor's DMA_READ instructions will read this as a single
    // contiguous buffer starting at scratch_base + input_offset.
    static std::vector<uint8_t> pack_inputs(
            const BatchProfile&               profile,
            const std::vector<InferenceRequest>& requests,
            size_t                            single_input_size)
    {
        const uint32_t n_fit = items_in_profile(profile,
                                                 (uint32_t)requests.size());
        const size_t   total = profile.batch_size * single_input_size;

        std::vector<uint8_t> buf(total, 0);   // zero-init → pads dummies

        for (uint32_t i = 0; i < n_fit; ++i) {
            assert(requests[i].input_data != nullptr);
            assert(requests[i].input_size_bytes == single_input_size);
            std::memcpy(buf.data() + i * single_input_size,
                        requests[i].input_data,
                        single_input_size);
        }
        return buf;
    }

    // Variant: pack a sub-range of requests (for overflow handling).
    static std::vector<uint8_t> pack_inputs(
            const BatchProfile&               profile,
            const std::vector<InferenceRequest>& requests,
            uint32_t                          offset,   // index into requests
            size_t                            single_input_size)
    {
        const uint32_t remaining = (uint32_t)requests.size() - offset;
        const uint32_t n_fit     = items_in_profile(profile, remaining);
        const size_t   total     = profile.batch_size * single_input_size;

        std::vector<uint8_t> buf(total, 0);

        for (uint32_t i = 0; i < n_fit; ++i) {
            const auto& req = requests[offset + i];
            assert(req.input_data != nullptr);
            std::memcpy(buf.data() + i * single_input_size,
                        req.input_data,
                        single_input_size);
        }
        return buf;
    }

    // ── Output unpacking ───────────────────────────────────────────────────

    // Copy Executor output slots back to each caller's output_data.
    // Dummy slots (i >= n_actual) are silently discarded.
    // Calls on_complete(true) on each request after copy.
    static void unpack_outputs(
            const BatchProfile&               profile,
            const uint8_t*                    output_buffer,  // Executor output
            const std::vector<InferenceRequest>& requests,
            size_t                            single_output_size,
            bool                              exec_ok = true)
    {
        const uint32_t n_fit = items_in_profile(profile,
                                                 (uint32_t)requests.size());
        for (uint32_t i = 0; i < n_fit; ++i) {
            const auto& req = requests[i];
            if (exec_ok && req.output_data != nullptr) {
                assert(req.output_size_bytes == single_output_size);
                std::memcpy(req.output_data,
                            output_buffer + i * single_output_size,
                            single_output_size);
            }
            if (req.on_complete) req.on_complete(exec_ok);
        }
    }

    // Variant: unpack from a sub-range (for overflow handling).
    static void unpack_outputs(
            const BatchProfile&               profile,
            const uint8_t*                    output_buffer,
            const std::vector<InferenceRequest>& requests,
            uint32_t                          offset,
            size_t                            single_output_size,
            bool                              exec_ok = true)
    {
        const uint32_t remaining = (uint32_t)requests.size() - offset;
        const uint32_t n_fit     = items_in_profile(profile, remaining);
        for (uint32_t i = 0; i < n_fit; ++i) {
            const auto& req = requests[offset + i];
            if (exec_ok && req.output_data != nullptr) {
                std::memcpy(req.output_data,
                            output_buffer + i * single_output_size,
                            single_output_size);
            }
            if (req.on_complete) req.on_complete(exec_ok);
        }
    }

    // ── High-level helper ──────────────────────────────────────────────────

    // Compute how many Executor calls are needed to drain all requests.
    // Normally 1. More than 1 only when n_actual > largest profile.
    //
    // Example:
    //   profiles: [batch=1, batch=4, batch=8]
    //   n_actual = 11 → ceil(11/8) = 2 calls (8 + 3 items)
    static uint32_t calls_needed(const NPUModel& model, uint32_t n_actual) {
        uint32_t max_bs = model.maxBatchSize();
        if (max_bs == 0) return 0;
        return (n_actual + max_bs - 1) / max_bs;
    }
};

} // extern "C++"
#endif
