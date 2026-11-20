# Generic PHY interface, after the Linux generic PHY framework (drivers/phy/).
#   neon_phy.h, neon_phy_mipi_dphy.h      public API
#   neon_phy.c                            call order and reference counts, the same for every PHY and chip (phy-core.c)
#   neon_phy_mipi_dphy.c                  D-PHY defaults and the checks that do not depend on the PHY (phy-core-mipi-dphy.c)
#   neon_phy_platform.h                   internal: what a family supplies, the table of its PHYs
#   platform/<family>/neon_phy_<family>.c the table of the family's PHYs, by name
#   platform/<family>/<phy>.c             one driver per PHY: FX1 csi-dphy0 is fx1_csidphy.c. Its registers are
#                                         written only when FX1_CSIDPHY_BASE is defined (draft map, no address yet)
# No Kconfig symbol of its own: built for its clients. Today the only one is the CSI-2 receiver.
if(CONFIG_NEON_MDF_DRIVER_CSI)
  # The core is the same for every family.
  target_sources(neon_mdf_driver PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/neon_phy.c
    ${CMAKE_CURRENT_LIST_DIR}/neon_phy_mipi_dphy.c)
  neon_mdf_driver_select_family_source(
    REQUIRED TARGET neon_mdf_driver PERIPHERAL phy
    SOURCES FX1 ${CMAKE_CURRENT_LIST_DIR}/platform/fx1/neon_phy_fx1.c)
  neon_mdf_driver_select_family_source(
    REQUIRED TARGET neon_mdf_driver PERIPHERAL phy
    SOURCES FX1 ${CMAKE_CURRENT_LIST_DIR}/platform/fx1/fx1_csidphy.c)
endif()
