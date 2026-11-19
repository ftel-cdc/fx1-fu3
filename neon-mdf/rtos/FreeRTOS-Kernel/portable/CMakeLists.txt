# NEON-MDF subset: the GCC Cortex-M0 port used by GT32U2086.
add_library(freertos_kernel_port OBJECT
  GCC/ARM_CM0/port.c
  GCC/ARM_CM0/portasm.c
  GCC/ARM_CM0/mpu_wrappers_v2_asm.c)
add_library(freertos_kernel_port_headers INTERFACE)
target_include_directories(freertos_kernel_port_headers INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/GCC/ARM_CM0)
target_link_libraries(freertos_kernel_port
  PUBLIC freertos_kernel_port_headers
  PRIVATE freertos_kernel_include)
