include_guard(GLOBAL)
include(${CMAKE_CURRENT_LIST_DIR}/gt32u2086_flash.cmake)

# Used by the SDK project helper and standalone GT32U2086 examples after generation.
# The raw payload and boot image use an _xip suffix before .bin for XIP.
function(neon_mdf_add_gt32u2086_image target)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  include("${NEON_MDF_PATH}/mcu-description/generated/GT32U2086_image.cmake")
  if(NOT NEON_MDF_GT32U2086_IMAGE_HEADER_SIZE EQUAL 128)
    message(FATAL_ERROR "GT32U2086 requires the RTL BootROM v5 128-byte image header")
  endif()
  if(NOT DEFINED NEON_MDF_GT32U2086_APP_VERSION)
    set(NEON_MDF_GT32U2086_APP_VERSION "0.0.1")
  endif()
  set(_suffix "")
  if(NEON_MDF_GT32U2086_IMAGE_BASE EQUAL 0x30000000)
    set(_suffix "_xip")
  endif()
  set(_payload "$<TARGET_FILE:${target}>${_suffix}.bin")
  set(_image "$<TARGET_FILE_DIR:${target}>/${target}_image${_suffix}.bin")
  set_property(TARGET ${target} PROPERTY NEON_MDF_GT32U2086_IMAGE_FILE "${_image}")
  set(_builder "${NEON_MDF_PATH}/tools/gt32u2086_image.py")
  set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS
    "${_builder}"
    "${NEON_MDF_PATH}/mcu-description/generated/GT32U2086_image.cmake")
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:${target}> "${_payload}"
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target}>
    COMMAND ${Python3_EXECUTABLE} "${_builder}" image
            --input "${_payload}"
            --output "${_image}"
            --flash-addr ${NEON_MDF_GT32U2086_IMAGE_BASE}
            --elf $<TARGET_FILE:${target}> --objdump ${CMAKE_OBJDUMP}
            --app-version ${NEON_MDF_GT32U2086_APP_VERSION}
    COMMENT "${target}: package GT32U2086 header + payload"
    VERBATIM
  )
  neon_mdf_add_gt32u2086_flash_targets(${target})
endfunction()
