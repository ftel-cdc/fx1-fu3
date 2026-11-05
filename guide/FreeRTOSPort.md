# Wire FreeRTOS for a new CPU architecture

`cmake/freertos.cmake`'s `neon_mdf_add_freertos()` is the only dispatch point selecting the
FreeRTOS-Kernel port (`rtos/FreeRTOS-Kernel/portable/GCC/<PORT>/` directory) according to the `arch`
of target — currently has wire 3 ports for 2 CPU families:
`GCC_ARM_CM0` (`FU2`), `GCC_ARM_CM4F` (`WEACT_F412`), `GCC_RISC_V_GENERIC` +
chip extension `RISCV_no_extensions` (`CH32V003F4P6`). This file explains the real dispatch + 2
pattern glue-code mechanism needed when adding a new port, to add a 3rd architecture without having
to guess from scratch.

Read `guide/AddNewMCU.md` first if the target does not yet exist in SDK — FreeRTOS is only 1 step in
the process of adding that target, not the first step.

## 1. Dispatch mechanism (`cmake/freertos.cmake`)

```cmake
if(_arch MATCHES "^cortex-m0")
  set(_freertos_port GCC_ARM_CM0)
elseif(_arch STREQUAL "cortex-m4")
  set(_freertos_port GCC_ARM_CM4F)
elseif(_arch MATCHES "^riscv")
  set(_freertos_port GCC_RISC_V_GENERIC)
  set(FREERTOS_RISCV_EXTENSION RISCV_no_extensions CACHE STRING "" FORCE)
else()
  message(FATAL_ERROR "FreeRTOS for arch '${_arch}'")
endif()
```

`_arch` reads from `NEON_MDF_TARGET_<TARGET>_ARCH` (`cmake/mcu_variants.cmake`, see
`guide/AddNewMCU.md` target registry entry). Adding new architecture = adding 1 branch `elif` select
the correct port directory available in `rtos/FreeRTOS-Kernel/portable/GCC/` (kernel already vendors
all official ports — lists `ls rtos/FreeRTOS-Kernel/portable/GCC/` to see which port is available
for the target CPU before writing glue code, high probability already).

After selecting the port, `add_subdirectory(rtos/FreeRTOS-Kernel ...)` (the official CMake build of
the kernel — SDK does NOT manually list the source file) creates 2 targets `freertos_kernel` (pure
C: tasks/queue/timers/heap_4... code) and `freertos_kernel_port` (`port.c`/`portasm`/`portASM.S`
specific to the selected port) — both do NOT have their own architecture flags (`-mcpu`/`-march`...)
because there is no link `neon_mdf_device_<TARGET>` — this file automatically applies with
`neon_mdf_apply_cortex_m_arch()`/`neon_mdf_apply_riscv_arch()` right after that. The new CPU port
reads the corresponding passage carefully to apply the correct flag for both targets, not just 1.

## 2. Pattern "Direct Routing" — when the handler name of another port vector table creates a common

Common problem: `mcu-description`'s codegen always generates vector tables using common CMSIS
handler names (`SVC_Handler`/`PendSV_Handler`/ `SysTick_Handler`, `weak`) — but some FreeRTOS ports
(e.g. `GCC_ARM_CM4F`) give themselves different names (`vPortSVCHandler`/`xPortPendSVHandler`/
`xPortSysTickHandler`) and check that the name matches the actual vector table when
`xPortStartScheduler()` runs (`configASSERT` in `portmacro.h`) — automatic weak- override (based on
the same name) does not occur because the names are already different.

**Symptoms if ignored** (actually encountered at bring-up `WEACT_F412`): `xPortStartScheduler()` hangs
permanently at `configASSERT` — confirmed via PC reading by SWD at debug, not HardFault or other
obvious error.

**Fix real** (see `examples/FreeRTOS_Blinky/Src/main.c`, self-written app — NOT fix
`startup_<target>.s` shared for all apps):

```c
void vPortSVCHandler(void);
void xPortPendSVHandler(void);
void xPortSysTickHandler(void);

__attribute__((naked)) void SVC_Handler(void)     { __asm volatile("b vPortSVCHandler"); }
__attribute__((naked)) void PendSV_Handler(void)  { __asm volatile("b xPortPendSVHandler"); }
void SysTick_Handler(void) { xPortSysTickHandler(); }
```

`SVC_Handler`/`PendSV_Handler` MUST be `naked` + jump with `b` (not `bl`/call normal C) —
`vPortSVCHandler`/`xPortPendSVHandler` is the "naked" function of the port itself (manages
`EXC_RETURN`/stack to switch tasks), it should keep the exception context intact as jumped directly
from the vector table. `SysTick_Handler` does not (only ticks up, does not touch `EXC_RETURN`),
calling normal C is sufficient.

At the same time `FreeRTOSConfig.h` must disable `configCHECK_HANDLER_INSTALLATION` (0) for this
type port. — enables default "Indirect Routing" assumption (matching name), does not match pattern
above (see comment in
`tools/templates_freertos/Inc/FreeRTOSConfig.h`).

