# Analysis of firmware architecture and neon-mdf handover documentation

This document is derived from the repository's source code as of 2026-08-11. The conclusions are
distinguished as follows:

- **Truth:** expressed directly in the source code or configuration.
- Declaration of project: documented by the project but not verified in the current environment.
- Inference: Reasonable technical conclusions drawn from the implementation.

## 1. Overview Conclusion

`neon-mdf` is not a single product firmware. It is an ESP-IDF room-specific SDK for small
microcontrollers, including:

- MCU descriptor using YAML and code generator.
- CMake/Kconfig. build system
- Startup, linker script and abstraction layer breaks for Cortex-M and RISC-V.
- Drivers use a common API but have their own implementation for each chip family.
- Bootloader UART, image encapsulation and mechanism OTA/rollback.
- FreeRTOS, sample applications, CLI and VS Code extensions.

Three existing targets:

| Target | CPU | Flash/RAM | Current Reliability |
|---|---|---:|---|
| `FU2` | Arm Cortex-M0, up to 60 MHz | 512 KiB / 128 KiB | Register address and UART/I2C/GPIO also as placeholder; not safe to run on silicon |
| `WEACT_F412` | STM32F412RET6, Cortex-M4F | 512 KiB / 256 KiB | Recognized by the project for hardware testing; main test rig |
| `CH32V003F4P6` | WCH QingKe V2A, RV32EC | 16 KiB / 2 KiB | Recorded as build/link; not verified on real board |

This repository does not contain Cortex-A, Kria K26, Zynq MPSoC, GIC, MMU, DDR, Linux, AXI or PL
communication. The actual architecture is bare-metal Cortex-M/PFIC, optionally running FreeRTOS.

Central Architectural Flow:

```text
mcu-description/devices/<TARGET>.yaml
       |
       +--> device header and register struct
       +--> startup code and vector table
       +--> linker script whole-flash/bootloader/application
       +--> debugger SVD
       +--> metadata CMake
                    |
application --> API driver general --> implementation by family --> MMIO
                    |
                    +--> FreeRTOS optional
                    +--> bootloader + OTA optional
```

The biggest technical risks are FU2 hardware emulators, OTA metadata does not transact, flash
drivers do not check for errors/timeouts, marginal cases of image limits in the bootloader, CRC does
not provide authenticity and lacks automatic testing.

## 2. Repository range

Checkout currently available 1.105 files. Most are CMSIS 5 and the entire port array of FreeRTOS
upstream. If this type of vendor libraries, automatic output generation, cache and extension
packages, code/configuration section under the project has about 181 files, 15.400 lines.

```text
/
+-- README.md, ARCHITECTURE.md, SETUP.md, VSCODE.md
+-- guide/ developer guide
+-- examples/ independent application projects
+-- neon-mdf/
| +-- CMakeLists.txt, Kconfig composition of SDK
|   +-- arch/                    startup, linker, IRQ abstraction
|   +-- components/              driver, bootloader, OTA
| +-- mcu-description/ YAML, schema and generator
|+-- cmake/ coordinate target/toolchain/build
| +-- tools/ CLI, image builder and template
|   +-- rtos/FreeRTOS-Kernel/    FreeRTOS vendor
+-- vscode-extension/ TypeScript development tool
```

Automatically generated startup/header/linker files are not directly edited. Sources of truth are
YAML, Python generator and handwritten `system_<TARGET>.c` files.

## 3. Architecture build

Each application is a standalone CMake project. The application's `CMakeLists.txt` include
`neon-mdf/tools/cmake/project.cmake`, after which the SDK is added using
`add_subdirectory()`.

Important Configuration Order:

```text
mcu_variants.cmake create/select device target and architecture flag flashmap.cmake create
partition, enable OTA if available flashmap kconfig.cmake create sdkconfig.h and target menuconfig
components/driver select implementation by family components/bootloader same level CRC, UART and
source bootloader application CMake create one or more ELFs flash_targets.cmake package and create
firmware flash target
```

The main target CMake:

