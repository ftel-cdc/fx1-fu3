#pragma once

// ============================================================
// NEON STUDIO — runtime logging contract
//
// runtime/ prints through two C functions that EVERY platform linking the
// runtime already provides:
//   bare metal target      : bsp/uart.c
//   QEMU MPS3              : device/qemu_mps3_an547/baremetal/semihosting_uart.c
//   host builds            : analyse/host_uart.cpp
// runtime/ only DECLARES them here. It must never include a board header
// to obtain them — runtime must stay device-agnostic.
// ============================================================

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
void uart_puts(const char* s);
void uart_put_u32(const char* prefix, uint32_t value, const char* suffix);
#ifdef __cplusplus
}
#endif