**When to worry about this**: check if `portable/GCC/<PORT>/port.c` of the new port — has
`configASSERT(pxVectorTable[...] == vPortSVCHandler)` or similar. `GCC_ARM_CM0` (FU2) DOES NOT have
this step (duplicate name, weak-override automatically works) — not all Cortex-M ports need this
glue.

## 3. Pattern "CLINT-less" — when MCU has no RISC-V standard timer

General problem: the default `GCC_RISC_V_GENERIC` port assumes to have a CLINT (Core Local
Interrupter — RISC-V standard `mtime`/`mtimecmp` timer) to create its own interrupt tick. The
vendor-specific MCU (e.g. WCH's QingKe V2A on `CH32V003F4P6`) may not have a CLINT — uses the
vendor's own timer + interrupt controller (PFIC here) — port needs to be specified via an extension
chip `RISCV_no_extensions` (`FREERTOS_RISCV_EXTENSION`, see section 1 — already in the vendor
kernel, no need to write the extension yourself, just choose the right one) → 2 the port's `weak`
function forces the app to overwrite itself:

- `vPortSetupTimerInterrupt()` (`port.c`) — manually configure the real timer + enable interrupts for
  it via the real interrupt controller. Call once from `xPortStartScheduler()`, BEFORE
  `xPortStartFirstTask()` (safe to change `mtvec` here — has no interrupt enabled). Real example
  (`CH32V003F4P6`, see `examples/FreeRTOS_Blinky/Src/main.c` for full code + whole datasheet
  lookup): set `mtvec` to Direct mode (bit`[1:0]=00`, ALL exception/interrupt jump to 1 address
  `freertos_risc_v_trap_handler` of `portASM.S` — different "address mode" vector-table
  private-each-IRQ that `startup_<target>.s` shares configuration available at reset), configuration
  counter vendor-specific, enable interrupts via vendor-specific interrupt controller.

- **`freertos_risc_v_application_interrupt_handler()`** (`portASM.S`) — because of
  `portasmHAS_MTIME=0` (due to selecting `RISCV_no_extensions`), port NO longer recognizes itself as
  "is this a tick timer" for itself anymore (that logic only exists in the branch
  `portasmHAS_MTIME!=0`) — app must manually read the interrupt cause (register the real cause of
  the architecture, e.g. `mcause` on standard RISC-V) and call itself
  `xTaskIncrementTick()`/`vTaskSwitchContext()` when it is true that the tick timer is — ignored (no
  infinite loop) for other interrupts so as not to "hang" falsely if an unknown interrupt is turned
  on by mistake.

`portYIELD()` (used by `vTaskDelay()`/... when active task switching is required OUTSIDE interrupt)
works independently of CLINT/interrupt-controller — is the standard synchronous exception of the
architecture (`ecall` on RISC-V), requiring no glue code.

**When to worry about this**: RISC-V MCUs do not have standard CLINT (read the reference manual
section interrupt controller/timer before assuming) — check
`rtos/FreeRTOS-Kernel/portable/GCC/RISC-V/chip_specific_extensions/` if the device matching
extension is available (`RISCV_no_extensions` can be used for any MCU that takes care of
timer/interrupt completely, not WCH alone).

## 4. `FreeRTOSConfig.h` by architecture

`tools/templates_freertos/Inc/FreeRTOSConfig.h` (app file, not SDK — each app has its own version,
edit freely) use `#if defined(<TARGET>)` to select different fields according to port — new port
needs to read carefully its own `portmacro.h` to see which macro `config*` is required (e.g.
`GCC_ARM_CM4F` requires `configMAX_SYSCALL_INTERRUPT_PRIORITY`/`configPRIO_BITS` matches real NVIC;
`GCC_ARM_CM0` does not, because this port fixes priority by a constant). If the bare-metal toolchain
of the new architecture does not have real libc (like `riscv-none-elf-gcc` build for `ilp32e` — does
not have multilib, see `guide/AddNewMCU.md`), `freertos_kernel`/`freertos_kernel_port` (use
`memcpy`/`memset`/`strlen` unconditionally in `heap_4.c`/`tasks.c`/...) need to shim minimal
self-sufficient libc — see `arch/RISCV/Core/Inc/libc_shim/` + `cmake/freertos.cmake`'s branch `_arch
MATCHES "^riscv"` as template.

## 5. Checklist add FreeRTOS port for new architecture

1. `ls rtos/FreeRTOS-Kernel/portable/GCC/` — official port is available for the target CPU yet (highly
   likely yes, kernel vendor is very complete).
2. Add branch `elif` in `neon_mdf_add_freertos()` (section 1) select the correct directory port + (if
   RISC-V) correct chip extension.
3. Apply the correct architectural flags to both `freertos_kernel` and `freertos_kernel_port` (2
   separate targets, not inheriting each other).
4. read `port.c`/`portmacro.h` of new port — have `configASSERT` check handler name match vector table
   (section 2)? Have standard CLINT/timer assume, the target MCU is real (section 3)?
5. If the toolchain does not have real libc, add shim (section 4).
6. Write `FreeRTOSConfig.h` for the first app test, fill in required `config*` according to
   `portmacro.h`.
7. Build + verify real on board (not believe clean build) — runs 2 minimum preemptive tasks (same as
   `examples/FreeRTOS_Blinky`) to prove scheduler/tick/context-switch works real, not just compile
   through.
