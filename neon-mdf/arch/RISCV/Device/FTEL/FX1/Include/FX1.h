/* Sinh tu mcu-description/devices/FX1.yaml; khong sua truc tiep. */
#ifndef NEON_DEVICE_FX1_H
#define NEON_DEVICE_FX1_H
#include "fx1/fx1_memory_map.h"
#define NEON_MDF_DEVICE_NAME "FX1"
#if FX1_NUM_HARTS != 0x2
#error "FX1.yaml khong khop BSP: FX1_NUM_HARTS"
#endif
#if FX1_MTIME_HZ != 0x1312D00
#error "FX1.yaml khong khop BSP: FX1_MTIME_HZ"
#endif
#if FX1_PLIC_NUM_SOURCES != 0xA
#error "FX1.yaml khong khop BSP: FX1_PLIC_NUM_SOURCES"
#endif
#if FX1_BOOTROM_BASE != 0x0
#error "FX1.yaml khong khop BSP: FX1_BOOTROM_BASE"
#endif
#if FX1_BOOTROM_SIZE != 0x200000
#error "FX1.yaml khong khop BSP: FX1_BOOTROM_SIZE"
#endif
#if FX1_SRAM_BASE != 0x1000000
#error "FX1.yaml khong khop BSP: FX1_SRAM_BASE"
#endif
#if FX1_SRAM_SIZE != 0x100000
#error "FX1.yaml khong khop BSP: FX1_SRAM_SIZE"
#endif
#if FX1_DDR_BASE != 0x80000000
#error "FX1.yaml khong khop BSP: FX1_DDR_BASE"
#endif
#if FX1_DDR_SIZE != 0x20000000
#error "FX1.yaml khong khop BSP: FX1_DDR_SIZE"
#endif
#if FX1_ISP_CSR_BASE != 0x11000000
#error "FX1.yaml khong khop BSP: FX1_ISP_CSR_BASE"
#endif
#if FX1_CLINT_BASE != 0x2000000
#error "FX1.yaml khong khop BSP: FX1_CLINT_BASE"
#endif
#if FX1_PLIC_BASE != 0xC000000
#error "FX1.yaml khong khop BSP: FX1_PLIC_BASE"
#endif
#if FX1_UART_BASE != 0x10000000
#error "FX1.yaml khong khop BSP: FX1_UART_BASE"
#endif
#if FX1_SYS_DMA_CSR_BASE != 0x10010000
#error "FX1.yaml khong khop BSP: FX1_SYS_DMA_CSR_BASE"
#endif
#if FX1_SIM_CTRL_BASE != 0x10020000
#error "FX1.yaml khong khop BSP: FX1_SIM_CTRL_BASE"
#endif
#if FX1_I2C0_BASE != 0x10030000
#error "FX1.yaml khong khop BSP: FX1_I2C0_BASE"
#endif
#if FX1_NPU_CSR_BASE != 0x11010000
#error "FX1.yaml khong khop BSP: FX1_NPU_CSR_BASE"
#endif
#if FX1_H264_H265_CSR_BASE != 0x11020000
#error "FX1.yaml khong khop BSP: FX1_H264_H265_CSR_BASE"
#endif
#if FX1_ETH_CSR_BASE != 0x11030000
#error "FX1.yaml khong khop BSP: FX1_ETH_CSR_BASE"
#endif
#if FX1_MIPI_CSR_BASE != 0x11040000
#error "FX1.yaml khong khop BSP: FX1_MIPI_CSR_BASE"
#endif
#if FX1_DDR3_CSR_BASE != 0x12000000
#error "FX1.yaml khong khop BSP: FX1_DDR3_CSR_BASE"
#endif
#if FX1_DDR_SYSCTRL_BASE != 0x12010000
#error "FX1.yaml khong khop BSP: FX1_DDR_SYSCTRL_BASE"
#endif
#if FX1_IRQ_UART != 0x1
#error "FX1.yaml khong khop BSP: FX1_IRQ_UART"
#endif
#if FX1_IRQ_SYS_DMA != 0x2
#error "FX1.yaml khong khop BSP: FX1_IRQ_SYS_DMA"
#endif
#if FX1_IRQ_ISP != 0x3
#error "FX1.yaml khong khop BSP: FX1_IRQ_ISP"
#endif
#if FX1_IRQ_NPU != 0x4
#error "FX1.yaml khong khop BSP: FX1_IRQ_NPU"
#endif
#if FX1_IRQ_H264_H265 != 0x5
#error "FX1.yaml khong khop BSP: FX1_IRQ_H264_H265"
#endif
#if FX1_IRQ_ETH != 0x6
#error "FX1.yaml khong khop BSP: FX1_IRQ_ETH"
#endif
#if FX1_IRQ_MIPI != 0x7
#error "FX1.yaml khong khop BSP: FX1_IRQ_MIPI"
#endif
#if FX1_IRQ_DDR != 0x8
#error "FX1.yaml khong khop BSP: FX1_IRQ_DDR"
#endif
#if FX1_IRQ_I2C0 != 0x9
#error "FX1.yaml khong khop BSP: FX1_IRQ_I2C0"
#endif
#ifndef __ASSEMBLER__
#include "fx1_registers.h"
#endif
#endif
