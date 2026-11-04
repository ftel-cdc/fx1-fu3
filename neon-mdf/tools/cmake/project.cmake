# tools/cmake/project.cmake
#
# The only file that an app's CMakeLists.txt needs to include - simulates ESP-IDF's
# $IDF_PATH/tools/cmake/project.cmake. Use:
#
#   cmake_minimum_required(VERSION 3.21)
#   project(my_app C ASM)
#   include($ENV{NEON_MDF_PATH}/tools/cmake/project.cmake)
#
#   neon_mdf_add_executable(my_app Src/main.c)
#
# NOTE other than real ESP-IDF: ESP-IDF for include() BEFORE project() then manually "shadow" command
# project() with its own macro to insert logic in the middle. Here project() that app calls first,
# include() later - simpler, avoid CMake
# Warning (dev) "cmake_minimum_required() should be called prior to this
# top-level project() call" (already verified: calling project() inside macro via include() causes
# this warning, direct call is out). In turn: lost the exact order ESP-IDF, but the number of lines
# boilerplate for the app is almost constant. See ARCHITECTURE.md section 5.

# --- NEON_MDF_PATH: same as $IDF_PATH of ESP-IDF --- Priority: hand set (-DNEON_MDF_PATH=...) >
# environment variable (source export.sh)
# > deduced by itself from this file location (in case of forgetting to set, it will still work if
# neon-mdf/ is next to the app - not recommended, just for minor errors).
if(NOT NEON_MDF_PATH)
  if(DEFINED ENV{NEON_MDF_PATH})
    set(NEON_MDF_PATH $ENV{NEON_MDF_PATH})
  else()
    get_filename_component(NEON_MDF_PATH "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
    message(WARNING "NEON_MDF_PATH not set (running `. neon-mdf/export.sh` first) - extrapolation triad: ${NEON_MDF_PATH}")
  endif()
endif()

# --- NEON_MDF_TARGET: same as ESP-IDF's IDF_TARGET (esp32, esp32c3...) --- Cache again so that later
# configurations do not need to pass -D, just like how `idf.py set-target <chip>` works (write 1
# time, use it again until you change the target + erase build/).
if(NOT DEFINED NEON_MDF_TARGET)
  if(DEFINED ENV{NEON_MDF_TARGET})
    set(NEON_MDF_TARGET $ENV{NEON_MDF_TARGET})
  else()
    set(NEON_MDF_TARGET FU2)
  endif()
endif()
set(NEON_MDF_TARGET ${NEON_MDF_TARGET} CACHE STRING "neon-mdf target (chip variant) - see cmake/mcu_variants.cmake")

include(${NEON_MDF_PATH}/cmake/mcu_variants.cmake)
if(NOT NEON_MDF_TARGET IN_LIST NEON_MDF_KNOWN_TARGETS)
  message(FATAL_ERROR "NEON_MDF_TARGET='${NEON_MDF_TARGET}' not registered in cmake/mcu_variants.cmake (valid target: ${NEON_MDF_KNOWN_TARGETS})")
endif()

add_subdirectory(${NEON_MDF_PATH} neon-mdf-build)
include(${NEON_MDF_PATH}/cmake/fu2_image.cmake)

# neon_mdf_add_executable(<name> <source.c...>) packages linking with neon_mdf_driver, the linker script and
# objcopy/size, like the convenience of idf_component_register() but much simpler (neon-mdf
# currently has only one composite "component" neon_mdf_driver, there is no multilayered
# component/dependency graph system such as ESP-IDF - see ARCHITECTURE.md section 6 that hasn't done
# that yet).
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

# neon_mdf_add_app_executable(<name> <source.c...>) works like neon_mdf_add_executable() but links
# -T${NEON_MDF_APP_LINKER_SCRIPT} (partition after bootloader, see
# mcu-description/schema/common.schema.yaml `bootloader:`) seen because full flash - used for app
# RUN UNDER 1 bootloader, FULL flash [bootloader+header+app] via target CMake flash_jtag (SWD) or
# flash_uart_stm32 (UART via real System Bootloader ROM - see cmake/flash_targets.cmake). Target
# must have NEON_MDF_TARGET with `bootloader:` in YAML (NEON_MDF_BOOTLOADER_ENABLED=ON) -
# FATAL_ERROR clear otherwise, seen because silently mistaken linker script.
function(neon_mdf_add_app_executable name)
  if(NOT NEON_MDF_BOOTLOADER_ENABLED)
    message(FATAL_ERROR "neon_mdf_add_app_executable('${name}'): target '${NEON_MDF_TARGET}' not declared `bootloader:` in YAML device (see mcu-description/devices/${NEON_MDF_TARGET}.yaml) - use neon_mdf_add_executable() if this app builds native flash, not via bootloader")
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

# neon_mdf_add_bootloader_executable(<name>) - DO NOT receive source files for app - SDK self-leveling
# main.c+arch files via NEON_MDF_BOOTLOADER_MAIN_SOURCES (see components/bootloader/CMakeLists.txt -
# compile directly as the source of this executable itself, not through an intermediate old message
# an empty stub; see the full comment there). Link with -T${NEON_MDF_BOOTLOADER_LINKER_SCRIPT}.
function(neon_mdf_add_bootloader_executable name)
  if(NOT NEON_MDF_BOOTLOADER_ENABLED)
    message(FATAL_ERROR "neon_mdf_add_bootloader_executable('${name}'): target '${NEON_MDF_TARGET}' not declared `bootloader:` in YAML device (see mcu-description/devices/${NEON_MDF_TARGET}.yaml)")
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
