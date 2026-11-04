# Wire Kconfig (dùng kconfiglib - thư viện ESP-IDF/Zephyr cũng dùng lại,
# không tự viết parser) vào build - giống hệ sdkconfig/menuconfig của
# ESP-IDF. Xem ARCHITECTURE.md mục 11.
#
# neon_mdf_process_kconfig(<APP_DIR> [<KCONFIG_FILE>])
#   APP_DIR      - thư mục app (nơi file `sdkconfig` sống, version-control
#                  theo từng app - giống hệt ESP-IDF, KHÔNG nằm trong neon-mdf/).
#   KCONFIG_FILE - (tuỳ chọn) file Kconfig gốc dùng cho app này. Mặc định
#                  ${NEON_MDF_PATH}/Kconfig (mainmenu của SDK, ví dụ target FU2
#                  dùng components/driver). Truyền riêng khi app có Kconfig CỦA
#                  RIÊNG NÓ, độc lập với components/driver.
#
# MENUCONFIG KET HOP (SDK + app, 2026-08-10): neon-mdf/Kconfig có dong cuoi
# `osource "$(NEON_MDF_APP_KCONFIG)"` (osource = "optional source", giong
# `-include` cua Make - KHONG loi neu file khong ton tai, da kiem chung that
# voi kconfiglib truoc khi dung). Ham nay LUON set bien moi truong
# NEON_MDF_APP_KCONFIG=${APP_DIR}/Kconfig truoc khi goi kconfiglib (ca luc
# configure lan luc mo menuconfig) - NEU app co file <APP_DIR>/Kconfig cua
# rieng no, no se TU DONG duoc gop chung vao 1 menuconfig/1 sdkconfig DUY
# NHAT voi cac option cua SDK (components/driver/Kconfig), khong can khai
# bao gi them, khong can truyen KCONFIG_FILE rieng. App KHONG co file do thi
# osource lang le bo qua (khong loi) - hanh vi cu giu nguyen 100%. Ap dung
# CHUNG cho ca CLI (neon_mdf.py) lan VS Code extension vi ca 2 deu chi goi
# target CMake `menuconfig` nay, khong co logic rieng nao khac o tang UI.
#
# Sau khi gọi:
#   - target `neon_mdf_kconfig` (INTERFACE) - include dir chứa sdkconfig.h,
#     link vào target nào cần #ifdef CONFIG_XXX trong code C.
#   - biến CONFIG_<NAME> (ON/OFF/giá trị) nằm trong scope gọi hàm này -
#     dùng if(CONFIG_...) trong CMakeLists.txt để bật/tắt source file.
#   - custom target `menuconfig` - chạy `cmake --build <dir> --target menuconfig`
#     để mở UI chỉnh sdkconfig tương tác (giống `idf.py menuconfig`).
function(neon_mdf_process_kconfig APP_DIR)
  # Set NGAY TU DAU (truoc moi lan kconfiglib chay) - xem osource o
  # neon-mdf/Kconfig. set(ENV{...}) o day chi anh huong tien trinh CMake
  # HIEN TAI (execute_process() ke thua duoc) - target menuconfig chay o
  # tien trinh MOI luc build nen phai tu truyen lai bien nay qua `-E env`
  # rieng, khong dua vao set(ENV{...}) o day.
  set(ENV{NEON_MDF_APP_KCONFIG} ${APP_DIR}/Kconfig)
  find_program(NEON_MDF_PYTHON3 python3 REQUIRED)

  execute_process(
    COMMAND ${NEON_MDF_PYTHON3} -c "import kconfiglib"
    RESULT_VARIABLE _kconfiglib_missing
    OUTPUT_QUIET ERROR_QUIET
  )
  if(NOT _kconfiglib_missing EQUAL 0)
    message(FATAL_ERROR "Thiếu kconfiglib cho Kconfig - chạy: pip3 install kconfiglib")
  endif()

  if(ARGC GREATER 1)
    set(_kconfig_file ${ARGV1})
  else()
    set(_kconfig_file ${NEON_MDF_PATH}/Kconfig)
  endif()
  get_filename_component(_kconfig_dir ${_kconfig_file} DIRECTORY)
  set(_sdkconfig_file ${APP_DIR}/sdkconfig)
  set(_config_dir ${CMAKE_BINARY_DIR}/config)
  set(_header_out ${_config_dir}/sdkconfig.h)
  set(_cmake_out ${_config_dir}/sdkconfig.cmake)

  execute_process(
    COMMAND ${NEON_MDF_PYTHON3} ${NEON_MDF_PATH}/tools/kconfig/gen_config_header.py
            --kconfig ${_kconfig_file}
            --sdkconfig ${_sdkconfig_file}
            --header-out ${_header_out}
            --cmake-out ${_cmake_out}
    RESULT_VARIABLE _gen_result
    OUTPUT_VARIABLE _gen_output
    ERROR_VARIABLE _gen_error
  )
  message(STATUS "[kconfig] ${_gen_output}")
  if(NOT _gen_result EQUAL 0)
    message(FATAL_ERROR "Sinh sdkconfig.h thất bại:\n${_gen_output}${_gen_error}")
  endif()

  include(${_cmake_out})

  if(NOT TARGET neon_mdf_kconfig)
    add_library(neon_mdf_kconfig INTERFACE)
    target_include_directories(neon_mdf_kconfig INTERFACE ${_config_dir})
  endif()

  if(NOT TARGET menuconfig)
    add_custom_target(menuconfig
      COMMAND ${CMAKE_COMMAND} -E env KCONFIG_CONFIG=${_sdkconfig_file} NEON_MDF_APP_KCONFIG=${APP_DIR}/Kconfig ${NEON_MDF_PYTHON3} -m menuconfig ${_kconfig_file}
      WORKING_DIRECTORY ${_kconfig_dir}
      USES_TERMINAL
      COMMENT "Mo menuconfig - luu vao ${_sdkconfig_file} (chay lai cmake configure sau khi luu de ap dung)"
    )
  endif()

  # Propagate moi bien CONFIG_* len PARENT_SCOPE cua ham nay - file nay duoc
  # goi tu neon-mdf/CMakeLists.txt (function co scope rieng, luon can
  # PARENT_SCOPE de neon-mdf/CMakeLists.txt doc duoc CONFIG_* ngay sau khi goi).
  execute_process(
    COMMAND ${NEON_MDF_PYTHON3} -c "
import re
names = []
with open('${_cmake_out}') as f:
    for line in f:
        m = re.match(r'set\\((CONFIG_[A-Za-z0-9_]+)', line)
        if m:
            names.append(m.group(1))
print(';'.join(names))
"
    OUTPUT_VARIABLE _config_names
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  foreach(_name ${_config_names})
    set(${_name} ${${_name}} PARENT_SCOPE)
  endforeach()
endfunction()
