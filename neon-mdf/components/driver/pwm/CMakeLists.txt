if(CONFIG_NEON_MDF_DRIVER_PWM)
  neon_mdf_driver_select_family_source(TARGET neon_mdf_driver PERIPHERAL pwm
    SOURCES FU2 ${CMAKE_CURRENT_LIST_DIR}/platform/fu2/neon_pwm.c)
endif()
