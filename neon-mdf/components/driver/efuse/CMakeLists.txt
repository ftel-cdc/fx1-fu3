if(CONFIG_NEON_MDF_DRIVER_EFUSE)
  neon_mdf_driver_select_family_source(TARGET neon_mdf_driver PERIPHERAL efuse
    SOURCES GT32U2086 ${CMAKE_CURRENT_LIST_DIR}/platform/gt32u2086/neon_efuse.c)
endif()
