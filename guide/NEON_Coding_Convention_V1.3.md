# NEON Coding Convention

## 1. Purpose

This document defines source organization, naming, header/source layout, Doxygen, API design and review rules for NEON Extension. It defines the conventions used by the GT32U2086 SDK.

Key NEON decisions:

- HAL public functions: `neon_<module>_<action>()`.
- Public types: `neon_<module>_<name>_t`; cross-module types: `neon_<name>_t`.
- Header guards: `__NEON_<MODULE>_H__` (project-specific exception; identifiers beginning `__` are reserved by ISO C/C++).
- File names, variables, and HAL public functions: lowercase `snake_case`.
- Opening braces on the same line; four-space indentation.

## 2. Source Folder And File Convention

| Item | Format | Example |
|---|---|---|
| Common entry header | `neon.h` | `neon.h` |
| Common types | `neon_types.h` | `neon_types.h` |
| Configuration | `neon_config.h` | `neon_config.h` |
| HAL module | `neon_<module>.c/.h` | `neon_uart.c`, `neon_uart.h` |
| Internal header | `neon_<module>_internal.h` | `neon_uart_internal.h` |
| SoC register header | `neon_<module>_regs.h` | `neon_uart_regs.h` |
| Shared core helper | `neon_<module>_core.c/.h` | `neon_qspi_core.c` |
| Utility | `neon_utils_<name>.c/.h` | `neon_utils_crc.c` |
| Test | `test_neon_<module>.c` | `test_neon_uart.c` |

One driver module normally has one public header and matching implementation. Hardware variants belong in platform-specific directories unless their public functionality actually differs. Do not edit third-party code merely to satisfy naming rules; isolate it behind NEON wrappers.

## 3. Naming Convention

| Item | Pattern | Example |
|---|---|---|
| HAL public function | `neon_<module>_<action>()` | `neon_uart_transmit()` |
| Private function | `<module>_<action>()` | `uart_wait_tx_ready()` |
| Handle type | `neon_<module>_handle_t` | `neon_uart_handle_t` |
| Configuration type | `neon_<module>_config_t` | `neon_uart_config_t` |
| Other struct | `neon_<module>_<name>_t` | `neon_uart_stats_t` |
| Enum type | `neon_<module>_<name>_t` | `neon_uart_state_t` |
| Enum value | `NEON_<MODULE>_<VALUE>` | `NEON_UART_STATE_BUSY` |
| Shared/common type | `neon_<name>_t` | `neon_status_t` |
| Callback typedef | `neon_<module>_<event>_callback_t` | `neon_uart_event_callback_t` |
| Public macro | `NEON_<MODULE>_<NAME>` | `NEON_UART_FIFO_SIZE` |
| Private macro | `<MODULE>_<NAME>` | `UART_TX_RETRY_LIMIT` |
| Parameter/field/local | `snake_case` | `baud_rate`, `timeout_ms` |
| Public global (avoid if possible) | `neon_<module>_<name>` | `neon_uart_default_config` |
| Header guard | `__NEON_<MODULE>_H__` | `__NEON_UART_H__` |

### 3.1 Types And Structs

Use `_t`, not `TypeDef`. Use the `_s` suffix for named struct tags when a tag is necessary. Field names do not repeat the type prefix.

The following example assumes `neon_uart_t`, `neon_uart_state_t` and `neon_uart_parity_t` have already been declared.

```c
typedef struct neon_uart_config_s {
    uint32_t baud_rate;
    uint32_t data_bits;
    neon_uart_parity_t parity;
} neon_uart_config_t;

typedef struct neon_uart_handle_s {
    neon_uart_t instance;
    neon_uart_state_t state;
} neon_uart_handle_t;
```

HAL handles contain runtime state and are managed by their driver; configuration structs contain caller-supplied settings. Do not embed the entire initialization configuration into a handle by default. If the public interface exposes the handle definition for static allocation, fields are still driver-owned; use APIs to read or change state.

### 3.2 Enums And Bitmasks

```c
typedef enum {
    NEON_UART_PARITY_NONE = 0,
    NEON_UART_PARITY_EVEN = 1,
    NEON_UART_PARITY_ODD = 2
} neon_uart_parity_t;

#define NEON_UART_EVENT_TX_DONE  (1UL << 0)
#define NEON_UART_EVENT_RX_DONE  (1UL << 1)
```

Use enums for mutually exclusive finite choices; use unsigned macros for combinable bitmasks and preprocessor constants. Assign explicit enum values when exposed in an ABI, protocol, register encoding or persistent storage. Do not use ordinal enum values as hardware bit encodings unless verified.

### 3.3 Common Status

```c
typedef enum {
    NEON_OK = 0,
    NEON_ERROR,
    NEON_BUSY,
    NEON_TIMEOUT,
    NEON_INVALID_PARAM
} neon_status_t;
```

