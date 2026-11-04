cmake_minimum_required(VERSION 3.21)
project(freertos_blinky C ASM)

# Fallback when variables for each field NEON_MDF_PATH cannot be processed cmake (e.g. VS Code CMake
# Tools does not recognize cmake.environment and spawn) - from deriving the value neon-mdf/ from the
# corresponding value of this app.
if(NOT DEFINED ENV{NEON_MDF_PATH})
  set(ENV{NEON_MDF_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../neon-mdf")
endif()
include($ENV{NEON_MDF_PATH}/tools/cmake/project.cmake)

include(${NEON_MDF_PATH}/cmake/freertos.cmake)
neon_mdf_add_freertos(${NEON_MDF_TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/Inc)

# FU2 does not yet have `bootloader:` in YAML (NEON_MDF_BOOTLOADER_ENABLED=OFF) - build payload +
# header for RTL BootROM v5 and one "freertos_blinky" executable, without OTA because no SDK bootloader provides
# cascade for flashmap).
#
# WEACT_F412 has a real bootloader + flashmap/OTA (see flashmap.WEACT_F412.csv scenery of this file) -
# build 3 executable: "bootloader" (SDK from the same main(), see components/bootloader/) +
# "freertos_blinky" (factory, slot "app") + "freertos_blinky_ota" (slot "app_ota") - same 1
# Src/main.c, different linker script (created from flashmap.WEACT_F412.csv) + macro
# NEON_OTA_APP_VERSION; see examples/OTA_Demo/ for a complete OTA example with a bad image for rollback testing.
#
# CH32V003F4P6 has bootloader but not flashmap/OTA (2026-08-10 - the translation does not have a file
# flashmap.CH32V003F4P6.csv so: FreeRTOS-Kernel I'm missing ~8.5KB flash/~2KB RAM (that's true
# 2026-08-11: text+data=8488B/12032B, bss=1936B/2048B - via sat tran RAM), can't fit 2 image
# (factory+ota)
# + bootloader in 16KB flash/2KB real RAM of the chip - see early comment Src/main.c and
# .claude/plans/proud-sniffing-teacup.md) - only build 1 executable via
# neon_mdf_add_app_executable() (linker script is specified from YAML's `bootloader:`, see
# tools/cmake/project.cmake). The bootloader validates header/CRC32 at boot and accepts UART updates,
# but flash cannot hold A/B images or automatic rollback metadata.
if(NEON_MDF_BOOTLOADER_ENABLED)
  neon_mdf_add_bootloader_executable(bootloader)
  include(${NEON_MDF_PATH}/cmake/flash_targets.cmake)

  if(NEON_MDF_APP_HAS_FLASHMAP)
    # flashmap.WEACT_F412.csv processed automatically by neon-mdf/CMakeLists.txt (see
    # cmake/flashmap.cmake) before this point: NEON_MDF_FLASHMAP_APP_BASE/
    # APP_OTA_BASE/OTADATA_BASE and the neon_mdf_ota target are already available.
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

    # flash_bootloader/flash_uart/serial_monitor/flash_all – see cmake/flash_targets.cmake (generic, same
    # 1 definition as Blinky).
    neon_mdf_add_flash_targets(bootloader freertos_blinky ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
    neon_mdf_add_ota_flash_target(freertos_blinky_ota ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
  else()
    # CH32V003F4P6: 1 executable only, no OTA - see comment above.
    neon_mdf_add_app_executable(freertos_blinky Src/main.c)
    target_link_libraries(freertos_blinky PRIVATE freertos_kernel)
    target_compile_definitions(freertos_blinky PRIVATE NEON_OTA_APP_VERSION=1)

    # CH32V003F4P6 does not yet have a compatible flash_jtag/flash_ota (using openocd - via the ARM/SWD
    # message, CH32V003 requires its own wlink, see plan RISC-V) - call is still error-free (only
    # CMake targets will not flash on that chip, for Phase 4).
    neon_mdf_add_flash_targets(bootloader freertos_blinky ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
  endif()
else()
  neon_mdf_add_executable(freertos_blinky Src/main.c)
  target_link_libraries(freertos_blinky PRIVATE freertos_kernel)
endif()
