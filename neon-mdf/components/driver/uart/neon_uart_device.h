/** @file neon_uart_device.h
 * @brief Device-specific UART instance aliases.
 */
#ifndef __NEON_UART_DEVICE_H__
#define __NEON_UART_DEVICE_H__
#include "mcu_device.h"
typedef UART_TypeDef *neon_uart_t;
#if defined(UART0)
#define NEON_UART0 UART0
#endif
#if defined(UART1)
#define NEON_UART1 UART1
#endif
#if defined(UART2)
#define NEON_UART2 UART2
#endif
#if defined(UART3)
#define NEON_UART3 UART3
#endif
#if defined(UART4)
#define NEON_UART4 UART4
#endif
#if defined(USART1)
#define NEON_UART0 USART1
#endif
#endif
