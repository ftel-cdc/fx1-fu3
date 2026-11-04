# Đăng ký các MCU target (giống khái niệm IDF_TARGET của ESP-IDF: esp32,
# esp32c3...) mà neon-mdf hỗ trợ. Thêm target mới = thêm 1 file
# mcu-description/devices/<DEVICE>.yaml + 2 dòng đăng ký ở đây (arch phải khớp
# đúng field `arch` trong file .yaml đó).
#
# Yêu cầu: biến NEON_MDF_PATH phải được set trước khi include file này (xem
# tools/cmake/project.cmake) - đây là thư mục gốc của neon-mdf, không phải
# CMAKE_SOURCE_DIR của app đang include project.cmake.

set(NEON_MDF_KNOWN_TARGETS FU2 STM32F103CB WEACT_F412 CH32V003F4P6)

set(NEON_MDF_TARGET_FU2_ARCH   cortex-m0)
set(NEON_MDF_TARGET_FU2_FAMILY FU2)

set(NEON_MDF_TARGET_STM32F103CB_ARCH   cortex-m3)
set(NEON_MDF_TARGET_STM32F103CB_FAMILY STM32F103)

# WEACT_F412 KHÔNG PHẢI FU2 - STM32F412RET6 thật trên board WeAct Studio
# STM32F4 Core Board (board THẬT user đang có), test rig phần cứng THẬT
# trong lúc chờ TRM FU2 (xem cảnh báo đầu mcu-description/devices/
# WEACT_F412.yaml). KHÔNG CÓ debugger on-board (khác Nucleo) - PHẢI dùng
# probe SWD rời. Lõi CPU là cortex-m4 (Cortex-M4F, CÓ FPU thật) - chip
# Cortex-M4 ĐẦU TIÊN trong SDK, cần cờ FPU riêng (xem biến _FPU dưới).
set(NEON_MDF_TARGET_WEACT_F412_ARCH   cortex-m4)
set(NEON_MDF_TARGET_WEACT_F412_FAMILY WEACT_F412)
set(NEON_MDF_TARGET_WEACT_F412_FPU    ON)

# CH32V003F4P6 - WCH, target RISC-V DAU TIEN cua SDK (QingKe V2A, RV32EC) -
# board dev THAT user dang co (WCH-LinkE probe). _VENDOR can rieng (khac
# Cortex-M) vi duong dan output la arch/RISCV/Device/<Vendor>/<Family>/ (xem
# generate_riscv() trong mcu-description/tools/generate.py va
# arch/RISCV/README.md - quy uoc 2 cap Vendor/Family, khac 1 cap cua
# CortexM/CMSIS/Device/<Family> vi RISC-V khong co "CMSIS" chung 1 kieu dat
# ten struct nhu ARM). _ABI (ilp32e) can rieng vi khong the tu doan tu _ARCH
# (isa "rv32ec" khong tu suy ra ABI 1-1, xem cmake/arch/riscv.cmake).
set(NEON_MDF_TARGET_CH32V003F4P6_ARCH   riscv-rv32ec)
set(NEON_MDF_TARGET_CH32V003F4P6_FAMILY CH32V003)
set(NEON_MDF_TARGET_CH32V003F4P6_VENDOR WCH)
set(NEON_MDF_TARGET_CH32V003F4P6_ABI    ilp32e)

