if(NOT NEON_MDF_TARGET STREQUAL "FX1")
  message(FATAL_ERROR "ftel_isp currently requires the FX1 platform")
endif()
add_library(neon_mdf_ftel_isp STATIC
  "common/ftel_isp_common.c"
  "pipeline/ftel_isp_init.c"
  "pipeline/ftel_isp_3a.c"
  "pipeline/ftel_isp_stream.c"
  "bayer domain/ifmt/ftel_isp_ifmt.c"
  "bayer domain/blc/ftel_isp_blc.c"
  "bayer domain/lsc/ftel_isp_lsc.c"
  "bayer domain/bpc/ftel_isp_bpc.c"
  "bayer domain/wb/ftel_isp_wb.c"
  "bayer domain/dg/ftel_isp_dg.c"
  "rgb domain/demosaic/ftel_isp_demosaic.c"
  "rgb domain/ccm/ftel_isp_ccm.c"
  "rgb domain/gamma/ftel_isp_gamma.c"
  "rgb domain/csc/ftel_isp_csc.c"
  "yuv domain/gtm/ftel_isp_gtm.c"
  "yuv domain/nr/ftel_isp_nr.c"
  "yuv domain/ee/ftel_isp_ee.c"
  "yuv domain/cnf/ftel_isp_cnf.c"
  "stats/aec/ftel_isp_aec.c"
  "stats/awb/ftel_isp_awb.c"
  "stats/af/ftel_isp_af.c"
  "algorithms/ae/ftel_isp_ae.c"
  "io/output/ftel_isp_output.c"
  "io/dma/ftel_isp_dma.c"
)
target_compile_features(neon_mdf_ftel_isp PUBLIC c_std_11)
target_include_directories(neon_mdf_ftel_isp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(neon_mdf_ftel_isp PUBLIC ${NEON_MDF_DEVICE_TARGET} neon_mdf_kconfig)
target_compile_definitions(neon_mdf_ftel_isp PUBLIC
  ISP_BASE=FX1_ISP_CSR_BASE ISP_CHECK_RECOVERY_IDLE=1 COMMON_RECOVERY_STATUS_BUSY=1u)
target_compile_options(neon_mdf_ftel_isp PUBLIC -include fx1/fx1_memory_map.h)
