if(CONFIG_NEON_MDF_DRIVER_RTC)
  neon_mdf_driver_select_family_source(TARGET neon_mdf_driver PERIPHERAL rtc
    SOURCES FU2 ${CMAKE_CURRENT_LIST_DIR}/platform/fu2/neon_rtc.c)
endif()