| Target | Roles |
|---|---|
| `neon_mdf_device_<TARGET>` | Startup, system, IRQ port, header and CPU flags |
| `neon_mdf_kconfig` | Export include path of `sdkconfig.h` |
| `neon_mdf_driver` | GPIO and driver configured for family |
| `neon_mdf_crc32` | CRC-32 reflective, polynomial `0xEDB88320` |
| `neon_mdf_bootloader_uart` | UART polling for recovery |
| `neon_mdf_flashmap` | Export `flashmap.h` generated automatically |
| `neon_mdf_ota` | read/write/confirm OTA metadata |
| `freertos_kernel[_port]` | Kernel and port FreeRTOS selected |

Kconfig is the compile-time composition mechanism, not the runtime registry. `sdkconfig` is
converted to `sdkconfig.h`; CMake uses symbols to determine which source drivers are compileable.

### Build Test Status

Tried `cmake --preset WEACT_F412` in `examples/Blinky`. CMake detected Arm GCC 10.2.1 but stopped at
`mcu_variants.cmake` because it was required to find executable named `python3`, while `python3` is
not available on `PATH`. Toolchain RISC-V is also not available on `PATH`. So this document does not
claim to have clean-build success in the current environment.

## 4. Reset and startup flow

### 4.1 Cortex-M

```text
hardware reset
  -> vector[0] load MSP = _estack
  -> vector[1] call Reset_Handler startup_<target>.s
  -> set SP
  -> copy .data from flash to RAM
  -> zero .bss
  -> SystemInit()                                 system_<target>.c
       -> M4F: open CP10/CP11 permissions for FPU
       -> SCB->VTOR = g_pfnVectors
       -> SystemCoreClock updates
  -> main()                                       bootloader/application
```

Startup is not currently browsing `.preinit_array`/`.init_array` and not calling
`__libc_init_array()`. This is suitable for the current C application but is a hidden constraint if
you add C++ or libraries using constructors. If `main()` returns, the behavior has no valid
contract; the current `main()` runs indefinitely.

The weak handler points to `Default_Handler`, which is an infinite loop. On Cortex-M4F, FreeRTOS
uses a direct-routing handler name so the example must explicitly link `SVC_Handler`,
`PendSV_Handler` and `SysTick_Handler` to the FreeRTOS port.

### 4.2 CH32V003

```text
reset at _start
  -> jump handle_reset; the following words are PFIC address-mode table
  -> initialize gp and sp
  -> copy .data, zero .bss
  -> set mstatus.MPIE
  -> write CSR WCH INTSYSCR 0x804=2
  -> mtvec = _start | 3, WCH Address mode
  -> SystemInit()
  -> mepc = main; mret
```

Using `mret` instead of `jal main` establishes a purposeful machine-mode interrupt state. When
running FreeRTOS, `vPortSetupTimerInterrupt()` changes `mtvec` to the trap handler Direct mode of
FreeRTOS because the generic port needs to decode `mcause` by itself.

### 4.3 Linker and memory layout

```text
FLASH: vector/.init -> .text -> .rodata -> unwind -> load image of .data RAM: .data -> .bss ->
redundant area heap/stack Peak RAM: _estack
```

Linker script WEACT whole-flash for 0x400 bytes for heap and 0x400 bytes for stack. This is only a
reservation at the linker, not implementation allocator. App and bootloader have different
origin/length FLASH but use the same physical RAM because they don't run simultaneously.

There is no MPU/MMU, cache maintenance, special DMA buffer, memory barrier policy or dynamic
allocation in the SDK's driver/bootloader.

## 5. Application Execution Model

### Bare-metal Blinky

`main()` turns on the GPIO clock, configures an output pin and iterates: toggle, busy delay, print
targeted log and confirm OTA after ten loops. The delay is measured in loops, not time; it depends
on the compiler and the clock.

### FreeRTOS Blinky

`main()` configures two batteries, creates two tasks with the same priority and calls scheduler.
Task 0 toggles every 200 ms; on WEACT OTA it confirms the firmware after about 4 seconds. Task 1
toggles every 500 or 250 ms depending on the version.

