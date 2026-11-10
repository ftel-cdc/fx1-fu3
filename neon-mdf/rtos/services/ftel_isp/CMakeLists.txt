# Tích hợp driver ISP với FreeRTOS: owner task, IRQ, ticket và recovery.
set(_isp_root "${NEON_MDF_PATH}/components/ftel_isp")
add_library(ftel_isp_freertos STATIC ftel_isp_freertos.c)
add_library(neon_mdf::ftel_isp_freertos ALIAS ftel_isp_freertos)
target_include_directories(ftel_isp_freertos
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
  PRIVATE "${_isp_root}" "${_isp_root}/pipeline")
target_link_libraries(ftel_isp_freertos PUBLIC neon_mdf::ftel_isp neon_mdf::freertos)
target_compile_features(ftel_isp_freertos PUBLIC c_std_11)
