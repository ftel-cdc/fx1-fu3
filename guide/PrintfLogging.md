# Export log using printf on GT32U2086

GT32U2086 has a compact `printf` set, outputting via the `_write` syscall hook, without newlib's heap or
stdio. The formatter uses 32-bit word access to match GT32U2086 SRAM. For GT32U2086, only the
`CONFIG_NEON_MDF_DRIVER_UART` flag enables both the UART driver and `printf/_write`.
There is no separate printf flag.

## Use in app

Build using the SDK's project helpers. The `<stdio.h>` header provides standard declarations; the
SDK provides implementations of `printf`, `vprintf`, `puts` and `putchar`. Enable UART in sdkconfig:

```text
CONFIG_NEON_MDF_DRIVER_UART=y
```

No need to manually write `_write()` or enable a different printf flag. When UART is turned off,
drivers and consoles are not compiled; the app that calls printf will report link errors. App only
needs to include `<stdio.h>` and then call `printf`. First output manually initialize UART0 at 19200
baud, 8N1, using the startup setup `SystemCoreClock`.

```c
#include <stdio.h>

int main(void)
{
    if (printf("[INFO] counter=%u status=0x%08lX\n", 1U, 0x1234UL) < 0) {
        /* Log failure; can be checked by debugger. */
    }
    for (;;) { }
}
```

The startup clock must match the board. For example, `Printf_Log` uses a 10 MHz fixed FPGA clock.
The console manually turns on/reset UART0 and configuration baud at the first write; it does not
automatically change the system-wide clock policy.

If another UART/baud is needed, it can be include `console/neon_console.h`, initialize a `static`
handle according to the application configuration and invoke `neon_console_attach_uart()` before
printing. This API is optional; only accepts READY handles, 8-bit word lengths, and non-zero
timeouts. The handle must exist for the duration of the attach. Invoke `neon_console_detach_uart()`
before deinit that handle; next print returns to default UART0. Other UART0 transactions must
coordinate with the console; the first printf can reset UART0 if the application handle has not been
attached yet.

## Mechanism of operation

1. when the family is GT32U2086 and `CONFIG_NEON_MDF_DRIVER_UART=y`, CMake puts
   `components/console/platform/gt32u2086/neon_console.c` and
   `components/driver/uart/platform/gt32u2086/neon_uart.c` into `neon_mdf_driver` library. There is no
   separate printf flag.
2. `<stdio.h>` declares `printf`. when the app calls a function, the linker takes the SDK
   implementation from the driver library. GT32U2086 links to `-nostdlib` and uses the SDK formatter, not
   newlib's stdio formatter.
3. `printf` collect arguments using `va_list`, call `vprintf`, then formatter analyze `%s`, `%d`, `%x`,
   width/precision… and generate each character.
4. `emit` puts the character in a 32-bit aligned word, then calls `_write(1, data, 1)`. `_write`
   initializes UART0 itself if no handle console is available, changes LF to CRLF, and calls HAL
   UART. stdout (`file=1`) and stderr (`file=2`) share the same console.
5. Driver UART polling for TX ready, write character to register THR; hardware UART outputs bits to the
   TX pin according to the configured baud. USB-UART and Serial Monitor receive byte streams and
   display them as text.

`neon_console_attach_uart` is the optional way to see the default UART, not the required step before
printf. `_write` is the console's general output layer. Functions not used by the app can be removed
from the firmware by the linker using `--gc-sections`; with UART enabled, the printf API is
available without additional flags.

### Syscall hook on bare-metal

With newlib, the output stream is usually `printf → stdio → _write → UART`. `_write` is a C function
whose firmware is the same level as the C library; on bare-metal it does not necessarily call the
kernel or issue SVC commands. It can call HAL UART or polling and write straight register TX,
depending on the backend.

GT32U2086 uses its own formatter because SRAM only supports word aligned access. Only `_write` can be
found, but not byte/halfword in newlib's formatter, buffer and backend functions. Current
implementation is
`printf → vprintf → formatter word-only → _write → HAL UART → TX`.
The app does not need to call drivers; inside the SDK, `_write` reuses HAL to manage
clock/reset/pinmux/baud and timeout, without repeating register logic.

Direct call `_write` returns the number of bytes input consumed, may partially count when UART fails
after sending data, or `-1` when no bytes have been sent. `neon_console_get_status()` explains the
error; this bare-metal backend does not set up newlib's errno. The buffer transmitted to `_write`
must own all word aligned containing it, like other GT32U2086 buffers.

## Format and behavior

| Supports | Examples |
| --- | --- |
| String, Character, Mark `%` | `%s`, `%c`, `%%` |
| Integer signed/unsigned | `%d`, `%i`, `%u` |
| Hex and octal | `%x`, `%X`, `%o`, `%08lX`, `%#x` |
| pointer | `%p` — always prefixed with `0x`, including pointer null |
| Integer length | `hh`, `h`, `l`, `ll`, `z` |
| Margins, marks, padding | `-`, `+`, space, `#`, `0` |
| Width/precision | `%8.3s`, `%*.*d`; each field max 1024 |

No floats are supported (`%f/%e/%g/...`), `%n`, wide characters, positional arguments or the entire
stdio library (`fprintf`, `snprintf`, `scanf`, `fflush`, ...). No `setvbuf()` is needed instead of
`fflush()`: each character is sent immediately using UART polling. Do not add newlib stdio to extend
these functions on GT32U2086, because those lines do not guarantee word access to SRAM.

LF (`\n`) changes itself to CRLF on UART; existing string `\r\n` is not added CR again.
`printf/vprintf` returns the number of logical characters before converting newline. when
encountering unsupported format or UART error, returns `-1`; log header may have been sent.
`neon_console_get_status()` returns `NEON_NOT_SUPPORTED`, `NEON_INVALID_PARAM`, `NEON_BUSY`,
`NEON_TIMEOUT` or corresponding driver error code. A successful call follows a status update; status
is not permanently saved error.

timeout is the budget poll for each UART byte, not milliseconds or deadline of the entire log
stream. Longer logs may take longer to occupy the main. The console rejects a nhau/concurrent nested
call with BUSY; no lock waiting. In FreeRTOS, use mutex or a task log for serialization. In ISR,
give data/events to main/task for processing. If other code also uses that UART or directly calls
`_write`, the application must protect the entire transaction to avoid data interspersal between log
characters.

Format strings and `%s` in SRAM must own all word aligned data, according to `neon_utils_memory.h`.
Buffers that DMA is updating need to be handed over ownership before printing. literal strings in
Flash are read with word aligned. Call printf from main/task after startup/SystemInit. Not used in
reset/ISR before platform ready.

## Build and view logs

View the complete project at [examples/Printf_Log](../examples/Printf_Log/README.md). SDK has Serial
Monitor in VS Code; select the correct port, baud 19200 and 8N1 for this example. Can be opened
directly with pyserial:

```sh
python3 -m serial.tools.miniterm /dev/ttyUSB0 19200
```

Connect UART0 TX of the board to RX of USB-UART, general GND; confirm pins and voltage level in
board map. Do not infer 3.3 V/5 V tolerance from UART name. Reset after opening monitor to see boot
log; exit with `Ctrl+]`.

## Confirmation Status

Checked build when UART is on/off, format integer 64-bits, line `_write`, initialize UART
automatically, down to CRLF and restore after timeout. Runtime GT32U2086 uses memory access functions in
word 32-bits. No baud, UART pin or log confirmation on real FPGA/silicon.
