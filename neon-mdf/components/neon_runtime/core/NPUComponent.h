#pragma once

// ============================================================
// NEON STUDIO — NPU COMPONENT
//
// Standalone NPU-specific interface — NOT tied to any particular
// device's identity. Provides:
//   1. NPU architecture type    (Systolic Array vs 3D Cube Matrix)
//   2. Dataflow strategy        (Output / Weight / Input Stationary)
//   3. Matrix block size        (SA: rows×cols, Cube: M×K×N)
//   4. NPU hardware specs       (TOPS, SRAM, DRAM BW, DMA...)
//   5. Op / dtype support       (native vs decomposed)
//   6. Roofline helpers         (ridge point, compute-bound check)
//
// A device attaches this component via multiple inheritance alongside
// its own DeviceTarget-derived identity class — it does not carry
// identity (getName/getDescription) itself:
//
//   DeviceTarget (identity — core/DeviceTarget.h)
//       └── QEMUMPS3Target          ← getName(), getDescription()
//               └── QEMUMPS3NPUTarget : public QEMUMPS3Target, public NPUComponent
//                       ├── QEMUMPS3Compiler (toolkit/)
//                       └── QEMUMPS3Runtime  (runtime/)
//   (VP_FX1_V2.0: NPUV4Runtime : DeviceTarget, NPUComponent, NPUDriver.)
//
//   NPUComponent (this file — standalone, no base)
//
// Same pattern for QEMUMPS3Target → QEMUMPS3NPUTarget → QEMUMPS3Compiler/Runtime.
//
// Does NOT include MLIR headers — safe to include from runtime.
// ============================================================

#include "NPU_ISA.h"

#include <cstdint>
#include <cstddef>
#include <cstdio>   // printf in printArchSummary()

