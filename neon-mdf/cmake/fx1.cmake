# Self-contained BSP snapshot; no external VP path or runtime dependency.
set(_fx1_root "${NEON_MDF_PATH}/arch/RISCV/Device/FTEL/FX1")
set(_fx1_linker "${CMAKE_CURRENT_BINARY_DIR}/FX1.ld")
execute_process(COMMAND "${CMAKE_C_COMPILER}" -E -P -x c -DFX1_LINKER_SCRIPT
  "-I${_fx1_root}/Include" "${_fx1_root}/Source/gcc/fx1.ld.S" -o "${_fx1_linker}"
  RESULT_VARIABLE _fx1_result)
if(NOT _fx1_result EQUAL 0)
  message(FATAL_ERROR "Cannot preprocess FX1 linker script")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${_fx1_root}/Source/gcc/fx1.ld.S" "${_fx1_root}/Include/fx1/fx1_memory_map.h")
add_library(neon_mdf_device_FX1 STATIC
  "${_fx1_root}/Source/gcc/startup_FX1.S"
  "${_fx1_root}/Source/fx1_fw.c" "${_fx1_root}/Source/fx1_libc.c")
target_include_directories(neon_mdf_device_FX1 PUBLIC "${_fx1_root}/Include")
target_compile_definitions(neon_mdf_device_FX1 PUBLIC FX1 NEON_MDF_ARCH_RISCV)
target_compile_options(neon_mdf_device_FX1 PUBLIC
  -march=rv32imafc_zicsr_zifencei -mabi=ilp32f -mcmodel=medany -fno-builtin)
target_link_options(neon_mdf_device_FX1 PUBLIC
  -march=rv32imafc_zicsr_zifencei -mabi=ilp32f -mcmodel=medany
  -nostartfiles "LINKER:-u,_start")
target_link_libraries(neon_mdf_device_FX1 PUBLIC gcc)
set(NEON_MDF_DEVICE_TARGET neon_mdf_device_FX1 PARENT_SCOPE)
set(NEON_MDF_LINKER_SCRIPT "${_fx1_linker}" PARENT_SCOPE)
set(NEON_MDF_TARGET_ARCH riscv-rv32imafc_zicsr_zifencei PARENT_SCOPE)
set(NEON_MDF_BOOTLOADER_ENABLED OFF PARENT_SCOPE)
