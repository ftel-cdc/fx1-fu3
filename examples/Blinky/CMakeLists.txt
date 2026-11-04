cmake_minimum_required(VERSION 3.21)
project(blinky C ASM)

# Fallback khi bien moi truong NEON_MDF_PATH khong toi duoc process cmake
# (vd: VS Code CMake Tools khong luon truyen cmake.environment vao spawn) -
# tu suy ra vi tri neon-mdf/ tu vi tri tuong doi cua app nay.
if(NOT DEFINED ENV{NEON_MDF_PATH})
  set(ENV{NEON_MDF_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../neon-mdf")
endif()
include($ENV{NEON_MDF_PATH}/tools/cmake/project.cmake)

# FU2 CHUA co `bootloader:` trong YAML (NEON_MDF_BOOTLOADER_ENABLED=OFF, xem
# mcu-description/devices/FU2.yaml) - build payload + header cho RTL BootROM v5, chi 1
# executable "blinky" duy nhat, KHONG co OTA (khong co bootloader lam nen
# tang cho flashmap).
#
# WEACT_F412 va CH32V003F4P6 (them 2026-08-10, target RISC-V dau tien) DEU
# CO - build 3 executable: "bootloader" (SDK tu cung main(), xem
# components/bootloader/) + "blinky" (factory, slot "app") + "blinky_ota"
# (slot "app_ota") - CUNG 1 Src/main.c, khac linker script (sinh tu
# flashmap.<DEVICE>.csv hoac flashmap.csv mac dinh, xem
# cmake/flashmap.cmake) + macro NEON_OTA_APP_VERSION - xem
# examples/OTA_Demo/ cho vi du OTA day du (co ban "xau" test rollback).
# flashmap.csv (mac dinh, cho WEACT_F412) va flashmap.CH32V003F4P6.csv
# (override rieng, flash 16KB nho hon rat nhieu) canh file nay - 1 app,
# NHIEU layout partition (moi device 1 kich thuoc flash khac nhau).
if(NEON_MDF_BOOTLOADER_ENABLED)
  neon_mdf_add_bootloader_executable(bootloader)

  # flashmap.csv/flashmap.<DEVICE>.csv xu ly TU DONG boi neon-mdf/CMakeLists.txt
  # (xem cmake/flashmap.cmake) TRUOC khi toi day - NEON_MDF_FLASHMAP_APP_BASE/
  # APP_OTA_BASE/OTADATA_BASE... va target neon_mdf_ota da co san.

  # The application uses the bootloader UART API directly on WEACT_F412.
  # It no longer supplies a printf/_write retarget for that family.
  function(_blinky_add_variant name linker_script version)
    add_executable(${name} Src/main.c)
    target_link_libraries(${name} PRIVATE neon_mdf_driver neon_mdf_bootloader_uart neon_mdf_ota)
    target_compile_definitions(${name} PRIVATE NEON_OTA_APP_VERSION=${version})
    target_link_options(${name} PRIVATE -T${linker_script} -Wl,-Map=$<TARGET_FILE:${name}>.map)
    set_target_properties(${name} PROPERTIES LINK_DEPENDS ${linker_script})
    add_custom_command(TARGET ${name} POST_BUILD
      COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${name}> $<TARGET_FILE:${name}>.bin
      COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${name}>
      COMMENT "${name}: objcopy -> .bin + size report"
    )
  endfunction()

  set(_blinky_ld_dir ${CMAKE_BINARY_DIR}/flashmap)
  _blinky_add_variant(blinky     ${_blinky_ld_dir}/app.ld     1)
  _blinky_add_variant(blinky_ota ${_blinky_ld_dir}/app_ota.ld 2)

  # flash_bootloader/flash_uart/serial_monitor/flash_all - custom target
  # DUNG CHUNG (dinh nghia trong neon-mdf, xem cmake/flash_targets.cmake) -
  # hien thang trong CMake Tools status bar cua VS Code (giong menuconfig),
  # KHONG can .vscode/tasks.json rieng nua. CH32V003F4P6 CHUA co
  # flash_jtag/flash_ota tuong duong (dung openocd - qua dac thu ARM/SWD,
  # CH32V003 can wlink rieng, xem plan RISC-V) - chi WEACT_F412 dung duoc 2
  # target nay that su, nhung goi ham khong loi cho CH32V003F4P6 (chi la
  # target CMake se khong flash duoc that tren chip do, cho Phase 4).
  include(${NEON_MDF_PATH}/cmake/flash_targets.cmake)
  neon_mdf_add_flash_targets(bootloader blinky ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
  neon_mdf_add_ota_flash_target(blinky_ota ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
else()
  neon_mdf_add_executable(blinky Src/main.c)
endif()
