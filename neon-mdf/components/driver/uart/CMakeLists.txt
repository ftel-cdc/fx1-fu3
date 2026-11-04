if(CONFIG_NEON_MDF_DRIVER_UART)
  neon_mdf_driver_select_family_source(
    TARGET neon_mdf_driver
    PERIPHERAL uart
    SOURCES
      FU2 ${CMAKE_CURRENT_LIST_DIR}/platform/fu2/neon_uart.c
      STM32F103 ${CMAKE_CURRENT_LIST_DIR}/platform/stm32f103/neon_uart.c
  )
endif()
