cmake_minimum_required(VERSION 3.21)
project(ota_demo C ASM)

# Fallback when variables for each field NEON_MDF_PATH cannot be processed cmake (e.g. VS Code CMake
# Tools does not recognize cmake.environment and spawn) - from deriving the value neon-mdf/ from the
# corresponding value of this app.
if(NOT DEFINED ENV{NEON_MDF_PATH})
  set(ENV{NEON_MDF_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../neon-mdf")
endif()
include($ENV{NEON_MDF_PATH}/tools/cmake/project.cmake)

if(NOT NEON_MDF_BOOTLOADER_ENABLED)
  message(FATAL_ERROR "OTA_Demo: need target with bootloader: in YAML device. select WEACT_F412 or STM32F103CB.")
endif()

if(NOT NEON_MDF_TARGET STREQUAL "WEACT_F412" AND NOT NEON_MDF_TARGET STREQUAL "STM32F103CB")
  message(FATAL_ERROR "OTA_Demo only supports WEACT_F412 and STM32F103CB (currenttarget: ${NEON_MDF_TARGET})")
endif()

neon_mdf_add_bootloader_executable(bootloader)

# flashmap.csv is processed automatically by neon-mdf/CMakeLists.txt (see cmake/flashmap.cmake) before
# this point: NEON_MDF_FLASHMAP_APP_BASE/APP_OTA_BASE/OTADATA_BASE and the neon_mdf_ota target are
# already available; no extra call is required.

# 3 executable, same as 1 Src/main.c, different linker script (created by tools/gen_flashmap.py, see
# ${CMAKE_BINARY_DIR}/flashmap/<partition>.ld) + macro NEON_OTA_APP_VERSION (+ NEON_OTA_SKIP_CONFIRM
# for test rollback) - see Src/main.c for meanings of each.
function(_ota_demo_add_variant name linker_script version)
  add_executable(${name} Src/main.c)
  target_link_libraries(${name} PRIVATE neon_mdf_driver neon_mdf_bootloader_uart neon_mdf_ota)
  target_compile_definitions(${name} PRIVATE NEON_OTA_APP_VERSION=${version})
  if(ARGC GREATER 3 AND ARGV3)
    target_compile_definitions(${name} PRIVATE NEON_OTA_SKIP_CONFIRM)
  endif()
  target_link_options(${name} PRIVATE -T${linker_script} -Wl,-Map=$<TARGET_FILE:${name}>.map)
  set_target_properties(${name} PROPERTIES LINK_DEPENDS ${linker_script})
  add_custom_command(TARGET ${name} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${name}> $<TARGET_FILE:${name}>.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${name}>
    COMMENT "${name}: objcopy -> .bin + size report"
  )
endfunction()

set(_ota_demo_ld_dir ${CMAKE_BINARY_DIR}/flashmap)
_ota_demo_add_variant(ota_demo          ${_ota_demo_ld_dir}/app.ld     1)
_ota_demo_add_variant(ota_demo_ota      ${_ota_demo_ld_dir}/app_ota.ld 2)
_ota_demo_add_variant(ota_demo_ota_bad  ${_ota_demo_ld_dir}/app_ota.ld 3 TRUE)

# flash_jtag/flash_uart_stm32/serial_monitor - app_target = ota_demo
# (factory) - exactly like the app without flashmap, no need to fix anything at
# flash_targets.cmake either (NEON_MDF_FLASHMAP_APP_BASE == NEON_MDF_APP_BASE,
# ota_demo bi rang step uses this dia indicated by the linker script of no).
include(${NEON_MDF_PATH}/cmake/flash_targets.cmake)
neon_mdf_add_flash_targets(bootloader ota_demo ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)

# flash_ota write version v2 from confirm; flash_ota_bad write version v3 does not mean confirm to
# test rollback. Both only write app_ota + otadata pending via SWD.
neon_mdf_add_ota_flash_target(ota_demo_ota ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
neon_mdf_add_ota_flash_target(ota_demo_ota_bad ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg flash_ota_bad)

# OTA via UART 0x55 interface of second-stage bootloader. Client from taking ports from each field
# variable NEON_MDF_SERIAL_PORT, fallback /dev/ttyACM0. run target first then reset board when
# client starts for READY.
add_custom_target(flash_ota_uart
  COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/send_ota_uart.py
          --version 2 $<TARGET_FILE:ota_demo_ota>.bin
  DEPENDS ota_demo_ota
  USES_TERMINAL
  VERBATIM
  COMMENT "OTA UART 0x55: send ota_demo_ota v2 (default /dev/ttyACM0); reset the board when prompted"
)

add_custom_target(flash_ota_uart_bad
  COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/send_ota_uart.py
          --version 3 $<TARGET_FILE:ota_demo_ota_bad>.bin
  DEPENDS ota_demo_ota_bad
  USES_TERMINAL
  VERBATIM
  COMMENT "OTA UART 0x55: send ota_demo_ota_bad v3 to test rollback"
)
