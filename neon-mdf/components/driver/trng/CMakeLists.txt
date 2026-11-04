if(CONFIG_NEON_MDF_DRIVER_TRNG)
  neon_mdf_driver_select_family_source(TARGET neon_mdf_driver PERIPHERAL trng
    SOURCES FU2 ${CMAKE_CURRENT_LIST_DIR}/platform/fu2/neon_trng.c)
endif()
