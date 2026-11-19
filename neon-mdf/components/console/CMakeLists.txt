# Preserve the optional public include "console/neon_console.h" after the move.
target_include_directories(neon_mdf_driver PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)

if(NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "GT32U2086" AND
   CONFIG_NEON_MDF_DRIVER_UART)
  # UART enables the complete printf/_write path; no separate printf flag.
  target_sources(neon_mdf_driver PRIVATE ${CMAKE_CURRENT_LIST_DIR}/platform/gt32u2086/neon_console.c)
endif()
