include_guard(GLOBAL)

# UART settings are read by the Python tool at execution time, including
# NEON_MDF_SERIAL_PORT, NEON_MDF_SERIAL_BAUD, NEON_MDF_UART_BYTE_GAP_MS and
# NEON_MDF_UART_LOG. Do not expand these environment values at configure time.
function(neon_mdf_add_gt32u2086_flash_targets target)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(_tool "${NEON_MDF_PATH}/tools/gt32u2086_flash.py")
  get_target_property(_image ${target} NEON_MDF_GT32U2086_IMAGE_FILE)
  if(NEON_MDF_GT32U2086_IMAGE_BASE EQUAL 0x30000000)
    set(_profile xip)
  else()
    set(_profile normal)
  endif()

  add_custom_target(${target}_flash_uart_gt32u2086
    COMMAND ${Python3_EXECUTABLE} "${_tool}"
            --image "${_image}"
            --profile ${_profile}
    DEPENDS ${target}
    USES_TERMINAL
    VERBATIM
    COMMENT "${target}: GT32U2086 ROMCODE UART update (select boot mode 100, then press RESET)"
  )
  # The first packaged application is the default; multi-image projects can
  # explicitly select <application>_flash_uart_gt32u2086 instead.
  if(NOT TARGET flash_uart_gt32u2086)
    add_custom_target(flash_uart_gt32u2086 DEPENDS ${target}_flash_uart_gt32u2086)
  endif()
  if(NOT TARGET serial_monitor)
    add_custom_target(serial_monitor
      COMMAND ${Python3_EXECUTABLE} "${_tool}" --monitor-only
      USES_TERMINAL
      VERBATIM
      COMMENT "GT32U2086 UART monitor (NEON_MDF_SERIAL_PORT / NEON_MDF_MONITOR_BAUD)"
    )
  endif()
endfunction()
