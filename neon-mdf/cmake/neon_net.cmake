# Network manager (components/neon_net, lwIP on FreeRTOS). Call after neon_mdf_add_freertos();
# the application then links neon_mdf::net. Like the ISP service, an OS-dependent component is
# added by the application instead of the SDK's add_subdirectory(components/...).
function(neon_mdf_add_net)
  if(TARGET neon_mdf::net)
    return()
  endif()
  if(NOT TARGET neon_mdf::freertos)
    message(FATAL_ERROR "Call neon_mdf_add_freertos() before neon_mdf_add_net()")
  endif()
  if(NOT NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "FX1")
    message(FATAL_ERROR "neon_mdf_add_net: the network manager is implemented for FX1 only")
  endif()
  include(${CMAKE_BINARY_DIR}/config/sdkconfig.cmake)
  if(NOT CONFIG_NEON_MDF_NET)
    message(FATAL_ERROR "neon_mdf_add_net: enable CONFIG_NEON_MDF_NET (menuconfig or sdkconfig)")
  endif()
  add_subdirectory("${NEON_MDF_PATH}/components/neon_net"
                   "${CMAKE_CURRENT_BINARY_DIR}/neon-net")
endfunction()
