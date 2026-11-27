#ifndef TEST_SERVICE_FX1_FW_H
#define TEST_SERVICE_FX1_FW_H
#include <stdint.h>
typedef enum { FX1_DMA_TO_DEVICE, FX1_DMA_FROM_DEVICE } fx1_dma_dir_t;
static inline uint32_t fx1_hartid(void) { return 0; }
static inline void fx1_mb(void) { }
static inline void fx1_dma_prepare(void *a, uint32_t n, fx1_dma_dir_t d) { (void)a; (void)n; (void)d; }
static inline void fx1_dma_complete(void *a, uint32_t n, fx1_dma_dir_t d) { (void)a; (void)n; (void)d; }
#endif
