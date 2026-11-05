# Add bootloader/OTA to app

Practical guide to add real OTA to an app — write `flashmap.csv`, edit `CMakeLists.txt`, call API in
code, and how to test real factory cycles→OTA→rollback. See `ARCHITECTURE.md` section 7-8 for design
reasons, `guide/DeviceYAML.md` section `bootloader:`/`erase_sectors:` for YAML field.

**This file is the most risky component in SDK** (real board brick 2 times in project history). — do
step by step, verify each step with real board before trusting, don't just trust clean build.

## 0. When SHOULD use OTA — check the budget first

OTA needs 2 full image copies (factory + OTA) at the same time in flash, plus `otadata`. Before
writing `flashmap.csv`, estimate:

```
bootloader.size + 2 × (image_header_size + actual_app_size) + otadata_size ≤ physical flash size
```

Real lessons from `CH32V003F4P6`: FreeRTOS-Kernel minimum has taken up ~8.5KB flash (real
measurement, see comment CMakeLists.txt) — with 16KB chip, 2 copies do not have enough space for the
app `FreeRTOS_Blinky` separately (without `flashmap.CH32V003F4P6.csv`). This is the decision
according to each app, not the limit of both target — `examples/Blinky` (bare-metal, no FreeRTOS) on
this same chip still have real OTA (12KB after bootloader). SDK does not force flashmap for all apps
with target — bootloader is still real (verify header/CRC32, get firmware via UART) even if app has
flashmap please do not, only 1 fixed slot (not A/B) please 2 slot. If budget is not enough, do not
create `flashmap.<TARGET>.csv` for that app (see item 2) — is not error, is valid architecture
choice.

## 1. Prerequisites

The build device must have `bootloader:` in YAML (`WEACT_F412`/ `STM32F103CB`/`CH32V003F4P6` already
exists, `FU2` not yet — build will `FATAL_ERROR` clear if mistakenly calling OTA helper for target
without bootloader).

## 2. Write `flashmap.csv`

Place next to the app's `CMakeLists.txt` (or `flashmap.<TARGET>.csv` if the app build for many other
devices flash size — `neon-mdf/CMakeLists.txt` prioritize files with correct suffix
`NEON_MDF_TARGET` first, fallback on general `flashmap.csv` if not available):

```
# name,     type, subtype, size
app,        app,  factory, 0x8000     # slot factory - no OTA bi timers used
otadata,    data, ota,     0x10000    # OTA status - need raw 1 sector
app_ota,    app,  ota,     0x60000    # OTA slot - only OTA recorded
```

Required correct 1 partition name `app` (`type=app,subtype=factory`) — the order of lines does not
matter (see `guide/DeviceYAML.md`). size per `type=app` partition already includes "slot"
`image_header_size` (see `bootloader.image_header_size` in YAML device) — not just code.

## 3. Fix `CMakeLists.txt` — build 2 (or N) copies from SAME 1 `main.c`

Minimum sample (2 copies, varieties `examples/Blinky/CMakeLists.txt`):

```cmake
neon_mdf_add_bootloader_executable(bootloader)

function(_myapp_add_variant name linker_script version)
  add_executable(${name} Src/main.c)
  target_link_libraries(${name} PRIVATE neon_mdf_driver neon_mdf_ota)
  target_compile_definitions(${name} PRIVATE NEON_OTA_APP_VERSION=${version})
  target_link_options(${name} PRIVATE -T${linker_script} -Wl,-Map=$<TARGET_FILE:${name}>.map)
  set_target_properties(${name} PROPERTIES LINK_DEPENDS ${linker_script})
  add_custom_command(TARGET ${name} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${name}> $<TARGET_FILE:${name}>.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${name}>
  )
endfunction()

set(_myapp_ld_dir ${CMAKE_BINARY_DIR}/flashmap)
_myapp_add_variant(myapp     ${_myapp_ld_dir}/app.ld     1)
_myapp_add_variant(myapp_ota ${_myapp_ld_dir}/app_ota.ld 2)

include(${NEON_MDF_PATH}/cmake/flash_targets.cmake)
neon_mdf_add_flash_targets(bootloader myapp ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
neon_mdf_add_ota_flash_target(myapp_ota ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
```

The above template uses `add_executable()` directly to select the linker script of each partition.
The SDK does not automatically retarget `printf()` to UART on `WEACT_F412`; UART logging via
`printf()` is currently not available on this target. See `examples/OTA_Demo/CMakeLists.txt` for how
to create full OTA variants.

