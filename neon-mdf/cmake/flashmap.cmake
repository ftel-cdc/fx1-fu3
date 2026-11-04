# flashmap.cmake - xu ly <APP_DIR>/flashmap.csv (partition table CUA RIENG
# project, giong het partitions.csv that cua ESP-IDF - KHONG phai cua device)
# qua tools/gen_flashmap.py - CUNG PATTERN voi cmake/kconfig.cmake
# (neon_mdf_process_kconfig()): Python sinh fragment, CMake chi include().
#
# neon_mdf_process_flashmap(<APP_DIR> [<CSV_PATH>])
#   Goi TU neon-mdf/CMakeLists.txt (TU DONG, giong osource cua kconfig cho
#   Kconfig rieng cua app - xem cmake/kconfig.cmake) - KHONG phai app tu goi.
#   neon-mdf/CMakeLists.txt tu kiem tra EXISTS <CSV_PATH> TRUOC khi goi ham
#   nay - app CHI CAN CO file flashmap la moi thu tu dong duoc wire, khong
#   can sua CMakeLists.txt cua app de "bat" flashmap (ban dau du dinh app tu
#   goi ham nay, nhung doi lai vi ly do ky thuat: ham nay add_subdirectory(components/ota) -
#   can chay TRUOC add_subdirectory(components/bootloader) de
#   components/bootloader/CMakeLists.txt link duoc target neon_mdf_ota moi
#   tao - da xac nhan bang test rieng rang thu tu target_link_libraries()
#   KHONG quan trong voi CMake hien dai, nhung goi tu neon-mdf/CMakeLists.txt
#   van la vi tri tu nhien nhat, gan voi bootloader.cmake fragment cung dang
#   duoc include o do).
#
#   CSV_PATH (tuy chon, mac dinh <APP_DIR>/flashmap.csv neu bo trong): du
#   THEO PROJECT nhu thiet ke ban dau (ESP-IDF partitions.csv-style), nhung
#   1 app co the build cho NHIEU device KHAC NHAU (vd Blinky: FU2/WEACT_F412/
#   CH32V003F4P6) voi flash size CHENH LECH RAT LON (16KB vs 512KB) - 1
#   flashmap.csv DUY NHAT khong the vua tat ca. neon-mdf/CMakeLists.txt tu
#   uu tien flashmap.<NEON_MDF_TARGET>.csv (override THEO DEVICE) NEU co,
#   fallback ve flashmap.csv (dung chung/mac dinh) neu khong - xem logic do
#   o day, ham nay CHI nhan CSV_PATH da duoc chon san, khong tu quyet dinh.
#
# Sau khi goi (chi khi flashmap.csv co that):
#   - Bien CMake NEON_MDF_FLASHMAP_<TEN_PARTITION>_BASE/SIZE (+ _VECTOR_BASE/
#     _CODE_SIZE cho partition type=app) trong scope goi ham nay.
#   - Bien NEON_MDF_FLASHMAP_VAR_NAMES (list ten cac bien tren) - dung boi
#     neon-mdf/CMakeLists.txt de propagate tiep PARENT_SCOPE tang thu 2 len
#     app (danh sach ten partition la DONG, khong co san de hardcode).
#   - Target INTERFACE `neon_mdf_flashmap` - include dir chua flashmap.h
#     (macro C cung ten voi bien CMake o tren, dung boi components/ota/ +
#     neon_bootloader_main.c).
#   - Target `neon_mdf_ota` (components/ota/, xem o duoi).
#   - 1 file .ld MOI cho MOI partition type=app, sinh vao
#     ${CMAKE_BINARY_DIR}/flashmap/<ten_partition>.ld - app CMakeLists.txt
#     tu link -T dung file tuong ung cho tung executable (vd app.ld cho ban
#     factory, app_ota.ld cho ban ota - xem examples/OTA_Demo/).
#
# YEU CAU: app phai co bootloader-enabled target (NEON_MDF_BOOTLOADER_ENABLED,
# tu NEON_MDF_APP_BASE/IMAGE_HEADER_SIZE/RAM_BASE/RAM_SIZE/ERASE_SECTORS da
# duoc mcu-description sinh - xem generate.py) - FATAL_ERROR neu goi ham nay
# ma thieu cac bien do, thay vi loi kho hieu tu Python script.
function(neon_mdf_process_flashmap APP_DIR)
  find_program(NEON_MDF_PYTHON3 python3 REQUIRED)

  if(NOT NEON_MDF_BOOTLOADER_ENABLED)
    message(FATAL_ERROR "neon_mdf_process_flashmap: target '${NEON_MDF_TARGET}' chua bat NEON_MDF_BOOTLOADER_ENABLED (thieu `bootloader:` trong device YAML) - flashmap.csv can bootloader lam nen tang (APP_BASE/IMAGE_HEADER_SIZE)")
  endif()

  # NEON_MDF_FLASH_BASE - tinh GIONG HET cach flash_targets.cmake da lam
  # (APP_BASE - BOOTLOADER_SIZE) thay vi them 1 bien moi - xem comment
  # tuong tu trong flash_targets.cmake target flash_jtag.
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
    message(FATAL_ERROR "Sinh flashmap that bai:\n${_gen_output}${_gen_error}")
  endif()

  include(${_cmake_out})

  if(NOT TARGET neon_mdf_flashmap)
    add_library(neon_mdf_flashmap INTERFACE)
    target_include_directories(neon_mdf_flashmap INTERFACE ${_flashmap_dir})
  endif()

  # components/ota/ (neon_ota.h - doc/ghi otadata, dung chung boi bootloader
  # LAN app) CHI can khi flashmap bat - add_subdirectory() ngay tai day de
  # app CMakeLists.txt chi can goi 1 ham nay la co du flashmap.h LAN target
  # neon_mdf_ota, khong can tu include/add_subdirectory rieng. neon_mdf_crc32
  # (components/bootloader/CMakeLists.txt) da chac chan ton tai vi flashmap
  # da FATAL_ERROR o tren neu NEON_MDF_BOOTLOADER_ENABLED chua bat.
  if(NOT TARGET neon_mdf_ota)
    add_subdirectory(${NEON_MDF_PATH}/components/ota ${CMAKE_BINARY_DIR}/neon_mdf_ota_build)
  endif()

  # Propagate moi bien NEON_MDF_FLASHMAP_* len PARENT_SCOPE - ham nay duoc
  # goi TRUC TIEP tu CMakeLists.txt cua app (khong qua add_subdirectory nao
  # them, khac kconfig.cmake) nen CHI can 1 tang PARENT_SCOPE, khong phai 2.
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
