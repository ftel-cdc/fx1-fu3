# rtos

- `FreeRTOS-Kernel/` is vendored from the official repository
  (`https://github.com/FreeRTOS/FreeRTOS-Kernel.git`), with only the kernel and
  no demo projects. Three ports are integrated and selected automatically by
  `arch` in `cmake/freertos.cmake`:

  - `portable/GCC/ARM_CM0` for FU2 (Cortex-M0): the generic port needs no custom
    changes.
  - `portable/GCC/ARM_CM4F` for WEACT_F412 (Cortex-M4F): explicitly alias
    SVC/PendSV/SysTick to the port's handler names; see `guide/FreeRTOSPort.md`.
  - `portable/GCC/RISC-V` with the `RISCV_no_extensions` chip extension for
    CH32V003F4P6: there is no CLINT, so the application configures the tick
    timer and `mtvec`; see the same guide.

  FX1 uses the same RISC-V kernel port with the CLINT chip extension,
  plus local PLIC/FPU integration in `port/fx1/`. The optional ISP service
  lives in `services/ftel_isp/`; it owns ISP access through a worker task,
  queue/tickets and interrupt notifications. Build/run commands and scope:
  [FX1 FreeRTOS](../../guide/FX1_FREERTOS.md).

  **Current status:** this is a **plain vendored copy**, not an active Git
  submodule; the original repository had not been initialized with `git init`.
  To convert it to a submodule after initializing this repository:

  ```sh
  rm -rf rtos/FreeRTOS-Kernel   # Remove the current vendored copy
  git submodule add --depth 1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git rtos/FreeRTOS-Kernel
  ```

  CMake integration in `cmake/freertos.cmake` uses the kernel's official
  `add_subdirectory()` support instead of listing its sources manually. See
  `examples/FreeRTOS_Blinky/`. Each application owns its `FreeRTOSConfig.h`
  in `Inc/`, since stack, heap and priority settings belong to the application.
  See section 9 of `ARCHITECTURE.md` for details and the verified
  `GCC_ARM_CM0` behavior: `configPRIO_BITS` is not required.
