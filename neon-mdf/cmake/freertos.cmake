# Wire FreeRTOS-Kernel (already vendor in rtos/FreeRTOS-Kernel) to build, using the official CMake
# system of the kernel (add_subdirectory
# + target `freertos_config`) see because manually list source file - avoid missing file when submodule
# update (eg mpu_wrappers_v2_asm.c which port ARM_CM0 need).
#
# neon_mdf_add_freertos(<TARGET_NAME> <CONFIG_DIR>)
#   TARGET_NAME - target registered in cmake/mcu_variants.cmake (taking arch to apply -mcpu/-mthumb to
#                 target freertos_kernel/_port, since these 2 targets do NOT link to
#                 neon_mdf_device_<X> so they do not inherit the architecture flag by themselves).
#                 CONFIG_DIR - folder containing FreeRTOSConfig.h of app (each app has its own file
#                 - see Projects/Examples/FreeRTOS_Blinky/Inc/).
#
# Called only once for the entire cmake configure (add_subdirectory kernel runs only once) - because
# each app is now a standalone CMake project (see tools/cmake/project.cmake), this is naturally
# true, no longer risking many FreeRTOS apps to build together once configured as before.
function(neon_mdf_add_freertos TARGET_NAME CONFIG_DIR)
  if(TARGET freertos_kernel)
    return()
  endif()

  if(NOT NEON_MDF_PATH)
    message(FATAL_ERROR "NEON_MDF_PATH not set before calling neon_mdf_add_freertos()")
  endif()

  set(_arch ${NEON_MDF_TARGET_${TARGET_NAME}_ARCH})
  if(NOT _arch)
    message(FATAL_ERROR "neon_mdf_add_freertos: target '${TARGET_NAME}' has not been periodized in cmake/mcu_variants.cmake")
  endif()

  # Dispatch port by arch - each port has its own portmacro.h/port.c file in
  # rtos/FreeRTOS-Kernel/portable/GCC/<PORT>/ already supplied upstream, without
  # to add a submodule). ARM_CM0 (FU2), ARM_CM4F (WEACT_F412, with an FPU -
  # see NEON_MDF_TARGET_<X>_FPU in mcu_variants.cmake) and GCC_RISC_V_GENERIC (CH32V003F4P6, add
  # 2026-08-10) are the ports that have this wire to me - adding other live target must be added
  # directly here.
  if(_arch MATCHES "^cortex-m0")
    set(_freertos_port GCC_ARM_CM0)
    set(_freertos_fpu OFF)
  elseif(_arch STREQUAL "cortex-m4")
    set(_freertos_port GCC_ARM_CM4F)
    set(_freertos_fpu ${NEON_MDF_TARGET_${TARGET_NAME}_FPU})
  elseif(_arch MATCHES "^riscv")
    # QingKe V2A (CH32V003) without CLINT (Core Local Interrupter of RISC-V) - real verified via QingKeV2
    # Processor Manual V1.0 (real from ch405-labs.com, compare this to mtvec/mcause/PFIC, do not
    # guess) - PFIC (vendor-specific) interrupt manager sees CLINT. FreeRTOS-Kernel RISC-V port
    # already supports this configuration through chip_specific_extensions
    # "RISCV_no_extensions" (portasmHAS_MTIME=0, portasmHAS_SIFIVE_CLINT=0 - see
    # rtos/FreeRTOS-Kernel/portable/GCC/RISC-V/chip_specific_extensions/RISCV_no_extensions/).
    # Real setup (mtvec Direct mode + STK create interrupt tick + PFIC IENR) male in
    # examples/FreeRTOS_Blinky/Src/main.c (vPortSetupTimerInterrupt()/
    # freertos_risc_v_application_interrupt_handler(), 2 "weak" functions of port.c/portASM.S that
    # the app overrides it; see the full comment there).
    set(_freertos_port GCC_RISC_V_GENERIC)
    set(FREERTOS_RISCV_EXTENSION RISCV_no_extensions CACHE STRING "" FORCE)
  else()
    message(FATAL_ERROR "neon_mdf_add_freertos: no wire port FreeRTOS for arch '${_arch}' (target '${TARGET_NAME}') - see rtos/FreeRTOS-Kernel/portable/GCC/ to select the matching port and add it quickly in cmake/freertos.cmake")
  endif()

  add_library(freertos_config INTERFACE)
  target_include_directories(freertos_config SYSTEM INTERFACE ${CONFIG_DIR})
  target_compile_definitions(freertos_config INTERFACE projCOVERAGE_TEST=0)
  # Non-FU2 ports take architecture flags below without linking the device.
  # Pass the target macro explicitly so FreeRTOSConfig.h can select its port.
  # FU2 also links the device to inherit its word-only SRAM compiler policy.
  # no ${TARGET_NAME} macro (e.g. WEACT_F412) as normal code app. FreeRTOSConfig.h needs this macro to
  # #if defined(WEACT_F412) chooses to use configMAX_SYSCALL_INTERRUPT_PRIORITY (port CM4F required,
  # see portmacro.h) - see here because link add device target (link include other path/flag not
  # necessary for FreeRTOS kernel).
  target_compile_definitions(freertos_config INTERFACE ${TARGET_NAME})
  if(TARGET_NAME STREQUAL "FU2")
    target_link_libraries(freertos_config INTERFACE ${NEON_MDF_DEVICE_TARGET})
    target_include_directories(freertos_config INTERFACE
      ${NEON_MDF_PATH}/components/driver)
  endif()

  set(FREERTOS_PORT ${_freertos_port} CACHE STRING "" FORCE)
  set(FREERTOS_HEAP 4 CACHE STRING "" FORCE)

  add_subdirectory(${NEON_MDF_PATH}/rtos/FreeRTOS-Kernel ${CMAKE_BINARY_DIR}/FreeRTOS-Kernel)

  if(_arch MATCHES "^riscv")
    # freertos_kernel (croutine/list/queue/tasks/timers/.../heap_4.c - C
    # C sources) and freertos_kernel_port (port.c/portASM.S) both require -march/-mabi
    # real (toolchain riscv64-elf-gcc not from inference using default ilp32e/rv32ec, see
    # cmake/arch/riscv.cmake) - missing will link wrong direct kien (ELFCLASS64 vs 32), same as
    # error observed while bringing up the device library.
    include(${NEON_MDF_PATH}/cmake/arch/riscv.cmake)
    string(REPLACE "riscv-" "" _riscv_isa ${_arch})
    set(_riscv_abi ${NEON_MDF_TARGET_${TARGET_NAME}_ABI})
    neon_mdf_apply_riscv_arch(freertos_kernel ${_riscv_isa} ${_riscv_abi})
    neon_mdf_apply_riscv_arch(freertos_kernel_port ${_riscv_isa} ${_riscv_abi})

    # FreeRTOS-Kernel core (vendor) using unregulated memcpy/memset/strlen
    # KIEN (heap_4.c/tasks.c/queue.c/...) - toolchain RISC-V bare-metal
    # (ABI ilp32e) does not have real libc, so no <string.h>/<stdlib.h> - from the same level shim i lack
    # (see arch/RISCV/Core/Inc/libc_shim/ + arch/RISCV/Core/Src/riscv_libc_shim.c, same property as
    # riscv_soft_math.c used for __udivsi3/...). only image oriented RISC-V (FU2/WEACT_F412 has real
    # newlib, do not use this shim).
    target_include_directories(freertos_kernel PRIVATE ${NEON_MDF_PATH}/arch/RISCV/Core/Inc/libc_shim)
    target_include_directories(freertos_kernel_port PRIVATE ${NEON_MDF_PATH}/arch/RISCV/Core/Inc/libc_shim)
    target_sources(freertos_kernel PRIVATE ${NEON_MDF_PATH}/arch/RISCV/Core/Src/riscv_libc_shim.c)
  else()
    include(${NEON_MDF_PATH}/cmake/arch/cortex-m.cmake)
    # FPU flag must be traced to both targets (port ARM_CM4F uses real VFP in port.c/portasm.c - missing
    # -mfpu=fpv4-sp-d16 -mfloat-abi=hard will link sai/UsageFault, see cmake/arch/cortex-m.cmake).
    neon_mdf_apply_cortex_m_arch(freertos_kernel ${_arch} ${_freertos_fpu})
    neon_mdf_apply_cortex_m_arch(freertos_kernel_port ${_arch} ${_freertos_fpu})
  endif()
endfunction()
