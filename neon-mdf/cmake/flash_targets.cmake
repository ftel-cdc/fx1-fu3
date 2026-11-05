# flash_targets.cmake provides shared CMake targets for building and flashing bootloader/application
# images through ST-LINK. Available to apps with NEON_MDF_BOOTLOADER_ENABLED
# (cmake/mcu_variants.cmake). Like menuconfig (cmake/kconfig.cmake), these targets appear directly
# in the VS Code CMake Tools Build Target menu. Apps call neon_mdf_add_flash_targets() to register
# all three targets without separate .vscode/tasks.json files.
#
# neon_mdf_add_flash_targets(<bootloader_target> <app_target> <openocd_cfg>)
#   bootloader_target: bootloader executable target, e.g. "bootloader", created by
#   neon_mdf_add_bootloader_executable(). app_target: application executable target, e.g. "blinky",
#   created by
#                        neon_mdf_add_app_executable())
#   openocd_cfg: OpenOCD .cfg path, e.g.
#                        ${NEON_MDF_PATH}/cmake/openocd/WEACT_F412_openocd.cfg -
#                        the shared SDK configuration file; apps reference it directly instead of making private copies. See
#                        cmake/openocd/)
#
# Creates three targets, available through `cmake --build <dir> --target <target>` or CMake Tools.
#                       Both full-image flash methods write [bootloader+header+app] in one session;
#                       obsolete partial targets flash_bootloader/flash_uart/flash_all were removed
#                       to avoid confusion. This also accommodates FU2 without assuming a running
#                       SDK bootloader can receive an app separately. flash_jtag writes bootloader
#                       and factory app through SWD in one OpenOCD session, without UART or a
#                       running bootloader. Since 2026-08-10, two separate `program` commands
#                       replace the assumption that the app immediately follows the bootloader.
#                       bootloader.bin goes at the flash base. tools/build_ota_image.py packages
#                       [header+app], written at NEON_MDF_FLASHMAP_APP_BASE when flashmap.csv
#                       exists, otherwise NEON_MDF_APP_BASE. This does not depend on CSV row order.
#                       flash_uart_stm32 writes both images through the chip ROM System Memory
#                       Bootloader, rather than the
#                       SDK bootloader, using tools/stm32_uart_flash_firmware.py. stm32flash was unreliable on the tested
#                       macOS setup because it attempted the handshake only once. The custom client
#                       retries, performs one mass erase, then writes both address regions in one
#                       UART session. Connect PB2 to GND and hold B0 manually; SWD triggers reset
#                       automatically and was more reliable than the physical Reset button in
#                       hardware testing.
#   serial_monitor: open the UART log monitor (miniterm).
#
# Apps with flashmap.csv (cmake/flashmap.cmake) can call
# neon_mdf_add_ota_flash_target(<ota_app_target> <openocd_cfg>) to create `flash_ota`, which
# simulates OTA through SWD for bootloader testing. See below and
# examples/OTA_Demo/).
#
# tools/send_ota_uart.py implements the SDK bootloader UART protocol. examples/OTA_Demo defines
# flash_ota_uart/flash_ota_uart_bad to call it. SWD/full-image packaging helpers
# build_header()/build_app_blob() remain in tools/build_full_image.py. The SDK bootloader
# (components/bootloader/neon_bootloader_main.c) is built and flashed normally. After the initial
# factory flash, send_ota_uart.py can update only the app_ota slot through that bootloader.
#
# ============================================================================
# REQUIRED POLICY for every full-image flash target, including flash_jtag/flash_uart_stm32 and future
#       transports: apps with flashmap.csv must boot the newly flashed FACTORY image. Stale otadata
#       must not select an old app or slot. Only flash_ota may select active_slot=1. There are two
#       implementations: (a) a whole-chip mass erase, such as flash_uart_stm32 using AN3155 Extended
#       Erase code 0xFFFF, automatically leaves otadata blank/invalid. No extra work is required:
#       further work: NEON_OTA_ReadData() reads an invalid magic value and returns 0 (invalid),
#       boot_flashmap() treats this as no prior OTA and selects factory by default. See
#       components/bootloader/neon_bootloader_main.c). flash_uart_stm32 already
#       already satisfies the policy. (b) A writer that erases only the image sectors, such as flash_jtag
# using `openocd program`, leaves otadata intact because the factory image ends before it. Write
# factory metadata (active_slot=0, state=VALID, boot_attempts=0, generated with
# `tools/build_otadata_image.py 0 0 0 <out>`) in the same session as the image. flash_jtag uses two
# OpenOCD `program` commands in one session. New full-image targets must explicitly implement (a) or
# (b). Writing both bootloader and app does not imply a whole-chip erase; that assumption caused
# three observed flash_jtag failures before this fix.
# ============================================================================
#
# NEON_MDF_SERIAL_PORT is a CACHE STRING used only as the configure-time default, e.g.
# -DNEON_MDF_SERIAL_PORT=.... flash_uart_stm32/serial_monitor read the current shell environment
# variable $NEON_MDF_SERIAL_PORT at runtime, falling back to the cache when unset. This lets
# vscode-extension/src/commands/flash.ts select a different port through ShellExecution env on each
# invocation without reconfiguring
# CMake.
set(NEON_MDF_SERIAL_PORT "/dev/cu.usbmodem102" CACHE STRING
  "Default ST-Link V2.1 VCP serial port. Override at runtime through NEON_MDF_SERIAL_PORT.")

