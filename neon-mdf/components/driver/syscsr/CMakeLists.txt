# SYSCSR là hạ tầng hệ thống riêng của FU2 nên luôn được biên dịch cho family
# FU2; family khác không có backend này và không thêm source.
neon_mdf_driver_select_family_source(
  TARGET neon_mdf_driver
  PERIPHERAL syscsr
  SOURCES FU2 ${CMAKE_CURRENT_LIST_DIR}/platform/fu2/neon_syscsr.c
)
