# Self-contained BSP snapshot; no external VP path or runtime dependency.
set(_fx1_root "${NEON_MDF_PATH}/arch/RISCV/Device/FTEL/FX1")
find_program(NEON_MDF_PYTHON3 NAMES python3 python REQUIRED)
execute_process(COMMAND "${NEON_MDF_PYTHON3}"
  "${NEON_MDF_PATH}/mcu-description/tools/generate.py" --device FX1
  RESULT_VARIABLE _fx1_gen_result OUTPUT_VARIABLE _fx1_gen_out ERROR_VARIABLE _fx1_gen_err)
if(NOT _fx1_gen_result EQUAL 0)
  message(FATAL_ERROR "FX1 YAML: ${_fx1_gen_out}${_fx1_gen_err}")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${NEON_MDF_PATH}/mcu-description/devices/FX1.yaml"
  "${NEON_MDF_PATH}/mcu-description/tools/generate.py"
  "${NEON_MDF_PATH}/mcu-description/tools/mcu_desc/model.py"
  "${NEON_MDF_PATH}/mcu-description/tools/mcu_desc/loader.py"
  "${NEON_MDF_PATH}/mcu-description/tools/backends/riscv/gen_fx1_bsp.py"
  "${NEON_MDF_PATH}/mcu-description/tools/backends/riscv/gen_fx1_registers.py")
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
  "${_fx1_root}/Source/fx1_fw.c" "${_fx1_root}/Source/fx1_libc.c"
  "${_fx1_root}/Source/sdk_irq_fx1.c")
target_include_directories(neon_mdf_device_FX1 PUBLIC "${_fx1_root}/Include" "${NEON_MDF_PATH}/arch/RISCV/Port/Inc")
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
