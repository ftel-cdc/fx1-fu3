cmake_minimum_required(VERSION 3.21)
project(blinky C ASM)

# Fallback when variables for each field NEON_MDF_PATH cannot be processed cmake (e.g. VS Code CMake
# Tools does not recognize cmake.environment and spawn) - from deriving the value neon-mdf/ from the
# corresponding value of this app.
if(NOT DEFINED ENV{NEON_MDF_PATH})
  set(ENV{NEON_MDF_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../neon-mdf")
endif()
include($ENV{NEON_MDF_PATH}/tools/cmake/project.cmake)

# FU2 does not yet have `bootloader:` in YAML (NEON_MDF_BOOTLOADER_ENABLED=OFF, see
# mcu-description/devices/FU2.yaml) - build payload + header for RTL BootROM v5, only 1 executable
# "blinky" only, without OTA because no SDK bootloader provides flashmap support.
#
# WEACT_F412 and CH32V003F4P6 (add 2026-08-10, first RISC-V target) both have - build 3 executable:
# "bootloader" (SDK from the same main(), see
# components/bootloader/) + "blinky" (factory, slot "app") + "blinky_ota"
# (slot "app_ota") - same as 1 Src/main.c, different linker script (created from flashmap.<DEVICE>.csv
# or default flashmap.csv, see cmake/flashmap.cmake) + macro NEON_OTA_APP_VERSION - see
# examples/OTA_Demo/ for a complete OTA example, including a bad image for rollback testing. flashmap.csv defaults to
# WEACT_F412) and flashmap.CH32V003F4P6.csv (private override, much smaller 16KB flash) this file
# scene - 1 app, multiple layout partitions (each device 1 flash size varies).
if(NEON_MDF_BOOTLOADER_ENABLED)
  neon_mdf_add_bootloader_executable(bootloader)

  # flashmap.csv/flashmap.<DEVICE>.csv processed automatically by neon-mdf/CMakeLists.txt (see
  # cmake/flashmap.cmake) before this point: NEON_MDF_FLASHMAP_APP_BASE/
  # APP_OTA_BASE/OTADATA_BASE and the neon_mdf_ota target are already available.

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
  # shared (defined in neon-mdf, see cmake/flash_targets.cmake) - presents directly in VS Code's CMake
  # Tools status bar (same as menuconfig), no separate .vscode/tasks.json needed. CH32V003F4P6 does
  # not yet have a matching flash_jtag/flash_ota (using openocd - via ARM/SWD message, CH32V003
  # needs its own wlink, see plan RISC-V) - only WEACT_F412 can use this real 2 target, but calls
  # error-free for CH32V003F4P6 (only the CMake target will not actually flash on that chip, for
  # Phase 4).
  include(${NEON_MDF_PATH}/cmake/flash_targets.cmake)
  neon_mdf_add_flash_targets(bootloader blinky ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
  neon_mdf_add_ota_flash_target(blinky_ota ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
else()
  neon_mdf_add_executable(blinky Src/main.c)
endif()
