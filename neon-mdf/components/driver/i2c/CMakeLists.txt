if(CONFIG_NEON_MDF_DRIVER_I2C)
  neon_mdf_driver_select_family_source(
    TARGET neon_mdf_driver PERIPHERAL i2c
    SOURCES FU2 ${CMAKE_CURRENT_LIST_DIR}/platform/fu2/neon_i2c.c
  )
endif()
