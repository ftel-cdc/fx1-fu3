cmake_minimum_required(VERSION 3.21)
project(freertos_blinky C ASM)

# Fallback khi bien moi truong NEON_MDF_PATH khong toi duoc process cmake
# (vd: VS Code CMake Tools khong luon truyen cmake.environment vao spawn) -
# tu suy ra vi tri neon-mdf/ tu vi tri tuong doi cua app nay.
if(NOT DEFINED ENV{NEON_MDF_PATH})
  set(ENV{NEON_MDF_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../neon-mdf")
endif()
include($ENV{NEON_MDF_PATH}/tools/cmake/project.cmake)

include(${NEON_MDF_PATH}/cmake/freertos.cmake)
neon_mdf_add_freertos(${NEON_MDF_TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/Inc)

# FU2 CHUA co `bootloader:` trong YAML (NEON_MDF_BOOTLOADER_ENABLED=OFF) -
# build payload + header cho RTL BootROM v5, chi 1 executable "freertos_blinky" duy
# nhat, KHONG co OTA (khong co bootloader lam nen tang cho flashmap).
#
# WEACT_F412 CO bootloader + flashmap/OTA that (xem flashmap.WEACT_F412.csv
# canh file nay) - build 3 executable: "bootloader" (SDK tu cung main(),
# xem components/bootloader/) + "freertos_blinky" (factory, slot "app") +
# "freertos_blinky_ota" (slot "app_ota") - CUNG 1 Src/main.c, khac linker
# script (sinh tu flashmap.WEACT_F412.csv) + macro NEON_OTA_APP_VERSION -
# xem examples/OTA_Demo/ cho vi du OTA day du (co ban "xau" test rollback).
#
# CH32V003F4P6 CO bootloader NHUNG KHONG flashmap/OTA (2026-08-10 - CHU
# DICH KHONG co file flashmap.CH32V003F4P6.csv nao ca: FreeRTOS-Kernel toi
# thieu da ~8.5KB flash/~2KB RAM (do that 2026-08-11: text+data=8488B/12032B, bss=1936B/2048B - qua sat tran RAM), KHONG THE fit 2 ban image (factory+ota)
# + bootloader trong 16KB flash/2KB RAM that cua chip - xem comment dau
# Src/main.c va .claude/plans/proud-sniffing-teacup.md) - chi build 1
# executable qua neon_mdf_add_app_executable() (linker script CO DINH tu
# YAML's `bootloader:`, xem tools/cmake/project.cmake) - bootloader van
# THAT (verify header/CRC32 luc boot, nhan firmware moi qua UART), chi
# KHONG co du phong A/B/tu dong rollback nhu nhanh flashmap.
if(NEON_MDF_BOOTLOADER_ENABLED)
  neon_mdf_add_bootloader_executable(bootloader)
  include(${NEON_MDF_PATH}/cmake/flash_targets.cmake)

  if(NEON_MDF_APP_HAS_FLASHMAP)
    # flashmap.WEACT_F412.csv xu ly TU DONG boi neon-mdf/CMakeLists.txt (xem
    # cmake/flashmap.cmake) TRUOC khi toi day - NEON_MDF_FLASHMAP_APP_BASE/
    # APP_OTA_BASE/OTADATA_BASE... va target neon_mdf_ota da co san.
    function(_freertos_blinky_flashmap_variant name linker_script version)
      add_executable(${name} Src/main.c)
      target_link_libraries(${name} PRIVATE neon_mdf_driver neon_mdf_ota freertos_kernel)
      target_compile_definitions(${name} PRIVATE NEON_OTA_APP_VERSION=${version})
      target_link_options(${name} PRIVATE -T${linker_script} -Wl,-Map=$<TARGET_FILE:${name}>.map)
      set_target_properties(${name} PROPERTIES LINK_DEPENDS ${linker_script})
      add_custom_command(TARGET ${name} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${name}> $<TARGET_FILE:${name}>.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${name}>
        COMMENT "${name}: objcopy -> .bin + size report"
      )
    endfunction()

    set(_freertos_blinky_ld_dir ${CMAKE_BINARY_DIR}/flashmap)
    _freertos_blinky_flashmap_variant(freertos_blinky     ${_freertos_blinky_ld_dir}/app.ld     1)
    _freertos_blinky_flashmap_variant(freertos_blinky_ota ${_freertos_blinky_ld_dir}/app_ota.ld 2)

    # flash_bootloader/flash_uart/serial_monitor/flash_all - xem
    # cmake/flash_targets.cmake (dung chung, cung 1 dinh nghia voi Blinky).
    neon_mdf_add_flash_targets(bootloader freertos_blinky ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
    neon_mdf_add_ota_flash_target(freertos_blinky_ota ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
  else()
    # CH32V003F4P6: 1 executable duy nhat, khong OTA - xem comment tren.
    neon_mdf_add_app_executable(freertos_blinky Src/main.c)
    target_link_libraries(freertos_blinky PRIVATE freertos_kernel)
    target_compile_definitions(freertos_blinky PRIVATE NEON_OTA_APP_VERSION=1)

    # CH32V003F4P6 CHUA co flash_jtag/flash_ota tuong duong (dung openocd -
    # qua dac thu ARM/SWD, CH32V003 can wlink rieng, xem plan RISC-V) - goi
    # ham van khong loi (chi la target CMake se khong flash duoc that tren
    # chip do, cho Phase 4).
    neon_mdf_add_flash_targets(bootloader freertos_blinky ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
  endif()
else()
  neon_mdf_add_executable(freertos_blinky Src/main.c)
  target_link_libraries(freertos_blinky PRIVATE freertos_kernel)
endif()
