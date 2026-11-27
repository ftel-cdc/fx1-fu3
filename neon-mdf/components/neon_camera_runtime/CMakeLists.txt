if(CONFIG_NEON_MDF_CAMERA_RUNTIME)
  add_library(neon_mdf_camera_runtime STATIC
    src/camera_runtime.c src/camera_config.c src/camera_events.c
    src/camera_scheduler.c src/camera_buffers.c src/camera_history.c
    src/camera_recovery.c)
  add_library(neon_mdf::camera_runtime ALIAS neon_mdf_camera_runtime)
  target_include_directories(neon_mdf_camera_runtime
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
  target_link_libraries(neon_mdf_camera_runtime PUBLIC neon_mdf_driver neon_mdf_kconfig)
  target_compile_features(neon_mdf_camera_runtime PUBLIC c_std_11)
  # Compatibility path is explicit. It is never used by the profile session.
  if(CONFIG_NEON_MDF_CAMERA_LEGACY)
    target_sources(neon_mdf_camera_runtime PRIVATE
      src/camera_legacy.c src/camera_legacy_config.c
      sensors/neon_camera_sensor_hal.c algorithms/camera_3a_reference.c)
    if(CONFIG_NEON_MDF_CAMERA_SENSOR_IMX675)
      target_sources(neon_mdf_camera_runtime PRIVATE sensors/neon_camera_sensor_imx675.c)
    elseif(CONFIG_NEON_MDF_CAMERA_SENSOR_OS04C10)
      target_sources(neon_mdf_camera_runtime PRIVATE sensors/neon_camera_sensor_os04c10.c)
    endif()
  endif()
endif()