The two tasks use different pins, but `neon_gpio_toggle_pin()` is still a string that reads
ODR/OUTDR and decides set/reset. this API is not atomic if task/ISR overlap mask changes.

No `xTaskCreate()` command checks for return value. With a heap missing, a task may not be created
and the application continues. Stack overflow hooks only shut down and hang, do not log or reset.

## 6. Bootloader and OTA

### 6.1 Image format

```text
partition base
+0x00  image_header_t: magic, version, size, crc32 — 16 byte
+0x10 unused padding
+image_header_size vector/startup and payload application
```

The redundant header space is larger than the 16 byte struct to satisfy the alignment of
vector/VTOR. CRC is calculated on the correct `size` payload bytes. `version` is saved but the
bootloader does not check. CRC detects only random errors; no tampering, replay or downgrade.

### 6.2 Timeline boot

```text
Reset_Handler -> SystemInit -> bootloader main
  -> initialize UART polling
  -> turn on hardware counter 1 ms
  -> wait for the first byte up to 3000 ms 0x55: enter update mode other bytes/timeout: select normal
       image
  -> no flashmap: check header and jump app fixed
  -> have flashmap: read otadata, CRC image, increase attempt/rollback, jump slot
```

Protocol update:

```text
SYNC 0x55 -> READY 'R' -> header 16 byte -> ACK/NAK
-> 256 byte chunks, ACK each chunk
-> CRC full image -> ACK/NAK end
```

ACK of the header is only sent after erase and program header. This is the right design for the host
to not send payload while the CPU is blocking because of the flash erase.

Once in update mode, every UART receiver can block infinitely. There is no transaction timeout,
sequence number, chunk CRC, retry, abort or watchdog.

### 6.3 Transfer permissions to application

WEACT reads MSP/reset PC from application vector, writes VTOR, loads MSP and calls reset address.
The code does not check whether the MSP is in RAM, whether the PC reset is in partition, alignment
and Thumb state.

Handoff production-grade should disable SysTick/interrupt, clear NVIC enable/pending, restore
CONTROL/BASEPRI/PRIMASK, check MSP/PC, use DSB/ISB around VTOR and branch does not create C call
frame.

### 6.4 State machine OTA

```text
invalid otadata ---------------------->boot factory

install OTA successfully
  -> write {slot=OTA, PENDING, attempts=0}
  -> reset

PENDING boot
  -> image broken ------------->write factory VALID ->boot factory
  -> attempts++
       attempts <= 3 --------> boot OTA
           app confirm ------>write OTA VALID crash/reset ------>next PENDING attempts > 3 --------->write
       factory VALID ->boot factory

OTA VALID + valid image -----------------> always boot OTA
```

Asymmetric model: OTA only records OTA slots, not touching the factory. The advantage is that there
is always a recovery image; the disadvantage is that it is impossible to update the factory
remotely.

Each metadata change erases both sectors and writes a 20 byte record. Power loss during writing
causes CRC metadata to fail; bootloader fallsback to factory. This fail-safe method allows booting
but does not preserve transaction status, has no redundant records, sequence, commit marker or
wear-leveling.

`neon_ota_confirm_valid()` does not verify `active_slot == OTA`, the state is actually PENDING, or
the PC is currently running in the selected slot. `neon_ota_read_data()` and `WriteData()` also do
not check null pointers.

## 7. Module analysis

### GPIO

API includes clock enable, init input/output, read, write and toggle. Driver does not save state;
persistent state is in the peripheral register.

- WEACT: RCC AHB1, MODER/IDR/ODR/BSRR.
- CH32: APB2, CFGLR/INDR/OUTDR/BSHR/BCR.
- FU2: DIR/DATA/SET/CLR placeholder; clock enable is no-op.

Set/reset equals BSRR/BSHR/BCR which is atomic at the peripheral. Toggles are not atomic due to
needing to read state first. Invalid modes are treated as input; out - of - range pin bits are
silently ignored.

### Flash

WEACT uses an irregular table of eight STM32F4 sectors and a 32 - bit program word in the VDD 3.3 V
assumption. CH32 uses sector 1 KiB and splits a word into two programs
halfword.