function(neon_mdf_add_flash_targets bootloader_target app_target openocd_cfg)
  find_program(NEON_MDF_OPENOCD openocd REQUIRED)
  find_program(NEON_MDF_PYTHON3 python3 REQUIRED)

  # Record the exact bootloader_target/app_target names in a manifest for
  # vscode-extension/src/commands/debug.ts. Guessing an OTA executable from an _ota suffix failed
  # for examples/OTA_Demo/ota_demo_ota_bad: it ends in _bad but shares the OTA linker script and
  # slot with ota_demo_ota. The old heuristic selected flash_jtag, then debugged symbols from a
  # different image. Write factory/bootloader names here; neon_mdf_add_ota_flash_target() appends
  # ota later. SDK CMakeLists.txt files call the functions in that order.
  file(WRITE "${CMAKE_BINARY_DIR}/neon_mdf_debug_targets.txt"
    "bootloader=${bootloader_target}\nfactory=${app_target}\n")

  if(NOT TARGET serial_monitor)
    add_custom_target(serial_monitor
      COMMAND bash -c "${NEON_MDF_PYTHON3} -m serial.tools.miniterm \"\${NEON_MDF_SERIAL_PORT:-${NEON_MDF_SERIAL_PORT}}\" 115200"
      USES_TERMINAL
      VERBATIM
      COMMENT "Serial Monitor: read UART logs using NEON_MDF_SERIAL_PORT, default ${NEON_MDF_SERIAL_PORT}; Ctrl+] exits"
    )
  endif()

  # Calculate the actual bootloader and factory-app addresses once for flash_jtag/flash_uart_stm32.
  # Since 2026-08-10, separate regions replace a contiguous image that assumed the app immediately
  # followed the bootloader and required app to be the first CSV row. With flashmap.csv,
  # tools/gen_flashmap.py supplies NEON_MDF_FLASHMAP_APP_BASE from the declared partition order.
  # Otherwise use NEON_MDF_APP_BASE, derived from bootloader.size in YAML.
  math(EXPR NEON_MDF_FLASH_BASE "${NEON_MDF_APP_BASE} - ${NEON_MDF_BOOTLOADER_SIZE}" OUTPUT_FORMAT HEXADECIMAL)
  if(DEFINED NEON_MDF_FLASHMAP_APP_BASE)
    set(_neon_mdf_app_write_base ${NEON_MDF_FLASHMAP_APP_BASE})
  else()
    set(_neon_mdf_app_write_base ${NEON_MDF_APP_BASE})
  endif()

  if(NOT TARGET flash_jtag)
    # Package [16-byte header][padding][app.bin] through tools/build_ota_image.py, reusing
    # build_app_blob() rather than duplicating packaging/CRC logic. Write bootloader.bin directly
    # because it is already a valid binary. Use two OpenOCD `program` commands in one session,
    # rather than joining the regions into a contiguous image.
    if(DEFINED NEON_MDF_FLASHMAP_OTADATA_BASE)
      # With flashmap.csv, flash_jtag writes only the "bootloader" partition and
      # + "app" (factory), leaving app_ota/otadata intact. The bootloader uses otadata.active_slot, which may
      # still select an OTA image from an earlier test or another app on the same board. A verified
      # flash can therefore boot the old OTA image. Always write factory metadata (active_slot=0,
      # state=VALID, boot_attempts=0) in the same OpenOCD session. This implements policy (b) above.
      # flash_uart_stm32 implements (a): mass_erase() in stm32_uart_flash_firmware.py already clears
      # otadata.
      add_custom_target(flash_jtag
        COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/build_ota_image.py
                $<TARGET_FILE:${app_target}>.bin
                ${NEON_MDF_IMAGE_HEADER_SIZE}
                $<TARGET_FILE_DIR:${app_target}>/${app_target}_blob.bin
        COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/build_otadata_image.py
                0 0 0
                $<TARGET_FILE_DIR:${app_target}>/otadata_factory.bin
        COMMAND ${NEON_MDF_OPENOCD} -f ${openocd_cfg}
                -c "program $<TARGET_FILE:${bootloader_target}>.bin ${NEON_MDF_FLASH_BASE} verify"
                -c "program $<TARGET_FILE_DIR:${app_target}>/${app_target}_blob.bin ${_neon_mdf_app_write_base} verify"
                -c "program $<TARGET_FILE_DIR:${app_target}>/otadata_factory.bin ${NEON_MDF_FLASHMAP_OTADATA_BASE} verify reset exit"
        DEPENDS ${bootloader_target} ${app_target}
        USES_TERMINAL
        VERBATIM
        COMMENT "Flash bootloader and factory app at the flashmap.csv address, and reset otadata to factory (active_slot=0,state=VALID) in one SWD session"
      )
    else()
      add_custom_target(flash_jtag
        COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/build_ota_image.py
                $<TARGET_FILE:${app_target}>.bin
                ${NEON_MDF_IMAGE_HEADER_SIZE}
                $<TARGET_FILE_DIR:${app_target}>/${app_target}_blob.bin
        COMMAND ${NEON_MDF_OPENOCD} -f ${openocd_cfg}
                -c "program $<TARGET_FILE:${bootloader_target}>.bin ${NEON_MDF_FLASH_BASE} verify"
                -c "program $<TARGET_FILE_DIR:${app_target}>/${app_target}_blob.bin ${_neon_mdf_app_write_base} verify reset exit"
        DEPENDS ${bootloader_target} ${app_target}
        USES_TERMINAL
        VERBATIM
        COMMENT "Flash bootloader and app through SWD in one OpenOCD session with two program commands; no UART or running bootloader required"
      )
    endif()
  endif()

  if(NOT TARGET flash_uart_stm32)
    # Use the STM32 ROM System Memory Bootloader (ST AN2606/AN3155), rather than the SDK bootloader.
    # tools/stm32_uart_flash_firmware.py accepts multiple (file,address) pairs and writes them in
    # one UART session: synchronize once, mass erase once, then write each region. As with
    # flash_jtag, this avoids assuming that the app immediately follows the bootloader. The ROM
    # service is available regardless of flash contents, like the ESP32 ROM bootloader used by
    # esptool.py; it does not require a cooperating SDK bootloader.
    #
    # This target implements policy (a): Extended Erase code 0xFFFF performs a whole-chip mass erase
    # before writing (mass_erase() in stm32_uart_flash_firmware.py). otadata becomes blank/invalid
    # and the bootloader selects factory. No separate metadata write is needed, unlike flash_jtag
    # policy (b).
    #
    # Do not use `stm32flash` for this tested workflow: it sends the 0x7F synchronization byte once, then
    # reports "Failed to init device" if the bootloader is not ready at that instant. The custom
    # client retries synchronization until success and was verified repeatedly on hardware.
    #
    # Manual preparation: connect the PB2 GPIO header pin to GND and hold the board B0 button. Reset is
    # automatic through SWD by writing AIRCR.SYSRESETREQ. In repeated tests on this board, this was
    # more reliable than pressing the physical Reset button, which inconsistently entered System
    # Bootloader mode.
    #
    # STM32F412 uses both BOOT0 and BOOT1 (=PB2) to select boot mode. On this WeAct board, PB2 is shared
    # with the LED and may float by design, a known WeAct Blackpill/F4x1 board issue. Without a GND
    # connection,
    # PB2 is grounded, BOOT1 can read a different random value at each reset, entering
    # the System Bootloader may be entered inconsistently.
    #
    # After writing, release B0, disconnect PB2 from GND so the app can drive its LED, and reset to run
    # the app.
    add_custom_target(flash_uart_stm32
      COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/build_ota_image.py
              $<TARGET_FILE:${app_target}>.bin
              ${NEON_MDF_IMAGE_HEADER_SIZE}
              $<TARGET_FILE_DIR:${app_target}>/${app_target}_blob.bin
      COMMAND bash -c "echo '=== CONNECT PB2->GND + HOLD B0 ON THE BOARD (NO RESET BUTTON NEEDED - automatic SWD reset) ==='; ${NEON_MDF_OPENOCD} -f ${openocd_cfg} -c init -c \"mww 0xE000ED0C 0x05FA0004\" -c shutdown >/dev/null 2>&1; PORT=\"\${NEON_MDF_SERIAL_PORT:-${NEON_MDF_SERIAL_PORT}}\"; ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/stm32_uart_flash_firmware.py \"$PORT\" $<TARGET_FILE:${bootloader_target}>.bin ${NEON_MDF_FLASH_BASE} $<TARGET_FILE_DIR:${app_target}>/${app_target}_blob.bin ${_neon_mdf_app_write_base}"
      DEPENDS ${bootloader_target} ${app_target}
      USES_TERMINAL
      VERBATIM
      COMMENT "Flash bootloader and app through the STM32 ROM System Memory Bootloader over UART; connect PB2 to GND and hold B0 during reset"
    )
  endif()
