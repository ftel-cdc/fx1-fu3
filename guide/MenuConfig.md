# Optional declaration `menuconfig`/`sdkconfig`

Instructions for adding 1 toggle options via `menuconfig` — are the same as ESP-IDF's
`Kconfig`/`sdkconfig` system (reused with `kconfiglib`, no parser). See [Architecture](Architecture.md) for a design overview; this file goes into the option declaration syntax + usage.

## General Models

```
neon-mdf/Kconfig                      # mainmenu SDK - rsource each component
  └─ components/driver/Kconfig         # option OF SDK
  └─ osource "$(NEON_MDF_APP_KCONFIG)" # option OF APP (if app has its own Kconfig)

<app>/Kconfig     # OPTIONS - separate app options, automatically merged into THE SAME menu
<app>/sdkconfig   # Selected configuration (text file, versioned with the application)
        │ (cmake/kconfig.cmake -> tools/kconfig/gen_config_header.py, run at configure)
        ▼
build/<TARGET>/config/sdkconfig.h      # #define CONFIG_X ... - used in C code
build/<TARGET>/config/sdkconfig.cmake  # set(CONFIG_X ...) - used in CMakeLists.txt
```

Option SDK (`components/driver/Kconfig`) and option app (`<app>/Kconfig`, if available) ** combine 1
menu, 1 `sdkconfig` file** — app does not need to know anything about the merge mechanism, just
create a `Kconfig` file next to your `CMakeLists.txt` (see section 3).

## 1. 1 option declaration syntax

Standard Kconfig syntax (identical to Linux kernel/ESP-IDF/Zephyr — `kconfiglib` read directly, no
separate syntax of this SDK):

```kconfig
menu "current menu name"

config TEN_OPTION
    bool "open ta shortcut shown in menuconfig"
    default y
    help
        open us long, this option stage is nothing, the image direction so.

config TEN_OPTION_SO
    int "Number of loop iterations ..."
    default 500000
    help
        ...

endmenu
```

| Type | Syntax `default` | generated in `sdkconfig.h` | generated in `sdkconfig.cmake` |
|---|---|---|---|
| `bool` | `default y` \| `default n` | `#define CONFIG_TEN_OPTION 1` (only if `y`, NO `#define` if `n` — use `#ifdef`, not `#if`) | `set(CONFIG_TEN_OPTION ON)` \| `OFF` |
| `int` | `default 500000` | `#define CONFIG_TEN_OPTION 500000` | `set(CONFIG_TEN_OPTION 500000)` |
| `hex` | `default 0x1000` | `#define CONFIG_TEN_OPTION 0x1000` | `set(CONFIG_TEN_OPTION 0x1000)` |
| `string` | `default "abc"` | `#define CONFIG_TEN_OPTION "abc"` | `set(CONFIG_TEN_OPTION "abc")` |

The option name in Kconfig (`TEN_OPTION`) **no prefix** — macro/variable generated automatically
adds `CONFIG_` (identical to ESP-IDF convention). Name the SDK option with a prefix `NEON_MDF_`
(e.g. `NEON_MDF_DRIVER_UART`) so that it does not match the app's option; app options do not need
this prefix (e.g. `BLINKY_DELAY_LOOPS`).

Real example (`components/driver/Kconfig`):

```kconfig
menu "components/driver"

config NEON_MDF_DRIVER_UART
    bool "Compile the UART driver (uart/neon_uart.c)"
    default y
    help
        warn UART driver gathering in build. disable to reduce Flash usage if app does not use UART - in
        that case uart/neon_uart.c is not compile.

endmenu
```

## 2. Used in C code

```c
#include "sdkconfig.h"

#if defined(CONFIG_NEON_MDF_DRIVER_UART)
  /* only translate variables when option BOOL is enabled */
#endif

#define DELAY_LOOPS CONFIG_BLINKY_DELAY_LOOPS /* option INT use directly as the number */
```

