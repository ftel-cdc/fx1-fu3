# Helpers used by the CMakeLists.txt of each peripheral. Each mapping consists of a pair of FAMILY
# SOURCE; source should be the absolute path from the CMAKE_CURRENT_LIST_DIR of the peripheral so
# that it does not depend on the CMake scope.
function(neon_mdf_driver_select_family_source)
  cmake_parse_arguments(ARG "REQUIRED" "TARGET;PERIPHERAL" "SOURCES" ${ARGN})

  if(NOT ARG_TARGET OR NOT ARG_PERIPHERAL OR NOT ARG_SOURCES)
    message(FATAL_ERROR "neon_mdf_driver_select_family_source: need TARGET,PERIPHERAL and SOURCES")
  endif()

  list(LENGTH ARG_SOURCES _neon_mdf_mapping_length)
  math(EXPR _neon_mdf_mapping_remainder "${_neon_mdf_mapping_length} % 2")
  if(_neon_mdf_mapping_remainder)
    message(FATAL_ERROR "${ARG_PERIPHERAL}: SOURCES must include FAMILY SOURCE pairs")
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
      "components/driver/${ARG_PERIPHERAL}: no implementation for family '${_neon_mdf_family}'")
  endif()
endfunction()
