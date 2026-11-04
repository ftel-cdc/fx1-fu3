/**
 * @file riscv_soft_math.c
 * @brief Implementation for riscv soft math.
 * @details
 * * riscv_soft_math.c - tu viet __udivsi3/__umodsi3/__mulsi3 (khong dung
 * libgcc he thong).
 *
 * LY DO: RV32EC (QingKe V2A cua CH32V003) KHONG co extension M (nhan/chia
 * phan cung) - BAT KY phep nhan/chia 32-bit nao trong C deu can goi ham
 * runtime cua compiler (thuong tu libgcc). Toolchain dang dung
 * (riscv64-elf-gcc qua Homebrew) KHONG co san multilib khop dung
 * `rv32ec_zicsr`/`ilp32e` (verify THAT qua `-print-multi-lib`: chi co
 * rv32i/rv32im/rv32iac/rv32imac/rv32imafc, TOAN BO deu ghep voi ABI
 * `ilp32` chuan 32 thanh ghi, KHONG co bien the `ilp32e` giam con 16 thanh
 * ghi nao ca) - dung `-lgcc` se lay NHAM libgcc cua rv64 mac dinh, gay loi
 * "ELFCLASS64 incompatible with ELFCLASS32" luc link.
 *
 * FIX: tu cung cap 3 ham nay, bien dich CUNG `-march=rv32ec_zicsr
 * -mabi=ilp32e` voi moi file khac (qua neon_mdf_device_<TARGET> - xem
 * cmake/mcu_variants.cmake) - dam bao dung ABI, khong phu thuoc libgcc he
 * thong nao ca. Thuat toan don gian (bit-serial, khong toi uu toc do) - du
 * dung cho cac phep tinh khong-hot-path hien co (vd SystemCoreClock/1000
 * trong bootloader) - neu sau nay can toi uu hon, thay the o day, KHONG
 * doi ten symbol (GCC tu goi dung ten nay theo quy uoc ABI chuan).
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

/* Chia+du (khong dau) chung 1 vong lap - tranh can them __mulsi3 rieng cho
 * umodsi3 (num - quot*den). */
static uint32_t static_riscv_soft_math_udivmod(uint32_t num, uint32_t den, uint32_t *rem_out)
{
  if (den == 0u) {
    if (rem_out) {
      *rem_out = 0u;
    }
    return 0u; /* chia cho 0 - hanh vi khong xac dinh theo C, tra ve 0 thay vi treo */
  }

  uint32_t quot = 0u;
  uint32_t qbit = 1u;

  /* Dich den len toi khi bit cao nhat cham bit 31 (hoac vuot num, dung
   * lai) - dong bo qbit cung dich theo. */
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

/* __udivdi3/__umoddi3 - chia 64-bit (khong dau) - them 2026-08-10, can boi
 * FreeRTOS-Kernel: macro pdMS_TO_TICKS() (rtos/FreeRTOS-Kernel/include/projdefs.h)
 * dung `uint64_t` LAM TRUNG GIAN MOT CACH CO CHU Y (tranh tran so voi
 * configTICK_RATE_HZ lon) NGAY CA KHI gia tri thuc te nho (vd 200ms*1000Hz)
 * - GCC luon sinh lenh chia 64-bit THEO KIEU C cua bieu thuc, khong theo gia
 * tri runtime, nen BAT KY app FreeRTOS nao goi vTaskDelay(pdMS_TO_TICKS(...))
 * (rat pho bien, vd examples/FreeRTOS_Blinky) deu can __udivdi3. Cung 1 ly
 * do/thuat toan bit-serial nhu 32-bit o tren (chi <<,>>,>=,-= tren uint64_t -
 * GCC tu lower thanh cap lenh 32-bit thuong, KHONG goi lai __udivdi3 nen
 * KHONG de quy). */
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