Public operations that can fail normally return `neon_status_t`; document functions with other return semantics explicitly. Security-sensitive status codes and fault responses require a separate design review.

### 3.4 Function Verbs And Modes

| Verb | Meaning | Example |
|---|---|---|
| `init` / `deinit` | Create/reset driver runtime context | `neon_uart_init()` |
| `set_config` / `get_config` | Apply/read peripheral configuration | `neon_uart_set_config()` |
| `enable` / `disable` | Toggle feature | `neon_uart_enable()` |
| `start` / `stop` | Start/stop processing | `neon_timer_start()` |
| `read` / `write` | Memory/register/device operations | `neon_flash_read()` |
| `transmit` / `receive` | Streaming data operation | `neon_uart_transmit()` |
| `get_state` / `get_error` | Read runtime diagnostics | `neon_uart_get_state()` |
| `register_callback` | Register a callback | `neon_uart_register_callback()` |

Use `_IT` and `_DMA` suffixes only for genuine interrupt/DMA processing variants. Avoid synonyms such as `Send` and `Transmit` for the same operation in one module. Private helpers are `static`, use `snake_case`, and do not use `static_` in their names.

### 3.5 Parameters, Macros And Constants

- Name quantities with units where ambiguity is possible: `timeout_ms`, `frequency_hz`, `size_bytes`.
- Input pointers not modified by the callee must point to `const` data.
- Use `U`, `UL` or `ULL` integer suffixes appropriate to the type.
- Prefer `static inline` over side-effecting or multi-line function-like macros.
- Avoid public mutable globals and magic numbers.
- Never prefix new C identifiers with an underscore except for the explicitly chosen legacy-style header guards.

## 4. Header File Format

```c
/**
 * @file    neon_uart.h
 * @brief   NEON UART HAL driver interface.
 * @details Declares UART configuration types and public APIs.
 */

#ifndef __NEON_UART_H__
#define __NEON_UART_H__

#ifdef __cplusplus
extern "C" {
#endif

/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include <stdint.h>
#include "neon_types.h"
#include "neon_uart_device.h"

/*==================================================================================================
*                                      DEFINES AND MACROS
==================================================================================================*/
#define NEON_UART_DEFAULT_BAUD  115200U

/*==================================================================================================
*                                STRUCTURES AND OTHER TYPEDEFS
==================================================================================================*/
typedef enum {
    NEON_UART_PARITY_NONE = 0,
    NEON_UART_PARITY_EVEN,
    NEON_UART_PARITY_ODD
} neon_uart_parity_t;

typedef enum {
    NEON_UART_STATE_RESET = 0,
    NEON_UART_STATE_IDLE,
    NEON_UART_STATE_BUSY,
    NEON_UART_STATE_ERROR
} neon_uart_state_t;

typedef struct neon_uart_config_s {
    uint32_t baud_rate;
    neon_uart_parity_t parity;
} neon_uart_config_t;

typedef struct neon_uart_handle_s {
    neon_uart_t instance;
    neon_uart_state_t state;
} neon_uart_handle_t;

/*==================================================================================================
*                                    FUNCTION PROTOTYPES
==================================================================================================*/
neon_status_t neon_uart_init(neon_uart_handle_t *huart, neon_uart_t instance);
neon_status_t neon_uart_set_config(neon_uart_handle_t *huart, const neon_uart_config_t *p_config);
neon_uart_state_t neon_uart_get_state(const neon_uart_handle_t *huart);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_UART_H__ */
```

`neon_uart_t` and the available instances (for example, `NEON_UART0`) must be declared in `neon_uart_device.h`. The handle is publicly defined to support caller-owned static allocation, but its fields remain driver-owned and must not be modified by the application. Initialization and configuration are separate operations.

Header rules:

- Include the corresponding type declarations for every prototype; a public header must compile on its own.
- Include only necessary dependencies and expose only public API/types.
- Wrap exported declarations in `extern "C"` for C++ consumers.
- Do not place non-`static` definitions in headers.
- Keep third-party and SoC-specific details out of generic headers where possible.

## 5. Source File Format

```c
/**
 * @file    neon_uart.c
 * @brief   NEON UART HAL driver implementation.
 */

/*==================================================================================================
*                                        INCLUDE FILES
==================================================================================================*/
#include "neon_uart.h"
#include <stdbool.h>
#include <stddef.h>

/*==================================================================================================
*                                      LOCAL VARIABLES
==================================================================================================*/
/* Module-local variables, if required. */

/*==================================================================================================
*                                  PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/
static bool uart_is_valid(const neon_uart_handle_t *huart);

/*==================================================================================================
*                                       GLOBAL FUNCTIONS
==================================================================================================*/
neon_uart_state_t neon_uart_get_state(const neon_uart_handle_t *huart) {
    if (!uart_is_valid(huart)) {
        return NEON_UART_STATE_ERROR;
    }

    return huart->state;
}

/*==================================================================================================
*                                       PRIVATE FUNCTIONS
==================================================================================================*/
static bool uart_is_valid(const neon_uart_handle_t *huart) {
    return huart != NULL;
}
```

