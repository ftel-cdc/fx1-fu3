# tools/cmake/project.cmake
#
# File DUY NHẤT mà CMakeLists.txt của 1 app cần include - mô phỏng
# $IDF_PATH/tools/cmake/project.cmake của ESP-IDF. Dùng:
#
#   cmake_minimum_required(VERSION 3.21)
#   project(my_app C ASM)
#   include($ENV{NEON_MDF_PATH}/tools/cmake/project.cmake)
#
#   neon_mdf_add_executable(my_app Src/main.c)
#
# LƯU Ý khác ESP-IDF thật: ESP-IDF cho include() TRƯỚC project() rồi tự
# "shadow" lệnh project() bằng macro riêng để chèn logic vào giữa. Ở đây
# project() do app gọi trước, include() sau - đơn giản hơn, tránh CMake
# Warning (dev) "cmake_minimum_required() should be called prior to this
# top-level project() call" (đã tự verify: gọi project() bên trong macro qua
# include() gây ra warning này, gọi trực tiếp thì hết). Đổi lại: mất đúng thứ
# tự y hệt ESP-IDF, nhưng số dòng boilerplate cho app gần như không đổi. Xem
# ARCHITECTURE.md mục 5.

# --- NEON_MDF_PATH: giống $IDF_PATH của ESP-IDF ---
# Ưu tiên: đã set tay (-DNEON_MDF_PATH=...) > biến môi trường (source export.sh)
# > tự suy ra từ vị trí file này (phòng khi quên set, vẫn chạy được nếu
# neon-mdf/ nằm ngay cạnh app - không khuyến khích, chỉ để đỡ lỗi vặt).
if(NOT NEON_MDF_PATH)
  if(DEFINED ENV{NEON_MDF_PATH})
    set(NEON_MDF_PATH $ENV{NEON_MDF_PATH})
  else()
    get_filename_component(NEON_MDF_PATH "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
    message(WARNING "NEON_MDF_PATH chua duoc set (chay `. neon-mdf/export.sh` truoc) - tam suy ra: ${NEON_MDF_PATH}")
  endif()
endif()

# --- NEON_MDF_TARGET: giống IDF_TARGET của ESP-IDF (esp32, esp32c3...) ---
# Cache lại để những lần configure sau không cần truyền lại -D, giống hệt
# cách `idf.py set-target <chip>` hoạt động (ghi 1 lần, dùng lại mãi cho tới
# khi đổi target khác + xoá build/).
if(NOT DEFINED NEON_MDF_TARGET)
  if(DEFINED ENV{NEON_MDF_TARGET})
    set(NEON_MDF_TARGET $ENV{NEON_MDF_TARGET})
  else()
    set(NEON_MDF_TARGET FU2)
  endif()
endif()
set(NEON_MDF_TARGET ${NEON_MDF_TARGET} CACHE STRING "neon-mdf target (chip variant) - xem cmake/mcu_variants.cmake")

include(${NEON_MDF_PATH}/cmake/mcu_variants.cmake)
if(NOT NEON_MDF_TARGET IN_LIST NEON_MDF_KNOWN_TARGETS)
  message(FATAL_ERROR "NEON_MDF_TARGET='${NEON_MDF_TARGET}' chưa đăng ký trong cmake/mcu_variants.cmake (target hợp lệ: ${NEON_MDF_KNOWN_TARGETS})")
endif()

add_subdirectory(${NEON_MDF_PATH} neon-mdf-build)
include(${NEON_MDF_PATH}/cmake/fu2_image.cmake)

# neon_mdf_add_executable(<name> <nguồn.c...>) - gói sẵn link neon_mdf_driver +
# linker script + objcopy/size, giống mức tiện lợi của idf_component_register()
# nhưng đơn giản hơn nhiều (neon-mdf hiện chỉ có 1 "component" gộp là
# neon_mdf_driver, không có hệ thống component/dependency graph nhiều tầng như
# ESP-IDF - xem ARCHITECTURE.md mục 6 lý do chưa làm tới mức đó).
function(neon_mdf_add_executable name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE neon_mdf_driver)
  target_link_options(${name} PRIVATE -T${NEON_MDF_LINKER_SCRIPT} -Wl,-Map=$<TARGET_FILE:${name}>.map)
  set_target_properties(${name} PROPERTIES LINK_DEPENDS ${NEON_MDF_LINKER_SCRIPT})
  if(NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "FU2")
    # FU2 enters through the ROM, with its own reset handler and word-only SRAM.
    target_link_options(${name} PRIVATE -nostdlib)
    # Select the SDK's word-only memory/CLZ runtime before scanning libgcc.
    # Otherwise a 64-bit clock calculation can pull in libgcc's byte-table
    # __clzsi2 before the device archive is visited. Force the SDK member in
    # even when the app does not call memcpy/strlen itself.
    target_link_options(${name} PRIVATE "LINKER:-u,__clzsi2")
    target_link_libraries(${name} PRIVATE ${NEON_MDF_DEVICE_TARGET} gcc)
    neon_mdf_add_fu2_image(${name})
  else()
    add_custom_command(TARGET ${name} POST_BUILD
      COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${name}> $<TARGET_FILE:${name}>.bin
      COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${name}>
      COMMENT "${name}: objcopy -> .bin + size report"
    )
  endif()
endfunction()

# neon_mdf_add_app_executable(<name> <nguồn.c...>) - giống neon_mdf_add_executable()
# nhưng link -T${NEON_MDF_APP_LINKER_SCRIPT} (partition SAU bootloader, xem
# mcu-description/schema/common.schema.yaml `bootloader:`) thay vì toàn bộ
# flash - dùng cho app CHẠY DƯỚI 1 bootloader, nạp TRỌN BỘ [bootloader+header+app]
# qua target CMake flash_jtag (SWD) hoặc flash_uart_stm32 (UART qua ROM System
# Bootloader thật - xem cmake/flash_targets.cmake). Target phải có NEON_MDF_TARGET với
# `bootloader:` trong YAML (NEON_MDF_BOOTLOADER_ENABLED=ON) - FATAL_ERROR rõ
# ràng nếu không, thay vì âm thầm link nhầm linker script.
function(neon_mdf_add_app_executable name)
  if(NOT NEON_MDF_BOOTLOADER_ENABLED)
    message(FATAL_ERROR "neon_mdf_add_app_executable('${name}'): target '${NEON_MDF_TARGET}' chưa khai `bootloader:` trong device YAML (xem mcu-description/devices/${NEON_MDF_TARGET}.yaml) - dùng neon_mdf_add_executable() nếu app này build nguyên flash, không qua bootloader")
  endif()
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE neon_mdf_driver)
  target_link_options(${name} PRIVATE -T${NEON_MDF_APP_LINKER_SCRIPT} -Wl,-Map=$<TARGET_FILE:${name}>.map)
  set_target_properties(${name} PROPERTIES LINK_DEPENDS ${NEON_MDF_APP_LINKER_SCRIPT})
  add_custom_command(TARGET ${name} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${name}> $<TARGET_FILE:${name}>.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${name}>
    COMMENT "${name}: objcopy -> .bin + size report"
  )
