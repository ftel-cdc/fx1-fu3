# flashmap.cmake processes <APP_DIR>/flashmap.csv, the app-specific partition table, like ESP-IDF
# partitions.csv but not device's) over tools/gen_flashmap.py - same PATTERN as cmake/kconfig.cmake
# (neon_mdf_process_kconfig()): Python creates fragments, CMake only include().
#
# neon_mdf_process_flashmap(<APP_DIR> [<CSV_PATH>])
#   call from neon-mdf/CMakeLists.txt (automatic, like kconfig's osource for the app's own Kconfig - see
#   cmake/kconfig.cmake) - not the caller app. neon-mdf/CMakeLists.txt checks EXISTS <CSV_PATH>
#   itself before calling this function - the app only needs to have a flashmap file so that each
#   message is wired automatically, and does not need to modify the app's CMakeLists.txt to "enable"
#   the flashmap (originally sufficiently specified app from calling this function, but in return
#   it's ridiculous: this function add_subdirectory(components/ota) - needs to be run before
#   add_subdirectory(components/bootloader) so that the components/bootloader/CMakeLists.txt link is
#   created by target neon_mdf_ota each - confirmed by separate test ranging order
#   target_link_libraries() is irrelevant with the current long CMake, but call from
#   neon-mdf/CMakeLists.txt remains the most magnetic value, assigned to the same bootloader.cmake
#   fragment being included there).
#
#   CSV_PATH (optional, defaults to <APP_DIR>/flashmap.csv if internal): fits PROJECT as original
#   (ESP-IDF partitions.csv-style), but the 1 app can be built for many other devices (e.g. Blinky:
#   FU2/WEACT_F412/ CH32V003F4P6) with huge difference in flash size (16KB vs 512KB) - the only 1
#   flashmap.csv cannot fit all. neon-mdf/CMakeLists.txt from flashmap
#   priority.<NEON_MDF_TARGET>.csv (override by device) if available, fallback to flashmap.csv
#   (shared/default) if not - see that logic here, this function only accepts selected CSV_PATH, not
#   specified ones.
#
# after calling (only if flashmap.csv is real):
#   - CMake variables NEON_MDF_FLASHMAP_<PARTITION_NAME>_BASE/SIZE, plus _VECTOR_BASE/
#     _CODE_SIZE for partition type=app) in the scope calls this function.
#   - NEON_MDF_FLASHMAP_VAR_NAMES variable (list name of above variables) - used by
#     neon-mdf/CMakeLists.txt to propagate next PARENT_SCOPE second tier mail to the app (partition
#     name list is dynamic, no hardcode).
#   - Target INTERFACE `neon_mdf_flashmap` - include dir not flashmap.h (macro C with the same name as
#     CMake variable above, used by components/ota/ +
#     neon_bootloader_main.c).
#   - Target `neon_mdf_ota` (components/ota/, see below).
#   - 1 .ld file each for each partition type=app, create
#     ${CMAKE_BINARY_DIR}/flashmap/<ten_partition>.ld - app CMakeLists.txt
#     from link -T use the same file for each executable (e.g. app.ld for factory version, app_ota.ld for
#     ota version - see examples/OTA_Demo/).
#
# requirement: app must have bootloader-enabled target (NEON_MDF_BOOTLOADER_ENABLED,
# from NEON_MDF_APP_BASE/IMAGE_HEADER_SIZE/RAM_BASE/RAM_SIZE/ERASE_SECTORS already
# created by mcu-description - see generate.py) - FATAL_ERROR if calling this function without those
# variables instead of obscure Python-script errors.
function(neon_mdf_process_flashmap APP_DIR)
  find_program(NEON_MDF_PYTHON3 python3 REQUIRED)

  if(NOT NEON_MDF_BOOTLOADER_ENABLED)
    message(FATAL_ERROR "neon_mdf_process_flashmap: target ${NEON_MDF_TARGET} does not enable NEON_MDF_BOOTLOADER_ENABLED (missing bootloader: in the device YAML). flashmap.csv requires a bootloader and its APP_BASE/IMAGE_HEADER_SIZE layout.")
  endif()

  # NEON_MDF_FLASH_BASE uses the same calculation as flash_targets.cmake: APP_BASE - BOOTLOADER_SIZE
  # see because add 1 variable each - see corresponding comment in flash_targets.cmake target
  # flash_jtag.
  math(EXPR _flashmap_flash_base "${NEON_MDF_APP_BASE} - ${NEON_MDF_BOOTLOADER_SIZE}" OUTPUT_FORMAT HEXADECIMAL)

  if(ARGC GREATER 1)
    set(_flashmap_csv ${ARGV1})
  else()
    set(_flashmap_csv ${APP_DIR}/flashmap.csv)
  endif()
  set(_flashmap_dir ${CMAKE_BINARY_DIR}/flashmap)
  set(_header_out ${_flashmap_dir}/flashmap.h)
  set(_cmake_out ${_flashmap_dir}/flashmap.cmake)
  set(_linker_dir ${_flashmap_dir})

  execute_process(
    COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/gen_flashmap.py
            --flashmap ${_flashmap_csv}
            --flash-base ${_flashmap_flash_base}
            --flash-size ${NEON_MDF_FLASH_SIZE}
            --app-base ${NEON_MDF_APP_BASE}
            --image-header-size ${NEON_MDF_IMAGE_HEADER_SIZE}
            --erase-sectors "${NEON_MDF_ERASE_SECTORS}"
            --header-out ${_header_out}
            --cmake-out ${_cmake_out}
            --linker-dir ${_linker_dir}
            --ram-base ${NEON_MDF_RAM_BASE}
            --ram-size ${NEON_MDF_RAM_SIZE}
            --device-name ${NEON_MDF_TARGET}
            --arch ${NEON_MDF_TARGET_ARCH}
    RESULT_VARIABLE _gen_result
    OUTPUT_VARIABLE _gen_output
    ERROR_VARIABLE _gen_error
  )
  message(STATUS "[flashmap] ${_gen_output}")
  if(NOT _gen_result EQUAL 0)
    message(FATAL_ERROR "failed flashmap generation:\n${_gen_output}${_gen_error}")
  endif()

  include(${_cmake_out})

  if(NOT TARGET neon_mdf_flashmap)
    add_library(neon_mdf_flashmap INTERFACE)
    target_include_directories(neon_mdf_flashmap INTERFACE ${_flashmap_dir})
  endif()

  # components/ota/ (neon_ota.h - read/write otadata, shared by bootloader times app) just when flashmap
  # turned on - add_subdirectory() right here to app CMakeLists.txt just call 1 this function is
  # enough flashmap.h times target neon_mdf_ota, no need from include/add_subdirectory separately.
  # neon_mdf_crc32
  # (components/bootloader/CMakeLists.txt) already exists because flashmap
  # already FATAL_ERROR above if NEON_MDF_BOOTLOADER_ENABLED is not enabled.
  if(NOT TARGET neon_mdf_ota)
    add_subdirectory(${NEON_MDF_PATH}/components/ota ${CMAKE_BINARY_DIR}/neon_mdf_ota_build)
  endif()

  # Propagate each NEON_MDF_FLASHMAP_* variable to PARENT_SCOPE - this function is called directly from
  # the app's CMakeLists.txt (not through add_subdirectory, other than kconfig.cmake) so only 1 tier
  # of PARENT_SCOPE is needed, not 2.
  execute_process(
    COMMAND ${NEON_MDF_PYTHON3} -c "
import re
names = []
with open('${_cmake_out}') as f:
    for line in f:
        m = re.match(r'set\\((NEON_MDF_FLASHMAP_[A-Za-z0-9_]+)', line)
        if m:
            names.append(m.group(1))
print(';'.join(names))
"
    OUTPUT_VARIABLE _flashmap_names
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  foreach(_name ${_flashmap_names})
    set(${_name} ${${_name}} PARENT_SCOPE)
  endforeach()
  set(NEON_MDF_FLASHMAP_VAR_NAMES ${_flashmap_names} PARENT_SCOPE)
endfunction()
