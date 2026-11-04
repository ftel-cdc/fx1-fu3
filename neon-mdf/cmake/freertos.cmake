# Wire FreeRTOS-Kernel (đã vendor ở rtos/FreeRTOS-Kernel)
# vào build, dùng đúng hệ thống CMake chính thức của kernel (add_subdirectory
# + target `freertos_config`) thay vì tự liệt kê source file tay - tránh sót
# file khi submodule update (vd mpu_wrappers_v2_asm.c mà port ARM_CM0 cần).
#
# neon_mdf_add_freertos(<TARGET_NAME> <CONFIG_DIR>)
#   TARGET_NAME - target đã đăng ký trong cmake/mcu_variants.cmake (lấy arch
#                 để áp -mcpu/-mthumb cho target freertos_kernel/_port, vì
#                 2 target này KHÔNG link vào neon_mdf_device_<X> nên không
#                 tự thừa hưởng cờ kiến trúc).
#   CONFIG_DIR  - thư mục chứa FreeRTOSConfig.h của app (mỗi app có 1 file
#                 riêng - xem Projects/Examples/FreeRTOS_Blinky/Inc/).
#
# Chỉ được gọi 1 lần cho cả 1 lần cmake configure (add_subdirectory kernel chỉ
# chạy 1 lần) - vì mỗi app giờ là 1 CMake project độc lập (xem
# tools/cmake/project.cmake) nên việc này tự nhiên đúng, không còn rủi ro
# nhiều app FreeRTOS cùng build chung 1 lần configure như trước.
function(neon_mdf_add_freertos TARGET_NAME CONFIG_DIR)
  if(TARGET_NAME STREQUAL "FX1")
    message(FATAL_ERROR "FX1 currently supports bare-metal only; FreeRTOS tick/trap integration is not ported")
  endif()
  if(TARGET freertos_kernel)
    return()
  endif()

  if(NOT NEON_MDF_PATH)
    message(FATAL_ERROR "NEON_MDF_PATH chua duoc set truoc khi goi neon_mdf_add_freertos()")
  endif()

  set(_arch ${NEON_MDF_TARGET_${TARGET_NAME}_ARCH})
  if(NOT _arch)
    message(FATAL_ERROR "neon_mdf_add_freertos: target '${TARGET_NAME}' chua duoc dang ky trong cmake/mcu_variants.cmake")
  endif()

  # Dispatch port theo arch - moi port co file portmacro.h/port.c rieng
  # trong rtos/FreeRTOS-Kernel/portable/GCC/<PORT>/ (da vendor san, khong
  # can them submodule). ARM_CM0 (FU2), ARM_CM4F (WEACT_F412, co FPU that -
  # xem NEON_MDF_TARGET_<X>_FPU trong mcu_variants.cmake) va GCC_RISC_V_GENERIC
  # (CH32V003F4P6, them 2026-08-10) la cac port da wire toi nay - them target
  # kien truc khac phai them nhanh o day.
  if(_arch MATCHES "^cortex-m0")
    set(_freertos_port GCC_ARM_CM0)
    set(_freertos_fpu OFF)
  elseif(_arch STREQUAL "cortex-m4")
    set(_freertos_port GCC_ARM_CM4F)
    set(_freertos_fpu ${NEON_MDF_TARGET_${TARGET_NAME}_FPU})
  elseif(_arch MATCHES "^riscv")
    # QingKe V2A (CH32V003) KHONG co CLINT (Core Local Interrupter chuan cua
    # RISC-V) - da xac nhan THAT qua QingKeV2 Processor Manual V1.0 (tai
    # that tu ch405-labs.com, doi chieu mtvec/mcause/PFIC luc lam viec nay,
    # khong doan) - PFIC (vendor-specific) quan ly ngat thay CLINT. Port
    # RISC-V cua FreeRTOS-Kernel HO TRO SAN cau hinh nay (KHONG phai tu
    # improvise) qua chip_specific_extensions "RISCV_no_extensions"
    # (portasmHAS_MTIME=0, portasmHAS_SIFIVE_CLINT=0 - xem
    # rtos/FreeRTOS-Kernel/portable/GCC/RISC-V/chip_specific_extensions/RISCV_no_extensions/).
    # Setup that (mtvec Direct mode + STK sinh ngat tick + PFIC IENR) nam
    # trong examples/FreeRTOS_Blinky/Src/main.c (vPortSetupTimerInterrupt()/
    # freertos_risc_v_application_interrupt_handler(), 2 ham "weak" cua
    # port.c/portASM.S ma app tu ghi de - xem comment day du o do).
    set(_freertos_port GCC_RISC_V_GENERIC)
    set(FREERTOS_RISCV_EXTENSION RISCV_no_extensions CACHE STRING "" FORCE)
  else()
    message(FATAL_ERROR "neon_mdf_add_freertos: chua wire port FreeRTOS cho arch '${_arch}' (target '${TARGET_NAME}') - xem rtos/FreeRTOS-Kernel/portable/GCC/ de chon port phu hop roi them nhanh trong cmake/freertos.cmake")
  endif()

  add_library(freertos_config INTERFACE)
  target_include_directories(freertos_config SYSTEM INTERFACE ${CONFIG_DIR})
  target_compile_definitions(freertos_config INTERFACE projCOVERAGE_TEST=0)
  # Non-FU2 ports take architecture flags below without linking the device.
  # Pass the target macro explicitly so FreeRTOSConfig.h can select its port.
  # FU2 also links the device to inherit its word-only SRAM compiler policy.
  # KHONG tu co macro ${TARGET_NAME} (vd WEACT_F412) nhu code app binh
  # thuong. FreeRTOSConfig.h can macro nay de #if defined(WEACT_F412) chon
  # dung configMAX_SYSCALL_INTERRUPT_PRIORITY (port CM4F bat buoc phai co,
  # xem portmacro.h) - truyen qua day thay vi link them device target (tranh
  # keo theo include path/flag khac khong can thiet cho riêng FreeRTOS kernel).
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
    # thuan) VA freertos_kernel_port (port.c/portASM.S) DEU can -march/-mabi
    # THAT (toolchain riscv64-elf-gcc KHONG tu suy dung ilp32e/rv32ec mac
    # dinh, xem cmake/arch/riscv.cmake) - thieu se link sai kien truc
    # (ELFCLASS64 vs 32), giong loi da gap luc bring-up device library.
    include(${NEON_MDF_PATH}/cmake/arch/riscv.cmake)
    string(REPLACE "riscv-" "" _riscv_isa ${_arch})
    set(_riscv_abi ${NEON_MDF_TARGET_${TARGET_NAME}_ABI})
    neon_mdf_apply_riscv_arch(freertos_kernel ${_riscv_isa} ${_riscv_abi})
    neon_mdf_apply_riscv_arch(freertos_kernel_port ${_riscv_isa} ${_riscv_abi})

    # FreeRTOS-Kernel core (vendor) dung memcpy/memset/strlen KHONG DIEU
    # KIEN (heap_4.c/tasks.c/queue.c/...) - toolchain RISC-V bare-metal
    # (ABI ilp32e) khong co libc that nen khong co <string.h>/<stdlib.h> -
    # tu cung cap shim toi thieu (xem arch/RISCV/Core/Inc/libc_shim/ +
    # arch/RISCV/Core/Src/riscv_libc_shim.c, cung tinh than
    # riscv_soft_math.c da dung cho __udivsi3/...). CHI anh huong RISC-V
    # (FU2/WEACT_F412 co newlib that, khong dung toi shim nay).
    target_include_directories(freertos_kernel PRIVATE ${NEON_MDF_PATH}/arch/RISCV/Core/Inc/libc_shim)
    target_include_directories(freertos_kernel_port PRIVATE ${NEON_MDF_PATH}/arch/RISCV/Core/Inc/libc_shim)
    target_sources(freertos_kernel PRIVATE ${NEON_MDF_PATH}/arch/RISCV/Core/Src/riscv_libc_shim.c)
  else()
    include(${NEON_MDF_PATH}/cmake/arch/cortex-m.cmake)
    # FPU flag PHAI truyen cho ca 2 target (port ARM_CM4F dung lenh VFP that
    # trong port.c/portasm.c - thieu -mfpu=fpv4-sp-d16 -mfloat-abi=hard se
    # link sai/UsageFault, xem cmake/arch/cortex-m.cmake).
    neon_mdf_apply_cortex_m_arch(freertos_kernel ${_arch} ${_freertos_fpu})
    neon_mdf_apply_cortex_m_arch(freertos_kernel_port ${_arch} ${_freertos_fpu})
  endif()
endfunction()