The snippet demonstrates naming and source layout, not a complete UART driver. A real module must also implement its declared initialization/configuration APIs. In `.c` files, `extern "C"` wrapping is unnecessary when compiled as C. If compiling `.c` as C++ is a project requirement, use an explicit build policy rather than ad hoc wrappers.

Source rules:

- Include matching public header first; include private and platform headers next.
- Group includes, local variables, private prototypes, public implementations, and private implementations under the shown section banners.
- Use four spaces, brace on the same line, and braces for every `if`, `else`, `for`, and `while`.
- Keep one blank line between logical blocks. Prefer readable line lengths; 120 columns is a recommendation, not an automatic reformatting mandate.
- Never add unused placeholders or debug globals to production implementations.

## 6. Comment And Doxygen Convention

Document every public API with `@brief`, `@param[in]` / `[out]` / `[in,out]` and `@return` (when non-void). Use `@details`, `@note`, and `@warning` as appropriate. Document units, valid ranges, blocking behavior, ISR eligibility, thread safety and ownership when relevant.

```c
/**
 * @brief      Transmit data using UART polling mode.
 * @param[in]  huart       Initialized UART handle.
 * @param[in]  data        Data buffer to transmit.
 * @param[in]  size_bytes  Number of bytes to transmit.
 * @param[in]  timeout_ms  Maximum wait time in milliseconds.
 * @return     NEON_OK on success, otherwise the error status.
 * @note       This function blocks until completion, timeout or error.
 */
neon_status_t neon_uart_transmit(neon_uart_handle_t *huart, const uint8_t *data,
                                 uint32_t size_bytes, uint32_t timeout_ms);
```

Do not add `@param None` for functions without parameters. Private helpers need comments when the intent or register side effects are not obvious.

## 7. Include And Configuration Convention

Use `neon_config.h` for compile-time features; name switches `NEON_<MODULE>_MODULE_ENABLED` and use consistent 0/1 values:

```c
#define NEON_UART_MODULE_ENABLED  1
#define NEON_SPI_MODULE_ENABLED   1
#define NEON_USE_ASSERT           1

#if defined(NEON_UART_MODULE_ENABLED) && (NEON_UART_MODULE_ENABLED == 1)
/* UART build-dependent declarations/implementation. */
#endif
```

Keep peripheral clock, pinmux and board frequencies in SoC/board configuration files. Public headers should not require unrelated application configuration to parse. `neon.h` may aggregate enabled public module headers, but individual headers remain independently includable.

## 8. Platform And Build Organization

- Keep device registers, startup, linker scripts and interrupt-vector integration in SoC/platform directories.
- Place generated artifacts under the firmware project's build/release folders, not within driver source directories.
- Use warning flags, static analysis and target-specific builds to check portability.
- Test a module under every supported SoC/compiler combination it claims to support.
- A HAL handle shall have a documented single owner. When ISR, tasks or both cores access the same instance, define synchronization and memory-order guarantees explicitly.
- Polling APIs shall use bounded timeouts where appropriate; no blocking waits inside ISR. ROM-only LL code can use deterministic bounded loops when no time base exists.

## 9. New Module Checklist

| Step | Check |
|---:|---|
| 1 | Choose HAL, LL, utility, or SoC layer with allowed dependencies |
| 2 | Name files `neon_<module>` |
| 3 | Add `__NEON_<MODULE>_H__` header guard |
| 4 | Use `neon_<module>_` for HAL functions; `NEON_<MODULE>_` for enum values and macros |
| 5 | Use `neon_<module>_<name>_t` for exported types |
| 6 | Add file header, section banners and Doxygen public APIs |
| 7 | Keep private functions `static` and `snake_case` |
| 8 | Check struct ownership, configuration/state separation |
| 9 | Check pointer `const`, units, bounds and error handling |
| 10 | Check ISR behavior, timeout, concurrent access and register side effects |
| 11 | Add feature flag only when build-time optionality is needed |
| 12 | Build, test and verify header self-containment |

## 10. Module Templates And API Lifecycle

Every new module follows the header/source layouts in Sections 4 and 5. A handle-based driver should, where relevant, separate handle initialization from peripheral configuration:

```c
neon_uart_handle_t huart;
neon_uart_config_t config = {
    .baud_rate = 115200U,
    .parity = NEON_UART_PARITY_NONE
};

neon_status_t status = neon_uart_init(&huart, NEON_UART0);
if (status == NEON_OK) {
    status = neon_uart_set_config(&huart, &config);
}
```