`sdkconfig.h` is included in include path (target `neon_mdf_kconfig`, already linked to
`neon_mdf_driver` PUBLIC — all `neon_mdf_driver` app links are `#include "sdkconfig.h"` right away,
no need to add anything in the `CMakeLists.txt` of the app).

## 3. Used in `CMakeLists.txt` (power file on/off)

```cmake
if(CONFIG_NEON_MDF_DRIVER_UART)
  list(APPEND _neon_mdf_driver_sources uart/neon_uart.c)
endif()
```

The `CONFIG_*` variable is valid only AFTER `neon_mdf_process_kconfig()` (`cmake/kconfig.cmake`) has
run — in `components/driver/CMakeLists.txt`. This is always possible because
`neon-mdf/CMakeLists.txt` calls this function BEFORE `add_subdirectory(components/driver)` (see
`guide/ArchitectDetail.md` section 4 for full order). This is the mechanism `components/driver` uses
to NOT compile a file `.c` when the option is disabled (see `guide/DriverImplement.md` section 3,
dispatch type (c)) — does not "hide feature at runtime", the app that calls the disabled function
will have a clear link error (`undefined reference`), not a hidden error.

## 4. Add new option for SDK (component available)

Straighten `components/driver/Kconfig` (add 1 new block `config ...` in the existing `menu`). No
need to edit `neon-mdf/Kconfig` (this file is already `rsource`).

## 5. Add Kconfig to app (option of 1 example)

Create `<app>/Kconfig` next to `<app>/CMakeLists.txt` — **don't need to edit the app's
`CMakeLists.txt`** to "enable" this feature (same as `osource` = Make's `-include` style, auto merge
if file exists, silent ignore otherwise — old behavior of app without this file stays 100%):

```kconfig
menu "App name (this app)"

config MY_APP_SOME_OPTION
    int "Option description"
    default 1000
    help
        ...

endmenu
```

read in `Src/main.c` via `#include "sdkconfig.h"` + `CONFIG_MY_APP_SOME_OPTION` identical to SDK
option — can not distinguish (and does not need to distinguish) which option comes from the SDK or
app in code using it.

## 6. Open menuconfig — edit value

```sh
cd examples/Blinky
cmake --build --preset GT32U2086 --target menuconfig   # open UI select (same as idf.py menuconfig)
cmake --preset GT32U2086                                 # mandatory re-configure after saving to apply
```

Or via VS Code: command **NEON-MDF: menuconfig**. Or CLI: `neon_mdf.py menuconfig`. All 3 paths call
the same target CMake `menuconfig` (`cmake/kconfig.cmake`) — without any other logic in UI layer.

It is also possible to edit manually `<app>/sdkconfig` (text file, like ESP-IDF) and then `cmake
--preset <TARGET>` again — for example disable UART:

```
# CONFIG_NEON_MDF_DRIVER_UART is not set
```

`sdkconfig` records itself every time it configures (resembles ESP-IDF): new options added to
Kconfig autofill `default` into the file, options that no longer exist in Kconfig (deleted from
`.Kconfig` file) are automatically removed from `sdkconfig` — no manual removal of this file is
required when changing the option structure.

## Checklist added a new option

1. Determine if it belongs to the SDK (fixing `components/driver/Kconfig`, or a new `Kconfig` file for
   another component + adding `rsource` in `neon-mdf/Kconfig`) or if it belongs to an app alone
   (`<app>/Kconfig`, automatically merged, no further fixes).
2. Choosing the right `type` (`bool`/`int`/`hex`/`string`) — decides usage in C (`#ifdef` for `bool`,
   direct value for `int`/`hex`/`string`).
3. Set rational `default` — app build for the first time (without `sdkconfig`) use YES this value, not
   empty/0.
4. If used to enable/disable compile 1 file `.c` — add `if(CONFIG_...)` in the right place in the
   relevant `CMakeLists.txt` (see section 3).
5. `cmake --preset <TARGET>` back (configure) so that `sdkconfig.h`/`.cmake` generated Kconfig matches
   new — old build (`build/`) does not update itself.
