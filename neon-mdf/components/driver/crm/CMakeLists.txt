# CRM is the platform clock/reset infrastructure of GT32U2086 should always be compiled for this family, not
# put after a Kconfig option.
neon_mdf_driver_select_family_source(
  TARGET neon_mdf_driver
  PERIPHERAL crm
  SOURCES GT32U2086 ${CMAKE_CURRENT_LIST_DIR}/platform/gt32u2086/neon_crm.c
)