#ifdef __cplusplus
extern "C++" {

// ============================================================
// NPU ARCHITECTURE TYPE
//
// Describes the fundamental compute structure of the NPU's
// matrix multiply unit.
// ============================================================

enum class NPUArchType {
    // Systolic Array (SA)
    // —————————————————
    // 2D mesh of PEs. Data flows through the array in a wave —
    // inputs move right, weights move down (or vice versa).
    // Latency ∝ array dimension; throughput = rows × cols MACs/cycle.
    // Example: Google TPU, Apple Neural Engine edge variant.
    //
    //   [PE][PE][PE]...[PE]   ← inputs →
    //   [PE][PE][PE]...[PE]
    //    ↓   ↓   ↓
    //  weights
    SYSTOLIC_ARRAY,

    // 3D Cube Matrix
    // —————————————
    // 3D volume of MACs (M × K × N). Computes a full M×N output
    // block in one cycle using K-wide reduction.
    // Higher utilization for large square matmuls;
    // lower utilization for skinny matrices.
    // Example: Huawei Ascend DaVinci core (16×16×16).
    //
    //        N
    //       ╱─────────╮
    //    M ─────────╮ │
    //       │  MACs │ │  K (reduction)
    //       ╰─────────╯
    CUBE_3D,

    // Vector / SIMD Array
    // ———————————————————
    // Wide SIMD units; no 2D or 3D matrix structure.
    // Each lane computes one multiply-accumulate.
    // Example: ARM Ethos-U55/U65.
    VECTOR_ARRAY,

    // Mixed / Custom
    MIXED,
};

inline const char* npuArchTypeName(NPUArchType a) {
    switch (a) {
    case NPUArchType::SYSTOLIC_ARRAY: return "Systolic Array";
    case NPUArchType::CUBE_3D:        return "3D Cube Matrix";
    case NPUArchType::VECTOR_ARRAY:   return "Vector Array";
    case NPUArchType::MIXED:          return "Mixed";
    default:                          return "Unknown";
    }
}

// ============================================================
// NPU DATAFLOW STRATEGY
//
// Determines which operand is kept stationary in the register
// file / accumulator, while the others stream through.
// Affects on-chip data reuse and effective SRAM bandwidth.
// ============================================================

enum class NPUDataflow {
    // Output Stationary (OS)
    // ——————————————————————
    // Partial sums stay in the accumulator until the full output
    // element is computed. Inputs and weights stream.
    // Minimises partial-sum writeback traffic.
    // Best when: large kernel, high accumulation depth.
    OUTPUT_STATIONARY,

    // Weight Stationary (WS)
    // ——————————————————————
    // Weights are loaded once into the PE register file and reused
    // across all input spatial positions.
    // Minimises weight fetch traffic.
    // Best when: large output spatial map, small batch.
    // Common in edge NPUs (low DRAM BW budget).
    WEIGHT_STATIONARY,

    // Input Stationary (IS)
    // —————————————————————
    // Inputs stay; weights and partial sums move.
    // Minimises input fetch traffic.
    // Best when: many filters, small input.
    INPUT_STATIONARY,

    // Row Stationary (RS)
    // ———————————————————
    // A row of the convolutional filter stays in one PE.
    // Maximises reuse of all three operands simultaneously.
    // Example: MIT Eyeriss architecture.
    ROW_STATIONARY,
};

inline const char* npuDataflowName(NPUDataflow d) {
    switch (d) {
    case NPUDataflow::OUTPUT_STATIONARY: return "Output Stationary";
    case NPUDataflow::WEIGHT_STATIONARY: return "Weight Stationary";
    case NPUDataflow::INPUT_STATIONARY:  return "Input Stationary";
    case NPUDataflow::ROW_STATIONARY:    return "Row Stationary";
    default:                             return "Unknown";
    }
}

// ============================================================
// MATRIX BLOCK SIZE
//
// Physical dimensions of the MAC array / compute block.
// Interpretation depends on NPUArchType.
// ============================================================

// Systolic Array config: rows × cols MAC array
struct NPUSAConfig {
    int rows;   // number of rows in the systolic array
    int cols;   // number of columns in the systolic array
    // Peak throughput = rows × cols MACs/cycle
};

// 3D Cube config: M × K × N MAC block (Ascend DaVinci style)
struct NPUCubeConfig {
    int m;   // output rows of the cube block (e.g. 16)
    int k;   // reduction dimension (e.g. 16)
    int n;   // output cols of the cube block (e.g. 16)
    // Peak throughput = m × k × n MACs / cycle
};

// Vector config: number of SIMD lanes
struct NPUVectorConfig {
    int lanes;          // SIMD width in elements (e.g. 128)
    int bits_per_lane;  // e.g. 8 for INT8
};

struct NPUMatrixBlock {
    NPUArchType arch;
    union {
        NPUSAConfig     sa;
        NPUCubeConfig   cube;
        NPUVectorConfig vector;
    };

    // Total MACs per cycle — regardless of arch type
    int macsPerCycle() const {
        switch (arch) {
        case NPUArchType::SYSTOLIC_ARRAY: return sa.rows * sa.cols;
        case NPUArchType::CUBE_3D:        return cube.m * cube.k * cube.n;
        case NPUArchType::VECTOR_ARRAY:   return vector.lanes;
        default:                          return 0;
        }
    }
};

// ============================================================
// NPU HARDWARE SPECS
//
// Full hardware descriptor for one NPU target.
// Previously called DeviceHWSpecs (now NPU-only).
// ============================================================

struct NPUHWSpecs {
    // --- Architecture ---
    NPUArchType    arch_type;        // SA, Cube, Vector...
    NPUDataflow    dataflow;         // OS, WS, IS, RS
    NPUMatrixBlock matrix_block;     // block dimensions

    // --- Compute ---
    float    peak_tops;              // peak INT8 tera-ops/sec
    float    peak_tops_fp32;         // peak FP32 tera-ops/sec
    int      num_cores;              // number of compute cores
    float    clock_mhz;             // operating clock frequency

    // --- On-chip SRAM ---
    size_t   sram_size_bytes;        // total on-chip SRAM
    float    sram_bandwidth_gbps;    // internal SRAM bandwidth

    // --- Off-chip DRAM ---
    float    dram_bandwidth_gbps;    // external DRAM bandwidth

    // --- DMA ---
    int      num_dma_channels;
    size_t   dma_burst_bytes;        // burst transfer size

    // --- Supported precisions ---
    bool     supports_float32;
    bool     supports_int8;
    bool     supports_int4;
    bool     supports_int16;

    // --- Concurrent model execution (Rev2+) ---
    // Number of models this hardware can run truly in parallel.
    // 1 = single model only (QEMU MPS3, most NPUs)
    // 2 = dual spatial-split lanes (Rev2 ISA; no supported device today)
    // Runtime uses this to decide between hardware-parallel path
    // and software time-slice fallback — API stays the same either way.
    uint8_t  max_concurrent_models;
};

// ============================================================
// NPU COMPONENT
//
// Attach this (via multiple inheritance) to every NPU device,
// alongside its own DeviceTarget-derived identity class.
// Standalone — does not implement getName()/getDescription();
// those stay on the identity side.
// ============================================================

class NPUComponent {
public:
    virtual ~NPUComponent() = default;

    // --- NPU specs (implement in each device) ---
    virtual NPUHWSpecs getNPUSpecs() const = 0;

    // --- Architecture info (derived from specs) ---
    NPUArchType    getArchType()       const { return getNPUSpecs().arch_type; }
    NPUDataflow    getDataflow()       const { return getNPUSpecs().dataflow; }
    NPUMatrixBlock getMatrixBlock()    const { return getNPUSpecs().matrix_block; }

    // --- Compute specs ---
    float   getPeakTOPS()          const { return getNPUSpecs().peak_tops; }
    float   getPeakTOPSFP32()      const { return getNPUSpecs().peak_tops_fp32; }
    int     getNumCores()          const { return getNPUSpecs().num_cores; }
    float   getClockMHz()          const { return getNPUSpecs().clock_mhz; }
    int     getMacsPerCycle()      const { return getNPUSpecs().matrix_block.macsPerCycle(); }

    // --- Memory specs ---
    size_t  getSRAMSize()          const { return getNPUSpecs().sram_size_bytes; }
    float   getSRAMBandwidth()     const { return getNPUSpecs().sram_bandwidth_gbps; }
    float   getDRAMBandwidth()     const { return getNPUSpecs().dram_bandwidth_gbps; }
    int     getNumDMAChannels()    const { return getNPUSpecs().num_dma_channels; }
    size_t  getDMABurstBytes()     const { return getNPUSpecs().dma_burst_bytes; }

    // Which on-chip buffer a tensor class must live in. A device with one
    // unified scratchpad returns the same total for all three; a banked device
    // (V4.5: A/B/C) returns the individual capacity. Default keeps every
    // existing target's behaviour byte-for-byte.
    enum class SramClass : uint8_t { ACTIVATION, WEIGHT, PSUM };
    virtual size_t getSRAMBudget(SramClass) const { return getSRAMSize(); }

    // --- Concurrent execution ---
    uint8_t maxConcurrentModels()      const { return getNPUSpecs().max_concurrent_models; }
    bool    supportsConcurrentExecution() const { return getNPUSpecs().max_concurrent_models > 1; }

    // --- Op / dtype support ---
    // Returns true if the NPU has NATIVE hardware support.
    // False → toolkit must decompose into supported primitives.
    virtual bool supportsOp(NeonOpcode op) const = 0;

    virtual bool supportsDtype(NeonDtype dtype) const {
        const auto& s = getNPUSpecs();
        switch (dtype) {
        case NeonDtype::FLOAT32: return s.supports_float32;
        case NeonDtype::INT8:    return s.supports_int8;
        case NeonDtype::INT4:    return s.supports_int4;
        case NeonDtype::INT16:   return s.supports_int16;
        case NeonDtype::UINT8:   return s.supports_int8; // UINT8 shares INT8 HW path
        default:                 return false;
        }
    }

    // --- Cycle-based performance helpers ---
    //
    // Peak FLOPs per clock cycle for the full SA (both INT8 ops per MAC).
    // Each MAC = 1 multiply + 1 accumulate = 2 FLOPs.
    //   peakFlopsPerCycle() = sa.rows × sa.cols × 2
    //
    // Used by MeasuredProfiler to compute mac_efficiency_pct from cycle count:
    //   mac_efficiency = theoretical_flops / (cycles × peakFlopsPerCycle(lane_rows)) × 100
    uint64_t peakFlopsPerCycle() const {
        return (uint64_t)getNPUSpecs().matrix_block.macsPerCycle() * 2;
    }

    // Lane-aware variant — scales by the fraction of SA rows assigned to one lane.
    // lane_rows: rows given to this lane (e.g. 40 for Lane A with N_split=40)
    // total_rows: full SA rows (e.g. 64 on FX1)
    uint64_t peakFlopsPerCycle(uint8_t lane_rows, uint8_t total_rows) const {
        if (total_rows == 0) return peakFlopsPerCycle();
        const auto& s = getNPUSpecs();
        // lane has lane_rows rows × sa.cols columns × 2 FLOPs/MAC
        return (uint64_t)lane_rows * (uint64_t)s.matrix_block.sa.cols * 2;
    }

    // Convert cycle count to milliseconds using this target's clock frequency
    float cyclesToMs(uint64_t cycles) const {
        float clock_mhz = getNPUSpecs().clock_mhz;
        if (clock_mhz <= 0.f) return 0.f;
        return (float)cycles / (clock_mhz * 1000.f);
    }

    // --- Roofline (used by analyse/) ---
    // Ridge point = compute / memory bandwidth boundary (ops/byte)
    float getRidgePoint() const {
        const auto& s = getNPUSpecs();
        if (s.dram_bandwidth_gbps <= 0.f) return 0.f;
        // peak_tops × 1e12 ops/s ÷ (BW GB/s × 1e9 B/s)
        return (s.peak_tops * 1000.f) / s.dram_bandwidth_gbps;
    }

    // arithmetic_intensity = FLOPs / bytes accessed
    bool isComputeBound(float arithmetic_intensity) const {
        return arithmetic_intensity > getRidgePoint();
    }

    // --- Human-readable summary ---
    // name: caller passes the identity side's getName() (NPUComponent
    // has no identity of its own — see class comment above).
    void printArchSummary(const char* name) const {
        const auto& s  = getNPUSpecs();
        const auto& mb = s.matrix_block;
        printf("NPU Target : %s\n",    name);
        printf("  Arch     : %s\n",    npuArchTypeName(s.arch_type));
        printf("  Dataflow : %s\n",    npuDataflowName(s.dataflow));
        switch (s.arch_type) {
        case NPUArchType::SYSTOLIC_ARRAY:
            printf("  SA Block : %d x %d  (%d MACs/cycle)\n",
                   mb.sa.rows, mb.sa.cols, mb.macsPerCycle());
            break;
        case NPUArchType::CUBE_3D:
            printf("  Cube     : %d x %d x %d  (%d MACs/cycle)\n",
                   mb.cube.m, mb.cube.k, mb.cube.n, mb.macsPerCycle());
            break;
        case NPUArchType::VECTOR_ARRAY:
            printf("  Vector   : %d lanes x %d-bit\n",
                   mb.vector.lanes, mb.vector.bits_per_lane);
            break;
        default: break;
        }
        printf("  Peak     : %.1f TOPS (INT8)  |  %.1f GOPS (FP32)\n",
               s.peak_tops, s.peak_tops_fp32 * 1000.f);
        printf("  SRAM     : %zu KB\n",    s.sram_size_bytes / 1024);
        printf("  DRAM BW  : %.1f GB/s\n", s.dram_bandwidth_gbps);
        printf("  Ridge pt : %.1f ops/byte\n", getRidgePoint());
        printf("  DMA ch   : %d  (burst %zu B)\n",
               s.num_dma_channels, s.dma_burst_bytes);
        printf("  Concurrent: %d model(s)\n", s.max_concurrent_models);
    }
};

} // extern "C++"
#endif // __cplusplus
