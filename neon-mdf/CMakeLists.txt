# SDK libraries; applications include tools/cmake/project.cmake.
include(${NEON_MDF_PATH}/cmake/mcu_variants.cmake)
neon_mdf_configure_target(${NEON_MDF_TARGET})
set(NEON_MDF_DEVICE_TARGET ${NEON_MDF_DEVICE_TARGET} PARENT_SCOPE)
set(NEON_MDF_LINKER_SCRIPT ${NEON_MDF_LINKER_SCRIPT} PARENT_SCOPE)
set(NEON_MDF_TARGET_ARCH cortex-m0 PARENT_SCOPE)

include(${NEON_MDF_PATH}/cmake/kconfig.cmake)
neon_mdf_process_kconfig(${CMAKE_SOURCE_DIR})
add_subdirectory(components/driver)
add_subdirectory(components/console)
