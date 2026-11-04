# components/bootloader - CHI duoc add_subdirectory() khi
# NEON_MDF_BOOTLOADER_ENABLED (target dang build co khai `bootloader:` trong
# YAML - xem cmake/mcu_variants.cmake, mcu-description/generated/<DEVICE>_bootloader.cmake).
#
# 2 thu vien CON LAI (KHONG con neon_mdf_bootloader nua, xem duoi):
#   - neon_mdf_crc32: CRC32 phan mem thuan tuy (khong phu thuoc chip/family)
#     - dung boi components/ota/ (CA bootloader LAN app, xem cmake/flashmap.cmake).
#   - neon_mdf_bootloader_uart: chi UART toi thieu (init/send/recv) - AN TOAN
#     cho bat ky executable nao link (bootloader LAN app - vd app muon
#     printf() qua cung UART).
#
# neon_bootloader_main.c + neon_bootloader_arch_<family>.c (2026-08-10, tach
# rieng - xem neon_bootloader_arch.h) KHONG con nam trong 1 thu vien rieng
# (neon_mdf_bootloader) nua - thu vien do CHI TUNG duoc dung boi DUY NHAT
# executable "bootloader" (khong ai khac link toi), nen khong can boc thanh
# .a - compile TRUC TIEP nhu nguon cua chinh executable do (xem
# NEON_MDF_BOOTLOADER_MAIN_SOURCES duoi + neon_mdf_add_bootloader_executable()
# trong tools/cmake/project.cmake). Loi ich: (1) khong con can 1 file .c
# rong (neon_bootloader_stub.c) chi de thoa man add_executable() can it
# nhat 1 nguon, (2) khong con vong lap phu thuoc CAP ARCHIVE (main.c can
# SystemCoreClock cua device.a, device.a's startup.s can main() cua
# bootloader.a) tung phai fix bang -Wl,--start-group/--end-group - object
# file TRUC TIEP tren executable luon duoc linker pull vao vo dieu kien,
# khong can group nua.
if(NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "WEACT_F412")
  set(_neon_mdf_bootloader_uart_sources platform/weact_f412/neon_bootloader_uart.c)
  set(_neon_mdf_bootloader_arch_sources platform/weact_f412/neon_bootloader_arch.c)
elseif(NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "STM32F103")
  set(_neon_mdf_bootloader_uart_sources platform/stm32f103/neon_bootloader_uart.c)
  # Cortex-M3 has the same SysTick/VTOR/reset behavior as this ARMv7-M file.
  set(_neon_mdf_bootloader_arch_sources platform/weact_f412/neon_bootloader_arch.c)
elseif(NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "CH32V003")
  set(_neon_mdf_bootloader_uart_sources platform/ch32v003/neon_bootloader_uart.c)
  set(_neon_mdf_bootloader_arch_sources platform/ch32v003/neon_bootloader_arch.c)
else()
  message(FATAL_ERROR "components/bootloader: chua co UART/arch implementation cho family '${NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY}'")
endif()

add_library(neon_mdf_crc32 STATIC neon_bootloader_crc32.c)
target_include_directories(neon_mdf_crc32 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# ${NEON_MDF_DEVICE_TARGET} PUBLIC de ke thua co -mcpu/-mfloat-abi/-mfpu (Cortex-M)
# hoac -march/-mabi (RISC-V), ap dung 1 lan qua neon_mdf_apply_cortex_m_arch()/
# neon_mdf_apply_riscv_arch() trong cmake/mcu_variants.cmake) - THIEU dong nay
# se link LOI ABI mismatch (da xac nhan that ca 2 kien truc).
target_link_libraries(neon_mdf_crc32 PUBLIC ${NEON_MDF_DEVICE_TARGET})

add_library(neon_mdf_bootloader_uart STATIC ${_neon_mdf_bootloader_uart_sources})
target_include_directories(neon_mdf_bootloader_uart PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(neon_mdf_bootloader_uart PUBLIC ${NEON_MDF_DEVICE_TARGET})

# NEON_MDF_BOOTLOADER_MAIN_SOURCES - danh sach file .c CUA CHINH executable
# "bootloader" (main.c + arch file - KHONG PHAI ten 1 thu vien nua) -
# PARENT_SCOPE 2 tang (giong moi bien khac tu components/bootloader/ ra toi
# app, xem neon-mdf/CMakeLists.txt) de neon_mdf_add_bootloader_executable()
# (chay o scope cua APP, trong tools/cmake/project.cmake) dung duoc.
# Headers cua chinh main.c/arch file (neon_bootloader_arch.h, image_header.h...)
# nam trong CUNG thu muc nay - executable "bootloader" se co include dir nay
# TU DONG qua viec link neon_mdf_bootloader_uart/neon_mdf_crc32 (ca 2 deu da
# PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} nhu tren, CUNG 1 duong dan), khong can
# them gi rieng.
set(NEON_MDF_BOOTLOADER_MAIN_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/neon_bootloader_main.c
  ${CMAKE_CURRENT_SOURCE_DIR}/${_neon_mdf_bootloader_arch_sources}
  PARENT_SCOPE
)