Both implementations:

- Poll BUSY infinite, no timeout.
- Do not check error program/erase/protection/alignment.
- Do not read-back data.
- To flash in unlock state.
- Do not check address/range and overflow `address + size`.
- Returned `void`, so the caller could not recover the error.
- No reentrant and unsafe in ISR.

### Public UART and I2C

This is a polling placeholder driver for FU2. Register map, divider and bit status are not based on
real TRM. `Timeout` is the number of poll rings for each byte/giai segment, not millisecond or full
transaction deadline. No pinmux, clock, reset, error flag, bus recovery, interrupt or DMA.

Bootloader UART is a family-specific implementation for recovery. WEACT_F412 currently does not have
the syscall retarget `printf()` to UART bootloader.

### IRQ abstraction

Cortex-M uses NVIC/PRIMASK. RISC-V uses PFIC and CSR WCH. This is only the API that controls the
enable/disable/priority/critical section, not the dispatcher; the handler name still comes from the
vector table or port FreeRTOS.

### Generator MCU Description

`model.py` defines object device/peripheral/register/interrupt. `loader.py` parses YAML and checks
schema, uniqueness, memory, bootloader sizing, erase geometry and vector alignment. `generate.py`
dispatch to backend Cortex-M or RISC-V to generate header, linker, startup, SVD and CMake fragments.

This is the point with the largest radius blast: a generator error can affect every target.
Golden-output tests and unit tests are required for validators.

### Host Tools

- `gen_flashmap.py`: parse CSV, align partition, check fit, create header and
  linker script.
- `build_ota_image.py`: pair header and CRC with payload.
- `build_otadata_image.py`: create scratch metadata/slot transfer.
- `build_full_image.py`: pair bootloader, factory and metadata.
- `stm32_uart_flash_firmware.py`: communication STM32 ROM bootloader.
- `neon_mdf.py`: build/flash/monitor/menuconfig. dispatch CLI

### Extension VS Code

Extensions only run in the development machine, do not link to firmware. The module manages
target/transport/port, runs CMake, reads CMake File API, flashes, debugs OpenOCD, menuconfig, serial
monitor and creates projects. The main risk is quoting paths, process lifecycle, stale build state
and WCH flash/debug has not been verified.

## 8. Data flow and ownership

### Firmware Update via UART

```text
host .bin
 -> host create header/CRC
 -> serial
 -> UART DATA register
 -> buffer stack 16 byte / 256 byte
 -> flash recorder slot controller
 -> CPU reread flash to calculate CRC
 -> OTA metadata recording
 -> reset -> select slot -> application
```

Do not use heap. Buffer belongs to the stack of `run_update_mode*()` and is reused round by round.
Payload is written immediately; losing connection leaves an unfinished OTA slot but the factory is
not seen changing.

### OTA Confirmation

```text
health policy of the app
 -> neon_ota_confirm_valid
 -> copy metadata flash to stack
 -> check magic/CRC
 -> convert state/attempt
 -> erase metadata sector
 -> program five word
 -> read bootloader at the next reset
```

Application must serialize all flash access by itself. The SDK has no mutex or protection mechanism
if other task/ISR use the same flash.

## 9. Breaker System

Bare-metal example does not enable interruption. Bootloader only poll counter flags, does not enable
SysTick interrupt. Default handler hangs indefinitely.

| Target | Tick | Context switch | Yield/service |
|---|---|---|---|
| FU2 M0 | `SysTick_Handler` of CM0 port | `PendSV_Handler` | `SVC_Handler` |
| WEACT M4F | wrapper call `xPortSysTickHandler` | tail-branch to `xPortPendSVHandler` | tail-branch to `vPortSVCHandler` |
| CH32 RV32EC | STK/PFIC IRQ 12 | trap call scheduler switch | machine `ecall` |

There is no ISR for UART, I2C, GPIO, flash, DMA, watchdog or app timer. On CH32, the handler ignores
unknown IRQ without clearing the source; a mistakenly enabled level interrupt may cause a storm
interrupt.

