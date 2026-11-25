cmake_minimum_required(VERSION 3.21)
project(isp_bayer_ae C ASM)

get_filename_component(NEON_MDF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../neon-mdf" ABSOLUTE)
if(NOT NEON_MDF_TARGET STREQUAL "FX1")
  message(FATAL_ERROR "Dung cmake --preset FX1 cho bai bring-up ISP nay")
endif()
set(NEON_MDF_ENABLE_FTEL_ISP ON)
include(${NEON_MDF_PATH}/tools/cmake/project.cmake)
include(${NEON_MDF_PATH}/cmake/freertos.cmake)
neon_mdf_add_freertos(FX1 ${CMAKE_CURRENT_SOURCE_DIR}/Inc)

neon_mdf_add_executable(isp_bayer_ae Src/main.c)
target_link_libraries(isp_bayer_ae PRIVATE neon_mdf::ftel_isp neon_mdf::freertos)
# Regmap chỉ dùng cho các kiểm tra readback trong chương trình bring-up.
target_include_directories(isp_bayer_ae PRIVATE ${NEON_MDF_PATH}/components/driver/isp)
set_target_properties(isp_bayer_ae PROPERTIES SUFFIX ".elf")
# Runner VP tùy chọn; configure/build ELF không cần repo/binary VP.
include(${NEON_MDF_PATH}/cmake/runners/fx1-vp.cmake)
add_custom_target(run-vp
  COMMAND ${CMAKE_COMMAND}
    -DVP=${NEON_FX1_VP_EXECUTABLE} -DELF=$<TARGET_FILE:isp_bayer_ae>
    -DLOG=${CMAKE_CURRENT_BINARY_DIR}/platform.log
    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_vp.cmake
  DEPENDS isp_bayer_ae USES_TERMINAL VERBATIM)

# Bước 1–3: executable không liên kết FreeRTOS, dùng cùng target MCU/component.
neon_mdf_add_executable(isp_init_baremetal Src/main_baremetal.c)
target_link_libraries(isp_init_baremetal PRIVATE neon_mdf::ftel_isp)
set_target_properties(isp_init_baremetal PROPERTIES SUFFIX ".elf")
add_custom_target(run-init-vp
  COMMAND ${CMAKE_COMMAND}
    -DVP=${NEON_FX1_VP_EXECUTABLE} -DELF=$<TARGET_FILE:isp_init_baremetal>
    -DLOG=${CMAKE_CURRENT_BINARY_DIR}/baremetal_init.log -DEXPECT=BAREMETAL_INIT
    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_vp.cmake
  DEPENDS isp_init_baremetal USES_TERMINAL VERBATIM)

# Bước 4–5: một frame sau mỗi init, thống kê AEC; tách bài init để dễ chốt từng mốc.
neon_mdf_add_executable(isp_bayer_aec_frame Src/frame.c)
target_link_libraries(isp_bayer_aec_frame PRIVATE neon_mdf::ftel_isp neon_mdf::freertos)
set_target_properties(isp_bayer_aec_frame PROPERTIES SUFFIX ".elf")
add_custom_target(run-frame-vp
  COMMAND ${CMAKE_COMMAND}
    -DVP=${NEON_FX1_VP_EXECUTABLE} -DELF=$<TARGET_FILE:isp_bayer_aec_frame>
    -DLOG=${CMAKE_CURRENT_BINARY_DIR}/frame_aec.log -DEXPECT=FRAME_AEC
    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_vp.cmake
  DEPENDS isp_bayer_aec_frame USES_TERMINAL VERBATIM)

# Service FreeRTOS: một owner, IRQ/semaphore, ticket và recovery.
neon_mdf_add_isp_freertos()
neon_mdf_add_executable(isp_freertos_service Src/service.c)
target_link_libraries(isp_freertos_service PRIVATE neon_mdf::ftel_isp_freertos)
set_target_properties(isp_freertos_service PROPERTIES SUFFIX ".elf")
# Chỉ test này trì hoãn xác nhận recovery để kiểm chứng giữ buffer khi faulted.
target_link_options(isp_freertos_service PRIVATE -Wl,--wrap=ftel_isp_fx1_recovery_poll)
add_custom_target(run-service-vp
  COMMAND ${CMAKE_COMMAND}
    -DVP=${NEON_FX1_VP_EXECUTABLE} -DELF=$<TARGET_FILE:isp_freertos_service>
    -DLOG=${CMAKE_CURRENT_BINARY_DIR}/service.log -DEXPECT=ISP_FREERTOS_SERVICE -DSIM_TIMEOUT_MS=2500
    -P ${CMAKE_CURRENT_SOURCE_DIR}/run_vp.cmake
  DEPENDS isp_freertos_service USES_TERMINAL VERBATIM)
