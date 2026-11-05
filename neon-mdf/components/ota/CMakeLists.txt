# components/ota - only be add_subdirectory() by cmake/flashmap.cmake (neon_mdf_process_flashmap())
# when the app has flashmap.csv - no automatic timer added for each bootloader-enabled app (other
# than components/bootloader); see neon_ota.h for the design rationale.
#
# Link both neon_mdf_driver (flash) times neon_mdf_crc32 (separated from
# components/bootloader/CMakeLists.txt, see comment there) times neon_mdf_flashmap (macro dia only
# otadata) - used by both bootloader executable times app executable, not glued according to the
# bootloader's main().
add_library(neon_mdf_ota STATIC neon_ota.c)
target_include_directories(neon_mdf_ota PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(neon_mdf_ota PUBLIC neon_mdf_driver neon_mdf_crc32 neon_mdf_flashmap)
