# Debugging in VS Code: implementation and hardware findings

`vscode-extension`'s command **NEON-MDF: Debug** completely replaces the traditional
`.vscode/launch.json` — without configuration debug and for each project, the extension itself reads
executable/target via CMake File API. This file explains the mechanism below + 2 real bugs have been
encountered and fixed on hardware, so as not to relapse when editing `debug.ts` / adding a new
architecture.

## Support Status

**Only Cortex-M** (`FU2`/`WEACT_F412`, via OpenOCD/SWD) works properly — `CH32V003F4P6` (RISC-V) **not
wire** (needs WCH's own OpenOCD fork, hasn't confirmed working — see `README.md` limit entry).
Selecting target RISC-V and clicking Debug will throw obvious errors (`debugArch.ts`'s
`resolveDebugArchProfile()`), not silently using the wrong ARM configuration.

## Process when pressing "NEON-MDF: Debug"

1. Make sure configured (`ensureConfigured`), read the actual executable list in build dir via CMake
   File API (`cmakeFileApi.ts`) — more than 1 executable then ask for selection via QuickPick.
2. **Flash BEFORE with the correct target CMake** (`flash_jtag` for factory, `flash_ota` for OTA) — DO
   NOT let cortex-debug flash itself using GDB's original `load` command (see "Gotcha #1" section
   below). Select the correct target CMake via the actual manifest
   `<builddir>/neon_mdf_debug_targets.txt` (recorded by `cmake/flash_targets.cmake`, line
   `factory=<name>`/`ota=<name>`/ `bootloader=<name>`) — DO NOT guess whether the executable name
   ends in `_ota` (old way, real bug: `examples/OTA_Demo`'s `ota_demo_ota_bad` shares an OTA slot
   but the name does not end in `_ota`, fixed 2026-08-11). An executable absent from the manifest is explicitly rejected instead of guessing its partition. Only fallback on guessing by
   name when the target has no bootloader (no manifest, e.g. `FU2`).
3. Derived files `.cfg` openocd (`cmake/openocd/<target>_openocd.cfg`) and `.svd`
   (`mcu-description/generated/<target>.svd`) in accordance with conventional name — without manual
   configuration.
4. Start the debug session (`vscode.debug.startDebugging`) with
   `servertype: "openocd"`, `runToEntryPoint: "main"`,
   `preLaunchCommands`/`postLaunchCommands` taken from `debugArch.ts` (item
   "Gotcha #2").

## Gotcha #1 — GDB `load` erases `image_header_t`

**The real symptom was encountered**: debug running but NOT stopping at `main()`, step tells `"Cannot
find bounds of current function"` — looks like it is debugging another code.

**Real cause** (confirmed by direct flash reading with SWD before/after): GDB `load` writes only the
right ELF content (vector table + code, starting from `<partition>_VECTOR_BASE`) — but
`image_header_t` (16 bytes magic/size/ crc32, located BEFORE the vector table, 1 flash sector with
code) is not part of ELF (host tooling adds itself at build blob to flash, see
`guide/BootloaderOTA.md`). Flash writing requires erasing all sectors before writing — `load` erased the header without restoring it (does not know it exists) → header to `0xFFFFFFFF` → bootloader found invalid
header → did not dare jump into app → CPU stuck in bootloader, breakpoint set according to symbol of
app.elf never match.

**Fix**: flash first with real CMake target (step 2 above, keep header intact) + `loadFiles: []` in
configuration debug (`debug.ts`) to make cortex-debug NOT automatically `load` again. **NOTE**: must
be `[]` (empty array), NOT `false` — read directly source cortex-debug: `false` still runs normally
`target-download`, only empty array/list actually skips step
download.

## Gotcha #2 — `monitor reset halt` unreliable

**The symptoms were real**: sometimes halt AFTER the first few commands of `Reset_Handler` (SP no longer at the top of RAM), inconsistency between attempts — despite the same command `monitor reset
halt`.

**Fix** (`debugArch.ts`'s `CORTEX_M_PROFILE`): turn on `DEMCR.VC_CORERESET` (bit0 of `0xE000EDFC`,
ARMv7-M specification) BEFORE debug the adapter resets itself:

```
preLaunchCommands:  ["monitor halt", "monitor mww 0xE000EDFC 0x00000001"]
postLaunchCommands: ["monitor mww 0xE000EDFC 0x00000000"]   # Disable reset vector catch for subsequent resets
```

This is the real hardware mechanism of the CPU itself (Cortex Debug control Block) — The CPU
automatically enters the Debug state at the moment of retrieving the reset vector, BEFORE running
any command of `Reset_Handler`, regardless of whether the reset is triggered by any mechanism
(physical SRST or software AIRCR). — does not depend on OpenOCD's polling/timing as does `monitor
reset halt`. Verified 4/4 iterations give identical results (SP at the top of RAM).

## `debugArch.ts` — CPU architecture profiles

```ts
interface DebugArchProfile {
  debugType: string;              // "cortex-debug"
  preLaunchCommands: string[];
  postLaunchCommands: string[];
}
```

Separate the CPU architecture specification (`debugType`, workaround reset — register `0xE000EDFC`
is ARMv7-M System Control Space, does NOT exist on RISC-V) from the general architecture
specification in `debug.ts` (read executable via CMake File API, path convention `.cfg`/`.svd`,
flash-first-and-attach to avoid Gotcha #1 — problem of this header/flashmap SDK design itself, not
ARM specification).

Add RISC-V profile when a real CH32V003F4P6 board is available + the debug tool (WCH OpenOCD fork or another tool) has been verified — write new `RISCV_PROFILE` in `debugArch.ts`, register additional branches
in `resolveDebugArchProfile()`. RISC-V Debug Spec uses its own Debug Module
(`dmcontrol`/`dmstatus`...) — address/semantics are completely different from `DEMCR`, do not copy
Cortex-M workaround to it.

## Troubleshooting

| Symptoms and signs | Inspection |
|---|---|
| "Cannot find bounds of current function" as soon as debug | Most likely Gotcha #1 — confirms `loadFiles: []` (not `false`) also in `debug.ts`. |
| Debug stops at unknown address, SP is not at the top of RAM | Gotcha #2 — confirms `preLaunchCommands` has written `0xE000EDFC`. |
| "not found openocd cfg file" | Target selected does not have this file `cmake/openocd/<target>_openocd.cfg` — RISC-V currently does not have this file (see "Support Status"). |
| Could not start debug session | Extension `marus25.cortex-debug` not installed. |
