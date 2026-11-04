/**
 * @file riscv_soft_math.c
 * @brief Implementation for riscv soft math.
 * @details
 * * riscv_soft_math.c provides __udivsi3/__umodsi3/__mulsi3 without the system libgcc.
 *
 * RV32EC on CH32V003 QingKe V2A lacks the M extension for hardware multiply/divide, so C operations
 * require compiler runtime helpers. The Homebrew riscv64-elf-gcc toolchain has no matching
 * `rv32ec_zicsr`/`ilp32e` multilib. Verified with `-print-multi-lib`:
 * rv32i/rv32im/rv32iac/rv32imac/rv32imafc all use the 32-register `ilp32` ABI, without a
 * 16-register `ilp32e` variant. -lgcc selects the default rv64 library and causes
 * "ELFCLASS64 incompatible with ELFCLASS32" at link time.
 *
 * Provide these three helpers and compile with the same `-march=rv32ec_zicsr
 * -mabi=ilp32e` settings as every other file through neon_mdf_device_<TARGET>
 * (cmake/mcu_variants.cmake). This guarantees the correct ABI without system libgcc. Simple
 * bit-serial algorithms suffice for current noncritical calculations, such as SystemCoreClock/1000
 * in the bootloader. Optimize here if needed, retaining the symbol names GCC expects by ABI
 * convention.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/

#include <stdint.h>

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/

static uint32_t static_riscv_soft_math_udivmod(uint32_t num, uint32_t den, uint32_t *rem_out);
static uint64_t static_riscv_soft_math_udivmod64(uint64_t num, uint64_t den, uint64_t *rem_out);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/

/**
 * @brief   udivsi3.
 * @param[in] a A.
 * @param[in] b B.
 * @return The requested value or operation result.
 */
uint32_t __udivsi3(uint32_t a, uint32_t b)
{
  return static_riscv_soft_math_udivmod(a, b, 0);
}

/**
 * @brief   umodsi3.
 * @param[in] a A.
 * @param[in] b B.
 * @return The requested value or operation result.
 */
uint32_t __umodsi3(uint32_t a, uint32_t b)
{
  uint32_t rem;
  static_riscv_soft_math_udivmod(a, b, &rem);
  return rem;
}

/**
 * @brief   mulsi3.
 * @param[in] a A.
 * @param[in] b B.
 * @return The requested value or operation result.
 */
uint32_t __mulsi3(uint32_t a, uint32_t b)
{
  uint32_t result = 0u;
  while (b != 0u) {
    if (b & 1u) {
      result += a;
    }
    a <<= 1;
    b >>= 1;
  }
  return result;
}

/**
 * @brief   udivdi3.
 * @param[in] a A.
 * @param[in] b B.
 * @return The requested value or operation result.
 */
uint64_t __udivdi3(uint64_t a, uint64_t b)
{
  return static_riscv_soft_math_udivmod64(a, b, 0);
}

/**
 * @brief   umoddi3.
 * @param[in] a A.
 * @param[in] b B.
 * @return The requested value or operation result.
 */
uint64_t __umoddi3(uint64_t a, uint64_t b)
{
  uint64_t rem;
  static_riscv_soft_math_udivmod64(a, b, &rem);
  return rem;
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/

/* Unsigned division and remainder share one loop, avoiding an extra __mulsi3 call for
 * umodsi3 (num - quot*den). */
static uint32_t static_riscv_soft_math_udivmod(uint32_t num, uint32_t den, uint32_t *rem_out)
{
  if (den == 0u) {
    if (rem_out) {
      *rem_out = 0u;
    }
    return 0u; /* Division by zero is undefined in C; return 0 instead of hanging */
  }

  uint32_t quot = 0u;
  uint32_t qbit = 1u;

  /* Shift the denominator and qbit together until its highest bit reaches bit 31 or it exceeds num. */
  while ((int32_t)den >= 0 && den <= num) {
    den <<= 1;
    qbit <<= 1;
  }

  while (qbit != 0u) {
    if (num >= den) {
      num -= den;
      quot |= qbit;
    }
    den >>= 1;
    qbit >>= 1;
  }

  if (rem_out) {
    *rem_out = num;
  }
  return quot;
}

/* __udivdi3/__umoddi3 provide unsigned 64-bit division; added 2026-08-10 for
 * FreeRTOS-Kernel: macro pdMS_TO_TICKS() (rtos/FreeRTOS-Kernel/include/projdefs.h)
 * deliberately uses `uint64_t` intermediate values to prevent overflow at large configTICK_RATE_HZ,
 * even when actual values are small, e.g. 200ms*1000Hz.
 * - GCC emits 64-bit division according to the C expression type, regardless of runtime values. FreeRTOS
 * apps calling vTaskDelay(pdMS_TO_TICKS(...)), such as examples/FreeRTOS_Blinky, therefore require
 * __udivdi3. The same bit-serial algorithm uses only <<, >>, >= and -= on uint64_t, lowered by GCC
 * to ordinary paired 32-bit operations. It does not call __udivdi3 recursively. */
static uint64_t static_riscv_soft_math_udivmod64(uint64_t num, uint64_t den, uint64_t *rem_out)
{
  if (den == 0u) {
    if (rem_out) {
      *rem_out = 0u;
    }
    return 0u;
  }

  uint64_t quot = 0u;
  uint64_t rem = 0u;

  for (int32_t i = 63; i >= 0; i--) {
    rem = (rem << 1) | ((num >> i) & 1u);
    if (rem >= den) {
      rem -= den;
      quot |= ((uint64_t)1u << i);
    }
  }

  if (rem_out) {
    *rem_out = rem;
  }
  return quot;
}
