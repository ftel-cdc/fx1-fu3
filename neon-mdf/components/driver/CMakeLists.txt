# Driver arch-agnostic: thao tác qua register struct sinh từ mcu-description.
# Mỗi peripheral tự chọn backend trong <peripheral>/CMakeLists.txt; target,
# include path và dependency dùng chung được giữ tại đây.

add_library(neon_mdf_driver STATIC)

target_include_directories(neon_mdf_driver PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Lấy CMSIS/device headers, cờ kiến trúc và các CONFIG_NEON_MDF_DRIVER_*.
target_link_libraries(neon_mdf_driver PUBLIC ${NEON_MDF_DEVICE_TARGET} neon_mdf_kconfig)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/driver_helpers.cmake)

foreach(_neon_mdf_driver_peripheral
    crm syscsr gpio flash uart i2c crc efuse adc dma spi pwm tim rtc trng aes qspi wdt)
  add_subdirectory(${_neon_mdf_driver_peripheral})
endforeach()