## 10. Important Technical Detection

### Severity/High

1. FU2 not yet deployable: address and peripheral behavior remain placeholder.
2. No secure boot: Any image with correct CRC is accepted.
3. Final chunks may exceed partitions: The bootloader checks for `size <= code_size` but always rounds
   the last program chunk to 256 bytes. If the image is nearly full of partitions and not divisible
   by 256, padding may exceed the limit while erasing using only the correct size. Check for
   `align_up(size, 256) <= code_size` or write only to the last word needed.
4. **Hidden flash error:** every API pays `void` and can poll BUSY permanently.
5. Handoff application not tight enough: missing SP/PC check and cleaning timer/interrupt status of
   core.

### Average level

6. OTA metadata has only one erase-sensitive record; two record/page, incremental sequence and commit
   marker should be used.
7. OTA API does not check null and does not validate slot/state. value domain
8. Wire format uses C struct/little-endian directly; needs static assert size/offset and serialize LE
   explicitly.
9. Update mode without timeout after SYNC or retry/abort. mechanism
10. GPIO toggle no thread-safe/ISR-safe.
11. Return value of `xTaskCreate()` is ignored.
12. Startup does not run the C/C++ constructor.
13. `SystemCoreClockUpdate()` also assumes a constant; replacing PLL can cause errors in baud, tick and
    delay without giving an error.

### Strengths

- Clear layering between hardware description, generated artifact, driver and application.
- Application uses the common API; implementation is selected at build time.
- Bootloader architecture glue separates Cortex-M and WCH from common OTA logic.
- Asymmetric OTA keeps factory image recovery stable.
- Comment saved many practical hardware lessons such as order erase/ACK and handler routing of
  FreeRTOS M4F.

### Technical debt

- Comment too long, mixing history with current contract.
- Some comments point to internal plan/memory not in the repository.
- The STK/PFIC CH32 constant is repeated between bootloader and example FreeRTOS.
- No test suite, CI, simulator harness or static analysis for project code.
- Generated files and VSIX/dist packages increase noise when reviewing if there is no check
  tra reproducibility.

## 11. File Map

| Group files | Our Responsibilities |
|---|---|
| `README.md`, `ARCHITECTURE.md` | Overview and design reasons |
| `SETUP.md`, `VSCODE.md`, `guide/*.md` | Environment, onboarding, YAML, drivers, Kconfig, RTOS |
| `neon-mdf/CMakeLists.txt`, `Kconfig` | Original SDK Composition |
| `cmake/toolchain/*.cmake` | Find compiler, objcopy, size and bare-metal specs |
| `cmake/arch/*.cmake` | CPU/ISA/FPU Flag |
| `cmake/mcu_variants.cmake` | Registry target and call generator |
| `cmake/kconfig.cmake` | create `sdkconfig.h`, target menuconfig |
| `cmake/flashmap.cmake` | Select CSV by target and enable OTA |
| `cmake/flash_targets.cmake` | Packing and flash JTAG/UART/OTA |
| `cmake/freertos.cmake` | Choose FreeRTOS port according to architecture |
| `tools/cmake/project.cmake` | Entry point for project application |
| `mcu-description/schema/**` | Contract YAML |
| `mcu-description/devices/*.yaml` | Memory, peripheral, IRQ and boot policy |
| `mcu-description/tools/mcu_desc/*.py` | Object model, parser and validator |
| `mcu-description/tools/backends/**` | Create header/startup/linker/SVD |
| `mcu-description/tools/generate.py` | CLI and backend dispatcher |
| `arch/CortexM/**/startup_*.s` | Reset, data/BSS, vector and weak handler |
| `arch/CortexM/**/linker/*.ld` | Layout whole-flash/boot/app |
| `arch/CortexM/**/system_*.c` | FPU, VTOR and handwritten clock setup |
| `arch/CortexM/CMSIS/` | CMSIS-Core provides NVIC and PRIMASK intrinsics for direct use |
| `arch/RISCV/Core/**` | CSR, soft-math and libc minimum shim |
| `arch/RISCV/**/startup_*.s` | gp/sp, C runtime, CSR, PFIC and `mret` |
| `arch/RISCV/Port/sdk_irq.*` | PFIC and critical section |
| `components/driver/neon_types.h` | Shared code status |
| `components/driver/gpio/*` | API GPIO and three implementations |
| `components/driver/flash/*` | API flash and implementation WEACT/CH32 |
| `components/driver/uart/*`, `i2c/*` | Driver polling placeholder FU2 |
| `components/bootloader/image_header.h` | Header image 16 byte |
| `neon_bootloader_crc32.*` | CRC-32 has no global state |
| `neon_bootloader_uart.*` | UART recovery by family |
| `neon_bootloader_arch.*` | Tick/reset/jump abstraction |
| `neon_bootloader_main.c` | Protocol, program image, slot selection, rollback |
| `components/ota/neon_ota.*` | Metadata and OTA confirmation |
| `tools/gen_flashmap.py` | create partition/header/linker from CSV |
| `tools/build_*image.py` | Packing image and metadata |
| `tools/neon_mdf.py` | CLI build/flash/monitor |
| `tools/templates*/**` | Scaffold project bare-metal/FreeRTOS |
| `examples/Blinky/**` | GPIO/Kconfig/OTA portable demo |
| `examples/FreeRTOS_Blinky/**` | Scheduler and specific port integration |
| `examples/OTA_Demo/**` | Factory, OTA confirmed and intentional rollback |
| `vscode-extension/src/**` | UI, CMake, flash, debug, monitor and project wizard |

