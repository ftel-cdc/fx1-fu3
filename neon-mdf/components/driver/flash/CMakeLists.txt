# FU2 uses HAL Flash both for the controller/bad-sector configuration, and not just for the
# bootloader. Older families still only drag the backend when the bootloader is enabled.
if(NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "FU2")
  target_sources(neon_mdf_driver PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/platform/fu2/neon_flash.c
  )
elseif(NEON_MDF_BOOTLOADER_ENABLED)
  neon_mdf_driver_select_family_source(
    TARGET neon_mdf_driver
    PERIPHERAL flash
    REQUIRED
    SOURCES
      WEACT_F412 ${CMAKE_CURRENT_LIST_DIR}/platform/weact_f412/neon_flash.c
      STM32F103 ${CMAKE_CURRENT_LIST_DIR}/platform/stm32f103/neon_flash.c
      CH32V003  ${CMAKE_CURRENT_LIST_DIR}/platform/ch32v003/neon_flash.c
  )
endif()
