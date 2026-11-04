# neon-mdf/CMakeLists.txt - build đúng phần "SDK" (components/driver + device
# target), KHÔNG chứa app nào. Giống $IDF_PATH/CMakeLists.txt của ESP-IDF:
# không tự đứng 1 mình được, luôn add_subdirectory() bởi
# tools/cmake/project.cmake (được include từ CMakeLists.txt của 1 app).

if(NOT NEON_MDF_PATH)
  set(NEON_MDF_PATH ${CMAKE_CURRENT_SOURCE_DIR})
endif()
if(NOT NEON_MDF_TARGET)
  message(FATAL_ERROR "NEON_MDF_TARGET phải được set trước khi add_subdirectory(neon-mdf)")
endif()

include(${NEON_MDF_PATH}/cmake/mcu_variants.cmake)
if(NOT NEON_MDF_TARGET IN_LIST NEON_MDF_KNOWN_TARGETS)
  message(FATAL_ERROR "NEON_MDF_TARGET='${NEON_MDF_TARGET}' chưa đăng ký trong cmake/mcu_variants.cmake (target hợp lệ: ${NEON_MDF_KNOWN_TARGETS})")
endif()

neon_mdf_configure_target(${NEON_MDF_TARGET})

# PARENT_SCOPE: file này luôn là subdirectory (không bao giờ top-level), nên
# luôn cần propagate lên caller (CMakeLists.txt của app) để dùng
# -T${NEON_MDF_LINKER_SCRIPT} cho executable của họ.
set(NEON_MDF_DEVICE_TARGET ${NEON_MDF_DEVICE_TARGET} PARENT_SCOPE)
set(NEON_MDF_LINKER_SCRIPT ${NEON_MDF_LINKER_SCRIPT} PARENT_SCOPE)
# NEON_MDF_TARGET_ARCH - can cho tools/cmake/project.cmake's
# neon_mdf_add_bootloader_executable() (phan biet co can them `gcc` vao
# -Wl,--start-group hay khong, xem comment o do) va cmake/flashmap.cmake
# (--arch cho gen_flashmap.py, xem comment o do).
set(NEON_MDF_TARGET_ARCH ${NEON_MDF_TARGET_ARCH} PARENT_SCOPE)
set(NEON_MDF_BOOTLOADER_ENABLED ${NEON_MDF_BOOTLOADER_ENABLED} PARENT_SCOPE)
if(NEON_MDF_BOOTLOADER_ENABLED)
  set(NEON_MDF_BOOTLOADER_LINKER_SCRIPT ${NEON_MDF_BOOTLOADER_LINKER_SCRIPT} PARENT_SCOPE)
  set(NEON_MDF_APP_LINKER_SCRIPT ${NEON_MDF_APP_LINKER_SCRIPT} PARENT_SCOPE)
  # Can cho flash_jtag/flash_uart_stm32 (xem cmake/flash_targets.cmake) - dia
  # chi/kich thuoc bootloader + app (factory) de ghi 2 vung RIENG (khong con
  # ghep 1 blob lien tuc) qua SWD (flash_jtag) hoac System Bootloader that
  # qua UART (flash_uart_stm32).
  set(NEON_MDF_BOOTLOADER_SIZE ${NEON_MDF_BOOTLOADER_SIZE} PARENT_SCOPE)
  set(NEON_MDF_IMAGE_HEADER_SIZE ${NEON_MDF_IMAGE_HEADER_SIZE} PARENT_SCOPE)
  set(NEON_MDF_APP_BASE ${NEON_MDF_APP_BASE} PARENT_SCOPE)
  # Can cho tools/gen_flashmap.py (xem cmake/flashmap.cmake) - PARENT_SCOPE
  # tang thu 2 (hop add_subdirectory(neon-mdf) nay), cung bai hoc voi 3 bien
  # tren.
  set(NEON_MDF_RAM_BASE ${NEON_MDF_RAM_BASE} PARENT_SCOPE)
  set(NEON_MDF_RAM_SIZE ${NEON_MDF_RAM_SIZE} PARENT_SCOPE)
  set(NEON_MDF_ERASE_SECTORS ${NEON_MDF_ERASE_SECTORS} PARENT_SCOPE)
endif()

# flashmap.csv (partition table CUA RIENG app, xem cmake/flashmap.cmake) -
# TUY CHON, tu dong bat NEU app co file nay (CMAKE_SOURCE_DIR = thu muc app,
# vi app luon la project() top-level - khong phai thu muc nay) - giong tinh
# than osource cua kconfig cho Kconfig rieng cua app, app KHONG can sua
# CMakeLists.txt cua rieng no de "bat" tinh nang nay, chi can co file la du.
# PHAI goi TRUOC add_subdirectory(components/bootloader) o duoi - de
# components/bootloader/CMakeLists.txt link duoc target neon_mdf_ota (tao
# boi neon_mdf_process_flashmap() qua add_subdirectory(components/ota)).
#
# UU TIEN flashmap.<NEON_MDF_TARGET>.csv (override THEO DEVICE) neu co,
# fallback flashmap.csv (mac dinh/dung chung) - can cho app build duoc
# nhieu device CO BOOTLOADER voi flash size chenh lech lon (vd Blinky:
# WEACT_F412 512KB vs CH32V003F4P6 16KB - KHONG the dung chung 1
# flashmap.csv, xem comment day du trong cmake/flashmap.cmake).
set(_flashmap_csv_candidate ${CMAKE_SOURCE_DIR}/flashmap.${NEON_MDF_TARGET}.csv)
if(NOT EXISTS ${_flashmap_csv_candidate})
  set(_flashmap_csv_candidate ${CMAKE_SOURCE_DIR}/flashmap.csv)