# neon_mdf_configure_target(<TARGET_NAME>)
#   - Chạy mcu-description generator để đảm bảo header/linker/startup mới nhất
#     theo mcu-description/devices/<TARGET_NAME>.yaml hiện tại.
#   - Tạo target tĩnh `neon_mdf_device_<TARGET_NAME>` chứa system_<TARGET_NAME>.c + startup.
#   - Set biến ra ngoài (PARENT_SCOPE):
#       NEON_MDF_DEVICE_TARGET   - tên CMake target để target_link_libraries
#       NEON_MDF_LINKER_SCRIPT   - path linker script để -T
#
# Lưu ý: generator chạy 1 lần lúc CMake configure (execute_process), không
# phải build-time custom command - sau khi sửa file .yaml phải re-run cmake
# configure để header được sinh lại. Đơn giản, đủ dùng ở giai đoạn hiện tại.
function(neon_mdf_configure_target TARGET_NAME)
  if(NOT NEON_MDF_PATH)
    message(FATAL_ERROR "NEON_MDF_PATH chua duoc set truoc khi include cmake/mcu_variants.cmake")
  endif()

  set(_arch   ${NEON_MDF_TARGET_${TARGET_NAME}_ARCH})
  set(_family ${NEON_MDF_TARGET_${TARGET_NAME}_FAMILY})
  if(NOT _arch)
    message(FATAL_ERROR "Target '${TARGET_NAME}' chua duoc dang ky trong cmake/mcu_variants.cmake")
  endif()

  # Lowercase device IDs use lowercase artifact names; keep legacy paths.
  string(TOLOWER "${TARGET_NAME}" _target_lower)
  foreach(_image FLASH BOOTLOADER APP)
    set(_linker_${_image} "${TARGET_NAME}_${_image}.ld")
    if("${TARGET_NAME}" STREQUAL "${_target_lower}")
      string(TOLOWER "${_linker_${_image}}" _linker_${_image})
    endif()
  endforeach()

  find_program(NEON_MDF_PYTHON3 python3 REQUIRED)
  if(NOT DEFINED NEON_MDF_PROFILE)
    set(NEON_MDF_PROFILE normal)
  endif()
  execute_process(
    COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/mcu-description/tools/generate.py --device ${TARGET_NAME} --profile "${NEON_MDF_PROFILE}"
    WORKING_DIRECTORY ${NEON_MDF_PATH}
    RESULT_VARIABLE _gen_result
    OUTPUT_VARIABLE _gen_output
    ERROR_VARIABLE _gen_error
  )
  message(STATUS "[mcu-description] ${_gen_output}")
  if(NOT _gen_result EQUAL 0)
    message(FATAL_ERROR "Sinh code cho target '${TARGET_NAME}' that bai:\n${_gen_output}${_gen_error}")
  endif()

  if(_arch MATCHES "^cortex-m")
    include(${NEON_MDF_PATH}/cmake/arch/cortex-m.cmake)

    set(_cmsis_device_root  ${NEON_MDF_PATH}/arch/CortexM/CMSIS/Device/${_family})
    set(_cmsis_core_include ${NEON_MDF_PATH}/arch/CortexM/CMSIS/CMSIS_5/CMSIS/Core/Include)

    add_library(neon_mdf_device_${TARGET_NAME} STATIC
      ${_cmsis_device_root}/Source/system_${TARGET_NAME}.c
      ${_cmsis_device_root}/Source/gcc/startup_${TARGET_NAME}.s
    )
    target_include_directories(neon_mdf_device_${TARGET_NAME} PUBLIC
      ${_cmsis_device_root}/Include
      ${_cmsis_core_include}
    )
    target_compile_definitions(neon_mdf_device_${TARGET_NAME} PUBLIC ${TARGET_NAME})
    neon_mdf_apply_cortex_m_arch(neon_mdf_device_${TARGET_NAME} ${_arch} ${NEON_MDF_TARGET_${TARGET_NAME}_FPU})

    if(_family STREQUAL "FU2")
      target_sources(neon_mdf_device_${TARGET_NAME} PRIVATE
        ${_cmsis_device_root}/Source/fu2_runtime.c)
      target_include_directories(neon_mdf_device_${TARGET_NAME} PRIVATE
        ${NEON_MDF_PATH}/components/driver)
      # Propagate the word-only SRAM compiler policy to every FU2 driver/app.
      # Narrow parameters can spill to SRAM at -O0; short enums use byte fields.
      target_compile_options(neon_mdf_device_${TARGET_NAME} PUBLIC
        "$<$<COMPILE_LANGUAGE:C,CXX>:-O2;-fno-short-enums;-fno-builtin;-fno-tree-switch-conversion>")
    endif()

    set(NEON_MDF_DEVICE_TARGET neon_mdf_device_${TARGET_NAME} PARENT_SCOPE)
    set(NEON_MDF_LINKER_SCRIPT ${_cmsis_device_root}/Source/gcc/linker/${_linker_FLASH} PARENT_SCOPE)
    set(NEON_MDF_TARGET_ARCH ${_arch} PARENT_SCOPE)

    # bootloader: TUY CHON (xem mcu-description/schema/common.schema.yaml).
    # generate.py CHI sinh mcu-description/generated/<TARGET>_bootloader.cmake
    # khi device YAML co khai `bootloader:` - day la cach DUY NHAT CMake biet
    # duoc device nay co bootloader hay khong (YAML chi duoc doc boi Python,
    # xem loader.py) - cung pattern voi cmake/kconfig.cmake (Python sinh
    # fragment, CMake chi include()).
    set(NEON_MDF_BOOTLOADER_ENABLED OFF)
    set(_bootloader_cmake_fragment ${NEON_MDF_PATH}/mcu-description/generated/${TARGET_NAME}_bootloader.cmake)
    if(EXISTS ${_bootloader_cmake_fragment})
      include(${_bootloader_cmake_fragment})
      set(NEON_MDF_BOOTLOADER_LINKER_SCRIPT ${_cmsis_device_root}/Source/gcc/linker/${_linker_BOOTLOADER} PARENT_SCOPE)
      set(NEON_MDF_APP_LINKER_SCRIPT ${_cmsis_device_root}/Source/gcc/linker/${_linker_APP} PARENT_SCOPE)
      # include() o tren chi set() cac bien nay trong SCOPE CUA HAM NAY (CMake
      # moi function() la 1 scope rieng) - PHAI truyen tiep PARENT_SCOPE thi
      # CMakeLists.txt goi neon_mdf_add_flash_targets() moi thay duoc, neu
      # khong 3 bien nay se RONG o do (da xac nhan bang test that: truoc khi
      # sua, message(STATUS "${NEON_MDF_IMAGE_HEADER_SIZE}") ngay sau
      # neon_mdf_add_flash_targets() trong CMakeLists.txt in ra chuoi rong).
      set(NEON_MDF_BOOTLOADER_SIZE ${NEON_MDF_BOOTLOADER_SIZE} PARENT_SCOPE)
      set(NEON_MDF_IMAGE_HEADER_SIZE ${NEON_MDF_IMAGE_HEADER_SIZE} PARENT_SCOPE)
      set(NEON_MDF_APP_BASE ${NEON_MDF_APP_BASE} PARENT_SCOPE)
      # RAM_BASE/RAM_SIZE/ERASE_SECTORS - chi can cho tools/gen_flashmap.py
      # (xem cmake/flashmap.cmake) - cung PARENT_SCOPE 2 tang nhu 3 bien tren,
      # bai hoc CMake giong het (da xac nhan that: thieu dong nay se lam
      # gen_flashmap.py nhan --ram-base RONG, lech het cac tham so sau no).
      set(NEON_MDF_RAM_BASE ${NEON_MDF_RAM_BASE} PARENT_SCOPE)
      set(NEON_MDF_RAM_SIZE ${NEON_MDF_RAM_SIZE} PARENT_SCOPE)
      set(NEON_MDF_ERASE_SECTORS ${NEON_MDF_ERASE_SECTORS} PARENT_SCOPE)
      set(NEON_MDF_FLASH_SIZE ${NEON_MDF_FLASH_SIZE} PARENT_SCOPE)
    endif()
    set(NEON_MDF_BOOTLOADER_ENABLED ${NEON_MDF_BOOTLOADER_ENABLED} PARENT_SCOPE)
  elseif(_arch MATCHES "^riscv")
    include(${NEON_MDF_PATH}/cmake/arch/riscv.cmake)

    set(_vendor ${NEON_MDF_TARGET_${TARGET_NAME}_VENDOR})
    set(_abi    ${NEON_MDF_TARGET_${TARGET_NAME}_ABI})
    if(NOT _vendor)
      message(FATAL_ERROR "Target RISC-V '${TARGET_NAME}' thieu NEON_MDF_TARGET_${TARGET_NAME}_VENDOR trong cmake/mcu_variants.cmake")
    endif()
    if(NOT _abi)
      message(FATAL_ERROR "Target RISC-V '${TARGET_NAME}' thieu NEON_MDF_TARGET_${TARGET_NAME}_ABI trong cmake/mcu_variants.cmake")
    endif()

    # isa = _arch tru tien to "riscv-" (vd "riscv-rv32ec" -> "rv32ec").
    string(REPLACE "riscv-" "" _isa ${_arch})

    set(_riscv_device_root ${NEON_MDF_PATH}/arch/RISCV/Device/${_vendor}/${_family})
    set(_riscv_core_include ${NEON_MDF_PATH}/arch/RISCV/Core/Inc)
    set(_port_root          ${NEON_MDF_PATH}/arch/RISCV/Port)

    add_library(neon_mdf_device_${TARGET_NAME} STATIC
      ${_riscv_device_root}/Source/system_${TARGET_NAME}.c
      ${_riscv_device_root}/Source/gcc/startup_${TARGET_NAME}.s
      ${_port_root}/Src/sdk_irq.c
      ${NEON_MDF_PATH}/arch/RISCV/Core/Src/riscv_soft_math.c
    )
    target_include_directories(neon_mdf_device_${TARGET_NAME} PUBLIC
      ${_riscv_device_root}/Include
      ${_riscv_core_include}
      ${_port_root}/Inc
    )
    # NEON_MDF_ARCH_RISCV - macro chung cho MOI target RISC-V (khong rieng
    # CH32V003F4P6) - dung boi components/bootloader/neon_bootloader_main.c
    # de bop lai DUNG phan ARM-specific (SysTick/jump_to_app/system reset,
    # xem comment dau file do) ma khong dung toi nhanh Cortex-M hien co.
    target_compile_definitions(neon_mdf_device_${TARGET_NAME} PUBLIC ${TARGET_NAME} NEON_MDF_ARCH_RISCV)
    neon_mdf_apply_riscv_arch(neon_mdf_device_${TARGET_NAME} ${_isa} ${_abi})
    # __udivsi3/__umodsi3/__mulsi3 - TU CUNG CAP (arch/RISCV/Core/Src/riscv_soft_math.c,
    # da them vao SOURCES o tren) - KHONG dung -lgcc he thong (toolchain
    # Homebrew nay khong co multilib khop ilp32e, xem giai thich day du
    # trong riscv_soft_math.c).

    set(NEON_MDF_DEVICE_TARGET neon_mdf_device_${TARGET_NAME} PARENT_SCOPE)
    set(NEON_MDF_LINKER_SCRIPT ${_riscv_device_root}/Source/gcc/linker/${_linker_FLASH} PARENT_SCOPE)
    set(NEON_MDF_TARGET_ARCH ${_arch} PARENT_SCOPE)

    # bootloader: TUY CHON, cung pattern voi nhanh cortex-m o tren (Python
    # sinh fragment, CMake chi include()) - CH32V003F4P6.yaml gio DA khai
    # `bootloader:` (Phase 6, xem plan RISC-V) nen nhanh nay THAT SU chay
    # toi, khac luc Phase 3 (khi con la OFF cung).
    set(NEON_MDF_BOOTLOADER_ENABLED OFF)
    set(_bootloader_cmake_fragment ${NEON_MDF_PATH}/mcu-description/generated/${TARGET_NAME}_bootloader.cmake)
    if(EXISTS ${_bootloader_cmake_fragment})
      include(${_bootloader_cmake_fragment})
      set(NEON_MDF_BOOTLOADER_LINKER_SCRIPT ${_riscv_device_root}/Source/gcc/linker/${_linker_BOOTLOADER} PARENT_SCOPE)
      set(NEON_MDF_APP_LINKER_SCRIPT ${_riscv_device_root}/Source/gcc/linker/${_linker_APP} PARENT_SCOPE)
      set(NEON_MDF_BOOTLOADER_SIZE ${NEON_MDF_BOOTLOADER_SIZE} PARENT_SCOPE)
      set(NEON_MDF_IMAGE_HEADER_SIZE ${NEON_MDF_IMAGE_HEADER_SIZE} PARENT_SCOPE)
      set(NEON_MDF_APP_BASE ${NEON_MDF_APP_BASE} PARENT_SCOPE)
      set(NEON_MDF_RAM_BASE ${NEON_MDF_RAM_BASE} PARENT_SCOPE)
      set(NEON_MDF_RAM_SIZE ${NEON_MDF_RAM_SIZE} PARENT_SCOPE)
      set(NEON_MDF_ERASE_SECTORS ${NEON_MDF_ERASE_SECTORS} PARENT_SCOPE)
      set(NEON_MDF_FLASH_SIZE ${NEON_MDF_FLASH_SIZE} PARENT_SCOPE)
    endif()
    set(NEON_MDF_BOOTLOADER_ENABLED ${NEON_MDF_BOOTLOADER_ENABLED} PARENT_SCOPE)
  else()
    message(FATAL_ERROR "Arch '${_arch}' khong duoc nhan dien cho target ${TARGET_NAME}")
  endif()
endfunction()
