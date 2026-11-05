cmake_minimum_required(VERSION 3.21)
project(__APP_NAME__ C ASM)

# Fallback when each field variable NEON_MDF_PATH cannot be processed cmake (e.g. VS Code CMake Tools
# does not recognize cmake.environment and spawn) - from deriving the neon-mdf/ value from the
# corresponding value of this app. This dan path (__NEON_MDF_RELATIVE_PATH__) is actually calculated
# by the application creation extension (see createApp.ts) - not "../../neon-mdf" (only used if the
# male app uses two sublevels neon-mdf/, e.g. default examples/<X>/; it will be different to choose
# a custom SDK path or create an app on another value via "Duyet...").
if(NOT DEFINED ENV{NEON_MDF_PATH})
  set(ENV{NEON_MDF_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/__NEON_MDF_RELATIVE_PATH__")
endif()
include($ENV{NEON_MDF_PATH}/tools/cmake/project.cmake)

# FU2 does not have `bootloader:` in YAML (NEON_MDF_BOOTLOADER_ENABLED=OFF, see
# mcu-description/devices/FU2.yaml) - build raw flash, only 1 executable.
# WEACT_F412/CH32V003F4P6 support this: build two executables, "bootloader" (provided by the SDK
# same main(), see components/bootloader/) + app (runs under bootloader, 1 single image via
# neon_mdf_add_app_executable() - no default OTA, see examples/Blinky/ if necessary to have real OTA
# via flashmap.csv). Build/flash/ debug via neon-mdf-tools extension (VS Code) or
# neon-mdf/tools/neon_mdf.py (CLI) - no need for .vscode/tasks.json/launch.json.
if(NEON_MDF_BOOTLOADER_ENABLED)
  neon_mdf_add_bootloader_executable(bootloader)

  neon_mdf_add_app_executable(__APP_NAME__ Src/main.c)

  # flash_bootloader/flash_uart/serial_monitor/flash_all - custom target
  # shared (defined in neon-mdf, see cmake/flash_targets.cmake). CH32V003F4P6 does not have real-line
  # compatible flash_jtag/flash_ota (using separate wlink, not wire and extension - see memory) -
  # call is still error-free, only target CMake will not be able to flash the actual chip.
  include(${NEON_MDF_PATH}/cmake/flash_targets.cmake)
  neon_mdf_add_flash_targets(bootloader __APP_NAME__ ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
else()
  neon_mdf_add_executable(__APP_NAME__ Src/main.c)
endif()
