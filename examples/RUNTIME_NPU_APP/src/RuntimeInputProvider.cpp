#include "RuntimeInputProvider.h"

namespace {
constexpr size_t kInputBytes = 1u * 3u * 32u * 32u;
constexpr uint32_t kDefaultGaussianSeed = 0x4E454F4Eu; /* "NEON" */
constexpr int32_t kGaussianUniformCount = 12;
constexpr int32_t kUniformSumMean = kGaussianUniformCount * 255 / 2;
/* Sum-of-12 uniform bytes has sigma ~= 256; 16/100 gives int8 sigma ~= 41,
 * matching the compiler's quantized Gaussian sample distribution. */
constexpr int32_t kGaussianScaleNumerator = 16;
constexpr int32_t kGaussianScaleDenominator = 100;
uint32_t g_rng_state = kDefaultGaussianSeed;

uint32_t nextRandom()
{
    uint32_t x = g_rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g_rng_state = x;
    return x;
}

int8_t nextGaussianInt8()
{
    int32_t sum = 0;
    for (int32_t i = 0; i < kGaussianUniformCount; ++i)
        sum += static_cast<int32_t>(nextRandom() & 0xFFu);

    int32_t value = ((sum - kUniformSumMean) * kGaussianScaleNumerator) /
                    kGaussianScaleDenominator;
    if (value < -128) value = -128;
    if (value > 127) value = 127;
    return static_cast<int8_t>(value);
}
}

extern "C" void RuntimeNpuAppSetGaussianSeed(uint32_t seed)
{
    g_rng_state = seed ? seed : kDefaultGaussianSeed;
}

extern "C" int RuntimeNpuAppPrepareInput(void *raw_in0, size_t capacity,
                                         DLTensor *tensor)
{
    if (!raw_in0 || !tensor || capacity < kInputBytes)
        return -1;

    int8_t *dst = static_cast<int8_t *>(raw_in0);
    for (size_t i = 0; i < kInputBytes; ++i)
        dst[i] = nextGaussianInt8();

    static int64_t shape[4] = {1, 3, 32, 32};
    tensor->data = raw_in0;
    tensor->device.device_type = kDLCPU;
    tensor->device.device_id = 0;
    tensor->ndim = 4;
    tensor->dtype.code = kDLInt;
    tensor->dtype.bits = 8;
    tensor->dtype.lanes = 1;
    tensor->shape = shape;
    tensor->strides = nullptr;
    tensor->byte_offset = 0;
    return 0;
}
