# Registration of target MCUs (same as ESP-IDF concept IDF_TARGET: esp32, esp32c3...) that neon-mdf
# supports. Add new target = add 1 file mcu-description/devices/<DEVICE>.yaml + 2 lines of
# registration here (arch must match field `arch` in that .yaml file).
#
# Required: variable NEON_MDF_PATH must be set before include file (see tools/cmake/project.cmake) -
# this is the root of neon-mdf, not the app's CMAKE_SOURCE_DIR that includes project.cmake.

set(NEON_MDF_TARGET_FX1_ARCH riscv-rv32imafc_zicsr_zifencei)
set(NEON_MDF_TARGET_FX1_FAMILY FX1)

set(NEON_MDF_KNOWN_TARGETS FX1 FU2 STM32F103CB WEACT_F412 CH32V003F4P6)

set(NEON_MDF_TARGET_FU2_ARCH   cortex-m0)
set(NEON_MDF_TARGET_FU2_FAMILY FU2)

set(NEON_MDF_TARGET_STM32F103CB_ARCH   cortex-m3)
set(NEON_MDF_TARGET_STM32F103CB_FAMILY STM32F103)

# WEACT_F412 NOT FU2 - True STM32F412RET6 on WeAct Studio STM32F4 Core Board (real user board), real
# hardware test rig while waiting for TRM FU2 (see early warning mcu-description/devices/
# WEACT_F412.yaml). NO on-board debugger (other than Nucleo) - MUST use removable SWD probe. CPU
# core is cortex-m4 (Cortex-M4F, has real FPU) - FIRST Cortex-M4 chip in the SDK, needs its own FPU
# flag (see _FPU variable below).
set(NEON_MDF_TARGET_WEACT_F412_ARCH   cortex-m4)
set(NEON_MDF_TARGET_WEACT_F412_FAMILY WEACT_F412)
set(NEON_MDF_TARGET_WEACT_F412_FPU    ON)

# CH32V003F4P6 - WCH, the first RISC-V target of SDK (QingKe V2A, RV32EC) - real user dev board
# (WCH-LinkE probe). _VENDOR needs its own (other than Cortex-M) output path
# arch/RISCV/Device/<Vendor>/<Family>/ (see generate_riscv() in mcu-description/tools/generate.py
# and arch/RISCV/README.md - two-level convention Vendor/Family, different from one level of
# CortexM/CMSIS/Device/<Family> RISC-V does not have a common "CMSIS" and has a common ARM-like
# struct name). _ABI (ilp32e) needs its own because it cannot be guessed from _ARCH (isa "rv32ec"
# does not deduce an ABI 1-1, see cmake/arch/riscv.cmake).
set(NEON_MDF_TARGET_CH32V003F4P6_ARCH   riscv-rv32ec)
set(NEON_MDF_TARGET_CH32V003F4P6_FAMILY CH32V003)
set(NEON_MDF_TARGET_CH32V003F4P6_VENDOR WCH)
set(NEON_MDF_TARGET_CH32V003F4P6_ABI    ilp32e)

