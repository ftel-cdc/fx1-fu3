cmake_minimum_required(VERSION 3.21)
project(ota_demo C ASM)

# Fallback khi bien moi truong NEON_MDF_PATH khong toi duoc process cmake
# (vd: VS Code CMake Tools khong luon truyen cmake.environment vao spawn) -
# tu suy ra vi tri neon-mdf/ tu vi tri tuong doi cua app nay.
if(NOT DEFINED ENV{NEON_MDF_PATH})
  set(ENV{NEON_MDF_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/../../neon-mdf")
endif()
include($ENV{NEON_MDF_PATH}/tools/cmake/project.cmake)

if(NOT NEON_MDF_BOOTLOADER_ENABLED)
  message(FATAL_ERROR "OTA_Demo: can target co bootloader: trong device YAML. Chon WEACT_F412 hoac STM32F103CB.")
endif()

if(NOT NEON_MDF_TARGET STREQUAL "WEACT_F412" AND NOT NEON_MDF_TARGET STREQUAL "STM32F103CB")
  message(FATAL_ERROR "OTA_Demo chi ho tro WEACT_F412 va STM32F103CB (target hien tai: ${NEON_MDF_TARGET})")
endif()

neon_mdf_add_bootloader_executable(bootloader)

# flashmap.csv duoc xu ly TU DONG boi neon-mdf/CMakeLists.txt (xem
# cmake/flashmap.cmake) TRUOC khi toi day - NEON_MDF_FLASHMAP_APP_BASE/
# APP_OTA_BASE/OTADATA_BASE... va target neon_mdf_ota da co san, khong can
# goi gi them o day.

# 3 executable, CUNG 1 Src/main.c, khac linker script (sinh boi
# tools/gen_flashmap.py, xem ${CMAKE_BINARY_DIR}/flashmap/<partition>.ld) +
# macro NEON_OTA_APP_VERSION (+ NEON_OTA_SKIP_CONFIRM cho ban test rollback) -
# xem Src/main.c cho y nghia tung ban.
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
# (factory) - CHINH XAC nhu app khong co flashmap, KHONG can sua gi o
# flash_targets.cmake ca (NEON_MDF_FLASHMAP_APP_BASE == NEON_MDF_APP_BASE,
# ota_demo bi rang buoc dung dia chi nay boi chinh linker script cua no).
include(${NEON_MDF_PATH}/cmake/flash_targets.cmake)
neon_mdf_add_flash_targets(bootloader ota_demo ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)

# flash_ota ghi ban v2 tu confirm; flash_ota_bad ghi ban v3 co y khong
# confirm de test rollback. Ca hai chi ghi app_ota + otadata pending qua SWD.
neon_mdf_add_ota_flash_target(ota_demo_ota ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
neon_mdf_add_ota_flash_target(ota_demo_ota_bad ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg flash_ota_bad)

# OTA qua giao thuc UART 0x55 cua second-stage bootloader. Client tu lay cong
# tu bien moi truong NEON_MDF_SERIAL_PORT, fallback /dev/ttyACM0. Chay target
# truoc roi reset board khi client bat dau cho READY.
add_custom_target(flash_ota_uart
  COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/send_ota_uart.py
          --version 2 $<TARGET_FILE:ota_demo_ota>.bin
  DEPENDS ota_demo_ota
  USES_TERMINAL
  VERBATIM
  COMMENT "OTA UART 0x55: gui ota_demo_ota v2 (mac dinh /dev/ttyACM0); reset board khi duoc yeu cau"
)

add_custom_target(flash_ota_uart_bad
  COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/send_ota_uart.py
          --version 3 $<TARGET_FILE:ota_demo_ota_bad>.bin
  DEPENDS ota_demo_ota_bad
  USES_TERMINAL
  VERBATIM
  COMMENT "OTA UART 0x55: gui ota_demo_ota_bad v3 de test rollback"
)
