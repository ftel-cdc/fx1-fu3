# Helpers dùng bởi CMakeLists.txt của từng peripheral. Mỗi mapping gồm một cặp
# FAMILY SOURCE; source nên là đường dẫn tuyệt đối từ CMAKE_CURRENT_LIST_DIR
# của peripheral để không phụ thuộc scope CMake.
function(neon_mdf_driver_select_family_source)
  cmake_parse_arguments(ARG "REQUIRED" "TARGET;PERIPHERAL" "SOURCES" ${ARGN})

  if(NOT ARG_TARGET OR NOT ARG_PERIPHERAL OR NOT ARG_SOURCES)
    message(FATAL_ERROR "neon_mdf_driver_select_family_source: cần TARGET, PERIPHERAL và SOURCES")
  endif()

  list(LENGTH ARG_SOURCES _neon_mdf_mapping_length)
  math(EXPR _neon_mdf_mapping_remainder "${_neon_mdf_mapping_length} % 2")
  if(_neon_mdf_mapping_remainder)
    message(FATAL_ERROR "${ARG_PERIPHERAL}: SOURCES phải gồm các cặp FAMILY SOURCE")
  endif()

  set(_neon_mdf_family "${NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY}")
  math(EXPR _neon_mdf_last_index "${_neon_mdf_mapping_length} - 2")
  foreach(_neon_mdf_index RANGE 0 ${_neon_mdf_last_index} 2)
    list(GET ARG_SOURCES ${_neon_mdf_index} _neon_mdf_candidate_family)
    math(EXPR _neon_mdf_source_index "${_neon_mdf_index} + 1")
    list(GET ARG_SOURCES ${_neon_mdf_source_index} _neon_mdf_candidate_source)
    if(_neon_mdf_family STREQUAL _neon_mdf_candidate_family)
      target_sources(${ARG_TARGET} PRIVATE ${_neon_mdf_candidate_source})
      return()
    endif()
  endforeach()

  if(ARG_REQUIRED)
    message(FATAL_ERROR
      "components/driver/${ARG_PERIPHERAL}: chưa có implementation cho family '${_neon_mdf_family}'")
  endif()
endfunction()
