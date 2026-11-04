if(CONFIG_NEON_MDF_DRIVER_SPI)
  neon_mdf_driver_select_family_source(TARGET neon_mdf_driver PERIPHERAL spi
    SOURCES FU2 ${CMAKE_CURRENT_LIST_DIR}/platform/fu2/neon_spi.c)
endif()
