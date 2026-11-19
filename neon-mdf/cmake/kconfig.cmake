# Integrate SDK and application Kconfig with kconfiglib; see guide/BuildSystem.md.
#
# neon_mdf_process_kconfig(<APP_DIR> [<KCONFIG_FILE>])
# APP_DIR owns sdkconfig and optional application Kconfig. KCONFIG_FILE defaults
# to ${NEON_MDF_PATH}/Kconfig. Its optional osource directive includes the
# application Kconfig through NEON_MDF_APP_KCONFIG without requiring that file.
# CLI and VS Code both invoke the same menuconfig target.
#
# Provides neon_mdf_kconfig (the generated header include directory), CONFIG_*
# variables in the caller's scope, and the menuconfig custom target. Reconfigure
# after saving menuconfig settings to apply changes to the build.
function(neon_mdf_process_kconfig APP_DIR)
  # Configure-time subprocesses inherit this environment. Pass the variable
  # separately to the menuconfig build command, which runs in a new process.
  set(ENV{NEON_MDF_APP_KCONFIG} ${APP_DIR}/Kconfig)
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  set(NEON_MDF_PYTHON3 ${Python3_EXECUTABLE})

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

  # Functions have their own scope; expose CONFIG_* to neon-mdf/CMakeLists.txt.
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
