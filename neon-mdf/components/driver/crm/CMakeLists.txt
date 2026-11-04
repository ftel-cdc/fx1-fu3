# CRM is the platform clock/reset infrastructure of FU2 should always be compiled for this family, not
# put after a Kconfig option.
neon_mdf_driver_select_family_source(
  TARGET neon_mdf_driver
  PERIPHERAL crm
  SOURCES FU2 ${CMAKE_CURRENT_LIST_DIR}/platform/fu2/neon_crm.c
)