endfunction()

# neon_mdf_add_bootloader_executable(<name>) - KHÔNG nhận file nguồn cho app -
# SDK tự cung cấp main.c+arch file qua NEON_MDF_BOOTLOADER_MAIN_SOURCES (xem
# components/bootloader/CMakeLists.txt - compile TRUC TIEP nhu nguon cua
# CHINH executable nay, khong con qua 1 thu vien trung gian nua kem file
# stub rong - xem comment day du o do). Link -T${NEON_MDF_BOOTLOADER_LINKER_SCRIPT}.
function(neon_mdf_add_bootloader_executable name)
  if(NOT NEON_MDF_BOOTLOADER_ENABLED)
    message(FATAL_ERROR "neon_mdf_add_bootloader_executable('${name}'): target '${NEON_MDF_TARGET}' chưa khai `bootloader:` trong device YAML (xem mcu-description/devices/${NEON_MDF_TARGET}.yaml)")
  endif()
  add_executable(${name} ${NEON_MDF_BOOTLOADER_MAIN_SOURCES})
  target_link_libraries(${name} PRIVATE neon_mdf_bootloader_uart neon_mdf_driver neon_mdf_crc32)
  if(NEON_MDF_APP_HAS_FLASHMAP)
    target_compile_definitions(${name} PRIVATE NEON_MDF_FLASHMAP_ENABLED)
    target_link_libraries(${name} PRIVATE neon_mdf_ota)
  endif()
  target_link_options(${name} PRIVATE -T${NEON_MDF_BOOTLOADER_LINKER_SCRIPT} -Wl,-Map=$<TARGET_FILE:${name}>.map)
  set_target_properties(${name} PROPERTIES LINK_DEPENDS ${NEON_MDF_BOOTLOADER_LINKER_SCRIPT})
  add_custom_command(TARGET ${name} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${name}> $<TARGET_FILE:${name}>.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${name}>
    COMMENT "${name}: objcopy -> .bin + size report"
  )
endfunction()
