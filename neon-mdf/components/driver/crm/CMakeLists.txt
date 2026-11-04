# CRM là hạ tầng clock/reset nền tảng của FU2 nên luôn được biên dịch cho
# family này, không đặt sau một Kconfig tùy chọn.
neon_mdf_driver_select_family_source(
  TARGET neon_mdf_driver
  PERIPHERAL crm
  SOURCES FU2 ${CMAKE_CURRENT_LIST_DIR}/platform/fu2/neon_crm.c
)
