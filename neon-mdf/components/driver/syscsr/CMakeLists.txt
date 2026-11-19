# SYSCSR is GT32U2086's own system infrastructure and should always be compiled for the GT32U2086 family; other
# families do not have this backend and do not add sources.
neon_mdf_driver_select_family_source(
  TARGET neon_mdf_driver
  PERIPHERAL syscsr
  SOURCES GT32U2086 ${CMAKE_CURRENT_LIST_DIR}/platform/gt32u2086/neon_syscsr.c
)