# neon_mdf_configure_target(<TARGET_NAME>)
#   - Run mcu-description generator to ensure the latest header/linker/startup according to current
#     mcu-description/devices/<TARGET_NAME>.yaml.
#   - Create static target `neon_mdf_device_<TARGET_NAME>` containing system_<TARGET_NAME>.c + startup.
#   - Set external variables (PARENT_SCOPE): NEON_MDF_DEVICE_TARGET - name CMake target to
#       target_link_libraries NEON_MDF_LINKER_SCRIPT - path linker script to -T
#
# Note: the generator runs once at CMake configure (execute_process), not build-time custom command -
# after editing the .yaml file, re-run cmake configure for the header to be created again. Simple
# enough to use at the current stage.
function(neon_mdf_configure_target TARGET_NAME)
  if(TARGET_NAME STREQUAL "FX1")
    include(${NEON_MDF_PATH}/cmake/fx1.cmake)
    return()
  endif()
  if(NOT NEON_MDF_PATH)
    message(FATAL_ERROR "NEON_MDF_PATH not set before include cmake/mcu_variants.cmake")
  endif()

  set(_arch   ${NEON_MDF_TARGET_${TARGET_NAME}_ARCH})
  set(_family ${NEON_MDF_TARGET_${TARGET_NAME}_FAMILY})
  if(NOT _arch)
    message(FATAL_ERROR "Target '${TARGET_NAME}' has not been periodized in cmake/mcu_variants.cmake")
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
    message(FATAL_ERROR "generate code for target '${TARGET_NAME}' failed:\n${_gen_output}${_gen_error}")
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

    # bootloader: optional (see mcu-description/schema/common.schema.yaml). generate.py only creates
    # mcu-description/generated/<TARGET>_bootloader.cmake when the YAML device has a `bootloader:`
    # declaration. This is how CMake determines whether the device has a bootloader; the YAML is read
    # only by Python, see loader.py) - same pattern as cmake/kconfig.cmake (Python fragment
    # generator, CMake only include()).
    set(NEON_MDF_BOOTLOADER_ENABLED OFF)
    set(_bootloader_cmake_fragment ${NEON_MDF_PATH}/mcu-description/generated/${TARGET_NAME}_bootloader.cmake)
    if(EXISTS ${_bootloader_cmake_fragment})
      include(${_bootloader_cmake_fragment})
      set(NEON_MDF_BOOTLOADER_LINKER_SCRIPT ${_cmsis_device_root}/Source/gcc/linker/${_linker_BOOTLOADER} PARENT_SCOPE)
      set(NEON_MDF_APP_LINKER_SCRIPT ${_cmsis_device_root}/Source/gcc/linker/${_linker_APP} PARENT_SCOPE)
      # include() above refers to set() these variables in the SCOPE of this function (CMake each function()
      # is a separate scope) - must be inline PARENT_SCOPE then CMakeLists.txt call
      # neon_mdf_add_flash_targets() each visible, otherwise these 3 variables will be empty there
      # (confirmed by real test: before fixing, message(STATUS "${NEON_MDF_IMAGE_HEADER_SIZE}")
      # right after neon_mdf_add_flash_targets() in CMakeLists.txt prints an empty string).
      set(NEON_MDF_BOOTLOADER_SIZE ${NEON_MDF_BOOTLOADER_SIZE} PARENT_SCOPE)
      set(NEON_MDF_IMAGE_HEADER_SIZE ${NEON_MDF_IMAGE_HEADER_SIZE} PARENT_SCOPE)
      set(NEON_MDF_APP_BASE ${NEON_MDF_APP_BASE} PARENT_SCOPE)
      # RAM_BASE/RAM_SIZE/ERASE_SECTORS - just for tools/gen_flashmap.py (see cmake/flashmap.cmake) - same
      # Propagate through both scopes with PARENT_SCOPE, like the three variables above. Otherwise
      # gen_flashmap.py receives an empty --ram-base and subsequent arguments shift; confirmed
      # during validation.
      # after no).
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
      message(FATAL_ERROR "Target RISC-V '${TARGET_NAME}' missing NEON_MDF_TARGET_${TARGET_NAME}_VENDOR in cmake/mcu_variants.cmake")
    endif()
    if(NOT _abi)
      message(FATAL_ERROR "Target RISC-V '${TARGET_NAME}' missing NEON_MDF_TARGET_${TARGET_NAME}_ABI in cmake/mcu_variants.cmake")
    endif()

    # isa = _arch minus first to "riscv-" (e.g. "riscv-rv32ec" -> "rv32ec").
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
    # NEON_MDF_ARCH_RISCV - generic macro for each RISC-V target (not CH32V003F4P6) - used by
    # components/bootloader/neon_bootloader_main.c to bop back to ARM-specific
    # (SysTick/jump_to_app/system reset, see comment at the beginning of that file) without using
    # existing Cortex-M.
    target_compile_definitions(neon_mdf_device_${TARGET_NAME} PUBLIC ${TARGET_NAME} NEON_MDF_ARCH_RISCV)
    neon_mdf_apply_riscv_arch(neon_mdf_device_${TARGET_NAME} ${_isa} ${_abi})
    # __udivsi3/__umodsi3/__mulsi3 - from the same level (arch/RISCV/Core/Src/riscv_soft_math.c, added
    # SOURCES above) - do not use -lgcc he flux (this Homebrew toolchain does not have multilib
    # match ilp32e, see full stage in riscv_soft_math.c).

    set(NEON_MDF_DEVICE_TARGET neon_mdf_device_${TARGET_NAME} PARENT_SCOPE)
    set(NEON_MDF_LINKER_SCRIPT ${_riscv_device_root}/Source/gcc/linker/${_linker_FLASH} PARENT_SCOPE)
    set(NEON_MDF_TARGET_ARCH ${_arch} PARENT_SCOPE)

    # bootloader: optional, same pattern as the cortex-m accelerator above (Python creates fragments,
    # CMake only include()) - CH32V003F4P6.yaml has now declared `bootloader:` (Phase 6, see RISC-V
    # plan) so this accelerator is essentially dead, unlike Phase 3 (when child is OFF together).
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
    message(FATAL_ERROR "Arch '${_arch}' is not received for target ${TARGET_NAME}")
  endif()
endfunction()