CMSIS and FreeRTOS-Kernel are considered as upstream libraries. Ports not selected do not
participate in executable; analyzing each port file of every CPU will be a review of FreeRTOS
upstream, not reverse engineering firmware implementation of the project.

## 12. Important Call Graph

```text
Reset_Handler
+-- SystemInit
`-- main
    +-- bare-metal
    |   +-- neon_gpio_clock_enable
    |   +-- neon_gpio_init
    | `-- loop -> TogglePin -> [options] OTA_ConfirmValid
    +-- FreeRTOS
    |   +-- GPIO init
    |   +-- xTaskCreate x2
    |   `-- vTaskStartScheduler
    |       +-- SysTick/STK -> xTaskIncrementTick
    |       +-- PendSV/trap -> vTaskSwitchContext
    |       `-- task -> GPIO/delay/OTA confirm
    `-- bootloader
        +-- UART_Init
        +-- SystickStart1ms / TryRecvByte
        +-- update -> Recv -> Flash erase/program -> CRC
        | `-- metadata -> reset or JumpToApp
        `-- normal boot -> metadata -> CRC -> attempt/rollback -> JumpToApp
```

Flash, UART polling, CRC full image and busy delay are all blocking. The driver/boot API is not
designed to be called from ISR. CRC does not use the static/global state, so it can be reentrant if
the input region is not changed simultaneously.

## 13. The learning order for new maintainer

1. Read YAML of target, generated header, startup and linker scripts in parallel.
2. Understand the application's `project.cmake`, `mcu_variants.cmake` and `CMakeLists.txt` to know
   which source actually goes into ELF.
3. Trace reset from startup through `SystemInit()` to correct `main()`.
4. For common application, learn GPIO/IRQ implementation and FreeRTOS routing.
5. Before fixing the update, it is necessary to understand simultaneously `image_header.h`,
   `neon_bootloader_main.c`, flash driver, `neon_ota.c`, `gen_flashmap.py` and host image builder.
   This is a unified protocol, not standalone files.

The safest changes are documents, host tooling with new test or drivers located behind the old API.
Be extremely careful with generator, startup, linker, erase geometry, bootloader handoff, metadata
layout and wire format: errors here can be successfully built but erase the wrong sector or lose
recovery path.

Before using production, it is necessary to have golden test for generator, serialize test
image/metadata, OTA machine state test on host, limit size from map file, warnings-as-errors, static
analysis, fault injection source loss at each point erase/program, fuzz UART malformed packet and
board-in-loop test for boot, update, rollback on each target.
