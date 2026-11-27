#ifndef TEST_NEON_FREERTOS_FX1_H
#define TEST_NEON_FREERTOS_FX1_H
#include "FreeRTOS.h"
#include "fx1/fx1_memory_map.h"
typedef BaseType_t (*NEON_FX1_IRQ_Handler)(void *);
BaseType_t NEON_FX1_IRQ_Register(uint32_t, uint32_t, NEON_FX1_IRQ_Handler, void *);
BaseType_t NEON_FX1_IRQ_Unregister(uint32_t, NEON_FX1_IRQ_Handler, void *);
#endif
