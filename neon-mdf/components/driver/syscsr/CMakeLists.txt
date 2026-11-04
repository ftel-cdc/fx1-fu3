# SYSCSR is FU2's own system infrastructure and should always be compiled for the FU2 family; other
# families do not have this backend and do not add sources.
neon_mdf_driver_select_family_source(
  TARGET neon_mdf_driver
  PERIPHERAL syscsr
  SOURCES FU2 ${CMAKE_CURRENT_LIST_DIR}/platform/fu2/neon_syscsr.c
)
