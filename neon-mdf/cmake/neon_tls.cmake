# TLS component (components/neon_tls, mbedTLS 3.6.8 over lwIP sockets). Call after neon_mdf_add_net();
# the application then links neon_mdf::tls.
function(neon_mdf_add_tls)
  if(TARGET neon_mdf::tls)
    return()
  endif()
  if(NOT TARGET neon_mdf::net)
    message(FATAL_ERROR "Call neon_mdf_add_net() before neon_mdf_add_tls()")
  endif()
  include(${CMAKE_BINARY_DIR}/config/sdkconfig.cmake)
  if(NOT CONFIG_NEON_MDF_TLS)
    message(FATAL_ERROR "neon_mdf_add_tls: enable CONFIG_NEON_MDF_TLS (menuconfig or sdkconfig)")
  endif()
  add_subdirectory("${NEON_MDF_PATH}/components/neon_tls"
                   "${CMAKE_CURRENT_BINARY_DIR}/neon-tls")
endfunction()