endfunction()

# neon_mdf_add_ota_flash_target(<ota_app_target> <openocd_cfg> [flash_target])
#   ota_app_target: executable linked with the OTA partition linker script from flashmap.csv, e.g.
#                    -T${CMAKE_BINARY_DIR}/flashmap/app_ota.ld); see
#                    examples/OTA_Demo/CMakeLists.txt.
#   openocd_cfg: same as in neon_mdf_add_flash_targets(). flash_target: optional, defaults to
#                    `flash_ota`; examples can create `flash_ota_bad` using the same packaging
#                    logic.
#
# Create `flash_ota` to simulate OTA through SWD while a network transport is unavailable; see
# project_fu2_bootloader_ota_direction. Write the OTA image and
# + OTADATA (pending, unconfirmed) in one OpenOCD session to test bootloader verification, slot
# switching and rollback on hardware. Call only after
# the app has processed flashmap.csv, providing NEON_MDF_FLASHMAP_APP_OTA_BASE/OTADATA_BASE,
# see cmake/flashmap.cmake). Missing prerequisites produce an explicit FATAL_ERROR.
function(neon_mdf_add_ota_flash_target ota_app_target openocd_cfg)
  find_program(NEON_MDF_OPENOCD openocd REQUIRED)
  find_program(NEON_MDF_PYTHON3 python3 REQUIRED)

  if(NOT DEFINED NEON_MDF_FLASHMAP_APP_OTA_BASE OR NOT DEFINED NEON_MDF_FLASHMAP_OTADATA_BASE)
    message(FATAL_ERROR "neon_mdf_add_ota_flash_target: missing NEON_MDF_FLASHMAP_APP_OTA_BASE/OTADATA_BASE; call neon_mdf_process_flashmap() first (automatic through neon-mdf/CMakeLists.txt when flashmap.csv exists)")
  endif()

  set(_neon_mdf_ota_flash_target flash_ota)
  if(ARGC GREATER 2)
    set(_neon_mdf_ota_flash_target ${ARGV2})
  endif()

  # Append to the manifest created by neon_mdf_add_flash_targets(). All SDK apps
  # (Blinky/FreeRTOS_Blinky/OTA_Demo) call this function afterward.
  if(_neon_mdf_ota_flash_target STREQUAL "flash_ota")
    file(APPEND "${CMAKE_BINARY_DIR}/neon_mdf_debug_targets.txt" "ota=${ota_app_target}\n")
  endif()

  if(NOT TARGET ${_neon_mdf_ota_flash_target})
    add_custom_target(${_neon_mdf_ota_flash_target}
      COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/build_ota_image.py
              $<TARGET_FILE:${ota_app_target}>.bin
              ${NEON_MDF_IMAGE_HEADER_SIZE}
              $<TARGET_FILE_DIR:${ota_app_target}>/${ota_app_target}_blob.bin
      COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/build_otadata_image.py
              1 1 0
              $<TARGET_FILE_DIR:${ota_app_target}>/otadata_pending.bin
      COMMAND ${NEON_MDF_OPENOCD} -f ${openocd_cfg}
              -c "program $<TARGET_FILE_DIR:${ota_app_target}>/${ota_app_target}_blob.bin ${NEON_MDF_FLASHMAP_APP_OTA_BASE} verify"
              -c "program $<TARGET_FILE_DIR:${ota_app_target}>/otadata_pending.bin ${NEON_MDF_FLASHMAP_OTADATA_BASE} verify reset exit"
      DEPENDS ${ota_app_target}
      USES_TERMINAL
      VERBATIM
      COMMENT "Simulate OTA through SWD (${_neon_mdf_ota_flash_target}): write [header+${ota_app_target}] to the OTA slot and mark otadata pending"
    )
  endif()
endfunction()
