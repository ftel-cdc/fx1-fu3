cmake_minimum_required(VERSION 3.21)
project(__APP_NAME__ C ASM)

# Fallback khi bien moi truong NEON_MDF_PATH khong toi duoc process cmake
# (vd: VS Code CMake Tools khong luon truyen cmake.environment vao spawn) -
# tu suy ra vi tri neon-mdf/ tu vi tri tuong doi cua app nay. Duong dan nay
# (__NEON_MDF_RELATIVE_PATH__) duoc tinh THAT boi extension luc tao app (xem
# createApp.ts) - khong phai luon la "../../neon-mdf" (chi dung neu app
# nam dung 2 cap duoi neon-mdf/, vd examples/<X>/ mac dinh; se khac
# neu chon SDK path tuy chinh hoac tao app o vi tri khac qua "Duyet...").
if(NOT DEFINED ENV{NEON_MDF_PATH})
  set(ENV{NEON_MDF_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/__NEON_MDF_RELATIVE_PATH__")
endif()
include($ENV{NEON_MDF_PATH}/tools/cmake/project.cmake)

# FU2 CHUA co `bootloader:` trong YAML (NEON_MDF_BOOTLOADER_ENABLED=OFF, xem
# mcu-description/devices/FU2.yaml) - build nguyen flash, chi 1 executable.
# WEACT_F412/CH32V003F4P6 CO - build 2 executable: "bootloader" (SDK tu
# cung main(), xem components/bootloader/) + app (chay duoi bootloader, 1
# anh duy nhat qua neon_mdf_add_app_executable() - KHONG OTA mac dinh, xem
# examples/Blinky/ neu can vi du co OTA that qua flashmap.csv). Build/flash/
# debug qua neon-mdf-tools extension (VS Code) hoac neon-mdf/tools/neon_mdf.py
# (CLI) - khong can .vscode/tasks.json/launch.json.
if(NEON_MDF_BOOTLOADER_ENABLED)
  neon_mdf_add_bootloader_executable(bootloader)

  neon_mdf_add_app_executable(__APP_NAME__ Src/main.c)

  # flash_bootloader/flash_uart/serial_monitor/flash_all - custom target
  # DUNG CHUNG (dinh nghia trong neon-mdf, xem cmake/flash_targets.cmake).
  # CH32V003F4P6 CHUA co flash_jtag/flash_ota tuong duong that (dung wlink
  # rieng, chua wire vao extension - xem memory) - goi ham van khong loi,
  # chi la target CMake se khong flash duoc that tren chip do.
  include(${NEON_MDF_PATH}/cmake/flash_targets.cmake)
  neon_mdf_add_flash_targets(bootloader __APP_NAME__ ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
else()
  neon_mdf_add_executable(__APP_NAME__ Src/main.c)
endif()