`${_myapp_ld_dir}/app.ld`/`app_ota.ld` generated automatically from `flashmap.csv` at configure —
not handwritten.

## 4. Call API in `Src/main.c`

```c
#include "neon_ota.h"

int main(void)
{
  /* ... initialize GPIO/peripheral... */

  uint32_t loop_count = 0;
  for (;;) {
    /* ... main application logic ... */
    loop_count++;
    if (loop_count == 10u) {   /* after a period of stable operation */
      neon_ota_confirm_valid();
    }
  }
}
```

`neon_ota_confirm_valid()` (`components/ota/neon_ota.h`) MUST be called by the app itself after it
has been running smoothly (not a bootloader of its own making) — bootloader has no way to know if
the app is "running well" other than waiting for the app to confirm. Calling too soon (not enough to
detect real errors) deactivates the safety of the rollback mechanism; calling too late / not calling
→ automatically rollbacks to the factory after `NEON_OTA_MAX_BOOT_ATTEMPTS` (default 3) unconfirmed
resets.

`NEON_OTA_APP_VERSION` (macro `target_compile_definitions` in section 3) only lets the code
distinguish itself from which copy is running (e.g. change the LED/in log cycle) — does not affect
the bootloader's OTA logic.

## 5. Flash first time — flash factory

```sh
cmake --preset WEACT_F412
cmake --build --preset WEACT_F412
cmake --build --preset WEACT_F412 --target flash_jtag
```

`flash_jtag` write bootloader + slot `app` (factory) + reset `otadata` to
`active_slot=0,state=VALID` in SWD — session after this command the board ALWAYS runs on the flashed
factory, regardless of previous OTA status (see `ARCHITECTURE.md` entry 7, this policy was a source
of real bugs before being added).

## 6. Real OTA test (emulate via SWD — without real network)

```sh
cmake --build --preset WEACT_F412 --target myapp_ota   # build OTA (version 2)
cmake --build --preset WEACT_F412 --target flash_ota
```

`flash_ota` write `[header+myapp_ota]` to slot `app_ota` + otadata
`active_slot=1,state=PENDING_VERIFY` — bootloader next boot will jump in
slot OTA. Verify:

```sh
# UART (if app has printf/log)
python3 -m serial.tools.miniterm /dev/cu.usbmodemXXXX 115200

# or read otadata directly via SWD (more accurate, not timing dependent UART)
openocd -f cmake/openocd/WEACT_F412_openocd.cfg -c init -c "reset halt" \
  -c "mdw <NEON_MDF_FLASHMAP_OTADATA_BASE>" -c shutdown
```

`otadata` is 5 consecutive 32-bit words: `magic`(=`0x4154414F`)/`active_slot`/
`state`(0=VALID/1=PENDING_VERIFY/2=INVALID)/`boot_attempts`/`crc32`. After the app itself
`ConfirmValid()`, `state` must switch to `0` (VALID) — this is the surest proof, independent of
reading at the time UART printed out.

## 7. Real auto-rollback test

Build 1 IDE never confirms (see `examples/OTA_Demo`'s `NEON_OTA_SKIP_CONFIRM` as a template — 1
additional `target_compile_definitions`, `#if !defined(NEON_OTA_SKIP_CONFIRM))` wraps around the
`ConfirmValid()` call). For example there is an existing target `flash_ota_bad` to write that build.
Once loaded, manually reset the board to the unconfirmed boot time of `NEON_OTA_MAX_BOOT_ATTEMPTS +
1` (default: 4th time) — the last time you should see the board go back to running the **factory**
build, and read `otadata` through SWD seeing `active_slot=0`.

## 8. Common Errors

| Symptoms and signs | Causes |
|---|---|
| Configure error `FATAL_ERROR...not declared bootloader:` | Target building without `bootloader:` in YAML (e.g. `FU2`) — change target. |
| `flash_ota` missing alarm `NEON_MDF_FLASHMAP_APP_OTA_BASE`/`OTADATA_BASE` | App does not have a valid `flashmap.csv`, or calls `neon_mdf_add_ota_flash_target()` before `flashmap.csv` is processed. |
| Linker error region FLASH overflow in OTA version | The flash budget is not enough for 2 copies (see item 0) — reduce the code, or remove OTA for that device (item 0). |
| Board does not rollback despite repeated resets | Check that you have actually built a `NEON_OTA_SKIP_CONFIRM` (not a regular one) before `flash_ota`. |
| `flash_jtag` is done and the board still runs the old OTA | Using old `flash_jtag` without reset step `otadata` — update SDK to the version with policy in item 5 (`ARCHITECTURE.md` item 7). |
