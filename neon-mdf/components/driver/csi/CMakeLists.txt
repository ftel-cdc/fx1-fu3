# CSI-2 receiver HAL. The header outside platform/ is the public API.
#   platform/common/neon_csi.c   every rule of the module, the same for every chip; it calls only the
#                                  neon_csi_hw_*() contract of platform/common/neon_csi_hw.h
#   platform/<family>/             what a chip adds: fx1_csi_caps.c (its limits) and exactly one backend,
#                                  fx1_csi_simulation.c or the future register backend
# A new chip adds its own platform/<family>/ and one pair per list below; the common file is listed for it too.
if(CONFIG_NEON_MDF_DRIVER_CSI)
  neon_mdf_driver_select_family_source(
    REQUIRED TARGET neon_mdf_driver PERIPHERAL csi
    SOURCES FX1 ${CMAKE_CURRENT_LIST_DIR}/platform/common/neon_csi.c)
  neon_mdf_driver_select_family_source(
    REQUIRED TARGET neon_mdf_driver PERIPHERAL csi
    SOURCES FX1 ${CMAKE_CURRENT_LIST_DIR}/platform/fx1/fx1_csi_caps.c)
  if(CONFIG_NEON_MDF_CAMERA_SIMULATION)
    neon_mdf_driver_select_family_source(
      REQUIRED TARGET neon_mdf_driver PERIPHERAL csi
      SOURCES FX1 ${CMAKE_CURRENT_LIST_DIR}/platform/fx1/fx1_csi_simulation.c)
  else()
    message(FATAL_ERROR
      "components/driver/csi: the FX1 register backend does not exist yet. It needs the register "
      "workbook FTEL_MEMORY_MAP_REGISTERS (v1.4 or later), including the DMA and metadata chapters. Enable NEON_MDF_CAMERA_SIMULATION.")
  endif()
endif()