endif()
set(NEON_MDF_APP_HAS_FLASHMAP OFF)
if(NEON_MDF_BOOTLOADER_ENABLED AND EXISTS ${_flashmap_csv_candidate})
  include(${NEON_MDF_PATH}/cmake/flashmap.cmake)
  neon_mdf_process_flashmap(${CMAKE_SOURCE_DIR} ${_flashmap_csv_candidate})
  set(NEON_MDF_APP_HAS_FLASHMAP ON)
  # PARENT_SCOPE tang thu 2 (hop add_subdirectory(neon-mdf) nay) cho MOI
  # bien NEON_MDF_FLASHMAP_* - danh sach ten la DONG (tuy partition trong
  # flashmap.csv cua tung app) nen phai lap qua NEON_MDF_FLASHMAP_VAR_NAMES
  # (do neon_mdf_process_flashmap() tra ve), khong hardcode duoc nhu 3 bien
  # BOOTLOADER_SIZE/IMAGE_HEADER_SIZE/APP_BASE o tren.
  foreach(_flashmap_name ${NEON_MDF_FLASHMAP_VAR_NAMES})
    set(${_flashmap_name} ${${_flashmap_name}} PARENT_SCOPE)
  endforeach()
endif()
set(NEON_MDF_APP_HAS_FLASHMAP ${NEON_MDF_APP_HAS_FLASHMAP} PARENT_SCOPE)

# components/driver viết cho register layout dùng chung của family FU2 (xem
# ARCHITECTURE.md mục 6) - guard theo FAMILY, không phải tên target
# chính xác. WEACT_F412 cũng dùng components/driver (GPIO backend riêng cho
# STM32F4, xem components/driver/CMakeLists.txt) nên nằm trong danh sách
# family được phép. STM32F103 dùng GPIO backend CRL/CRH riêng; CH32V003
# (RISC-V, WCH) dùng GPIO backend CFGLR riêng.
if(NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "FU2" OR
   NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "STM32F103" OR
   NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "WEACT_F412" OR
   NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "CH32V003")
  include(${NEON_MDF_PATH}/cmake/kconfig.cmake)
  neon_mdf_process_kconfig(${CMAKE_SOURCE_DIR})

  add_subdirectory(components/driver)
  add_subdirectory(components/console)

  # FU2 has a mask-ROM first-stage loader.  Keep its policy in a separate
  # component from the optional flash-resident second-stage bootloader.
  # Applications do not link this target unless they explicitly build a ROM
  # image. Application-only SDK checkouts can omit the ROM sources and use
  # the device's existing ROM instead.
  if(NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "FU2" AND
     EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/components/bootrom/CMakeLists.txt")
    add_subdirectory(components/bootrom)
  endif()

  # components/bootloader can neon_mdf_driver (flash) da duoc dinh nghia o
  # tren - CHI add khi device dang build THAT SU co khai `bootloader:` (xem
  # cmake/mcu_variants.cmake).
  if(NEON_MDF_BOOTLOADER_ENABLED)
    add_subdirectory(components/bootloader)
    # NEON_MDF_BOOTLOADER_MAIN_SOURCES - PARENT_SCOPE tang thu 2 (hop
    # add_subdirectory(neon-mdf) nay) - dung boi
    # neon_mdf_add_bootloader_executable() trong tools/cmake/project.cmake
    # (chay o scope cua app) de compile TRUC TIEP main.c+arch file nhu
    # nguon cua executable "bootloader", xem
    # components/bootloader/CMakeLists.txt.
    set(NEON_MDF_BOOTLOADER_MAIN_SOURCES ${NEON_MDF_BOOTLOADER_MAIN_SOURCES} PARENT_SCOPE)
  endif()
endif()

# FX1 uses the bundled CLINT/PLIC BSP, with no MCU GPIO HAL dependency.
if(NEON_MDF_TARGET STREQUAL "FX1")
  include(${NEON_MDF_PATH}/cmake/kconfig.cmake)
  neon_mdf_process_kconfig(${CMAKE_SOURCE_DIR})
  add_library(neon_mdf_driver INTERFACE)
  target_link_libraries(neon_mdf_driver INTERFACE ${NEON_MDF_DEVICE_TARGET} neon_mdf_kconfig)
  if(CONFIG_NEON_MDF_FTEL_ISP)
    add_subdirectory(components/ftel_isp)
  endif()
endif()
