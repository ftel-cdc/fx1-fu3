# Wire Kconfig (use kconfiglib, the reusable ESP-IDF/Zephyr library, not parser) to build - same as
# ESP-IDF's sdkconfig/menuconfig. See ARCHITECTURE.md section 11.
#
# neon_mdf_process_kconfig(<APP_DIR> [<KCONFIG_FILE>])
#   APP_DIR - app directory (where the `sdkconfig` file lives, version-control by app - identical to
#                  ESP-IDF, NOT in neon-mdf/). KCONFIG_FILE - (optional) original Kconfig file used
#                  for this app. Defaults to ${NEON_MDF_PATH}/Kconfig (mainmenu of SDK, e.g. target
#                  FU2 uses components/driver). Passes separately when the app has ITS OWN Kconfig,
#                  independent of components/driver.
#
# MENUCONFIG combined (SDK+app, 2026-08-10): neon-mdf/Kconfig with dynamic cuoi
# `osource "$(NEON_MDF_APP_KCONFIG)"` uses an optional source, like
# Make's `-include` - no error if the file does not exist, checked with kconfiglib before using. This
# This function always sets NEON_MDF_APP_KCONFIG=${APP_DIR}/Kconfig in the environment before calling
# kconfiglib (all times configure when opening menuconfig) - if the app has its own
# <APP_DIR>/Kconfig file no, no will be automatically aggregated with 1 menuconfig/1 sdkconfig only
# with SDK options (components/driver/Kconfig), no additional declaration is required, no separate
# KCONFIG_FILE tracing is needed. App that does not have that file osource lang le ignored (no
# error) - old behavior retains 100%. applied to all CLI (neon_mdf.py) times VS Code extension since
# both 2 only call this CMake target `menuconfig`, no other unique logic at UI level.
#
# After calling:
#   - target `neon_mdf_kconfig` (INTERFACE) - include dir containing sdkconfig.h, link to any target that
#     requires #ifdef CONFIG_XXX in C code.
#   - CONFIG_<NAME> variable (ON/OFF/value) is in the scope calling this function - use if(CONFIG_...) in
#     CMakeLists.txt to enable/disable source files.
#   - custom target `menuconfig` - run `cmake --build <dir> --target menuconfig` to open sdkconfig
#     interactive UI (same as `idf.py menuconfig`).
function(neon_mdf_process_kconfig APP_DIR)
  # Set NOW from the beginning (before every time kconfiglib is run) - see osource in neon-mdf/Kconfig.
  # set(ENV{...}) affects only the current CMake process and its execute_process() children -
  # target menuconfig runs first in the program every build, so it must be passed back to `-E env`
  # separately, rather than through set(ENV{...}) here.
  set(ENV{NEON_MDF_APP_KCONFIG} ${APP_DIR}/Kconfig)
  find_program(NEON_MDF_PYTHON3 python3 REQUIRED)

  execute_process(
    COMMAND ${NEON_MDF_PYTHON3} -c "import kconfiglib"
    RESULT_VARIABLE _kconfiglib_missing
    OUTPUT_QUIET ERROR_QUIET
  )
  if(NOT _kconfiglib_missing EQUAL 0)
    message(FATAL_ERROR "Missing kconfiglib for Kconfig - run: pip3 install kconfiglib")
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
    message(FATAL_ERROR "create failed sdkconfig.h:\n${_gen_output}${_gen_error}")
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
      COMMENT "open menuconfig - save to ${_sdkconfig_file} (run cmake configure again after saving to apply)"
    )
  endif()

  # Propagate each CONFIG_* variable to PARENT_SCOPE of this function - this file is called from
  # neon-mdf/CMakeLists.txt. Functions have their own scope, so PARENT_SCOPE is required for
  # neon-mdf/CMakeLists.txt reads CONFIG_* immediately after calling).
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
