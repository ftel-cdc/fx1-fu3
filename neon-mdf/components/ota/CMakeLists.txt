# components/ota - CHI duoc add_subdirectory() boi
# cmake/flashmap.cmake (neon_mdf_process_flashmap()) khi app CO flashmap.csv -
# KHONG BAO GIO tu dong them cho moi app bootloader-enabled (khac
# components/bootloader) - xem neon_ota.h cho ly do thiet ke.
#
# Link CA neon_mdf_driver (flash) LAN neon_mdf_crc32 (tach rieng tu
# components/bootloader/CMakeLists.txt, xem comment o do) LAN neon_mdf_flashmap
# (macro dia chi otadata) - dung duoc boi CA bootloader executable LAN app
# executable, khong keo theo main() cua bootloader.
add_library(neon_mdf_ota STATIC neon_ota.c)
target_include_directories(neon_mdf_ota PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(neon_mdf_ota PUBLIC neon_mdf_driver neon_mdf_crc32 neon_mdf_flashmap)
