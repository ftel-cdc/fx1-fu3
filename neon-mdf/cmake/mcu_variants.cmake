# GT32U2086 is the only supported device.
set(NEON_MDF_KNOWN_TARGETS GT32U2086)
set(NEON_MDF_TARGET_GT32U2086_ARCH cortex-m0)
set(NEON_MDF_TARGET_GT32U2086_FAMILY GT32U2086)

function(neon_mdf_configure_target TARGET_NAME)
  if(NOT TARGET_NAME STREQUAL "GT32U2086")
    message(FATAL_ERROR "Unsupported target '${TARGET_NAME}'; use GT32U2086")
  endif()
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  if(NOT DEFINED NEON_MDF_PROFILE)
    set(NEON_MDF_PROFILE normal)
  endif()
  execute_process(
    COMMAND ${Python3_EXECUTABLE} ${NEON_MDF_PATH}/mcu-description/tools/generate.py
            --device ${TARGET_NAME} --profile ${NEON_MDF_PROFILE}
    WORKING_DIRECTORY ${NEON_MDF_PATH}
    RESULT_VARIABLE _gen_result
    OUTPUT_VARIABLE _gen_output
    ERROR_VARIABLE _gen_error
  )
  if(NOT _gen_result EQUAL 0)
    message(FATAL_ERROR "Device generation failed:\n${_gen_output}${_gen_error}")
  endif()
  message(STATUS "[mcu-description] ${_gen_output}")

  include(${NEON_MDF_PATH}/cmake/arch/cortex-m.cmake)
  set(_device_root ${NEON_MDF_PATH}/arch/CortexM/CMSIS/Device/GT32U2086)
  add_library(neon_mdf_device_${TARGET_NAME} STATIC
    ${_device_root}/Source/system_GT32U2086.c
    ${_device_root}/Source/gcc/startup_GT32U2086.s
    ${_device_root}/Source/gt32u2086_runtime.c
  )
  target_include_directories(neon_mdf_device_${TARGET_NAME} PUBLIC
    ${_device_root}/Include
    ${NEON_MDF_PATH}/arch/CortexM/CMSIS/CMSIS_5/CMSIS/Core/Include
  )
  target_include_directories(neon_mdf_device_${TARGET_NAME} PRIVATE
    ${NEON_MDF_PATH}/components/driver)
  target_compile_definitions(neon_mdf_device_${TARGET_NAME} PUBLIC GT32U2086)
  neon_mdf_apply_cortex_m_arch(neon_mdf_device_${TARGET_NAME} cortex-m0)
  # All application, driver and kernel code must use the word-only SRAM policy.
  target_compile_options(neon_mdf_device_${TARGET_NAME} PUBLIC
    "$<$<COMPILE_LANGUAGE:C,CXX>:-O2;-fno-short-enums;-fno-builtin;-fno-tree-switch-conversion>")
  set(NEON_MDF_DEVICE_TARGET neon_mdf_device_${TARGET_NAME} PARENT_SCOPE)
  set(NEON_MDF_LINKER_SCRIPT ${_device_root}/Source/gcc/linker/GT32U2086_FLASH.ld PARENT_SCOPE)
endfunction()
