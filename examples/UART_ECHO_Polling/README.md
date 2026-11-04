# UART_ECHO_Polling — FU2 UART0 echo with log

Receive a byte in polling, print the log of the data sent by the user, then send it back and print
the log on the FPGA side. Defaults to UART0, 19200 baud, 8N1, FIFO enabled, 10 MHz FPGA clock. No
interrupt, DMA or FreeRTOS.

Type `A` on terminal will see:

```text
[USER -> FPGA] 'A' (0x41)
[FPGA -> USER] A (0x41)
```

The `A` character after the FPGA label is sent by `neon_uart_transmit()`. The hex labels/codes are
printed by printf. Each byte is processed immediately, no Enter or line buffer is required. For
non-printable bytes, the USER line displays `.` and hex codes; the echo byte remains the same so
CR/LF or the control character can make the pointer position change visible.

## Build and configuration

```sh
cd /home/quang-hoang/merge_neon_mdf/CDC_NEON_MDF_merged/examples/UART_ECHO_Polling
cmake --preset FU2
cmake --build --preset FU2
```

Open menuconfig:

```sh
cmake --build --preset FU2 --target menuconfig
```

Keep `components/driver → Compile UART driver` enabled. Menu `UART_ECHO_Polling` for baud
correction, fixed FPGA clock and RX/TX timeout. Clock must match board; timeout is in poll rings,
not milliseconds. Save with `S`, exit with `Q`, then re-run configure and build as above.

## flash and run

Appropriate PDI hardware flashes into the FPGA first. UART0 TX/PC0 and RX/PC1 according to FU2
mapping are routed to FTDI according to PDI; use the correct serial port. If USB-UART is connected
externally, connect TX ↔ RX and generic GND, confirm voltage according to board map.

The boot file is `build/FU2/uart_echo_polling_image.bin` (header ROM v5 128-byte + payload). File
`uart_echo_polling.bin` is only raw payload. See [FU2.md](../../guide/FU2.md).

On an FPGA connector, see the corresponding path/port and flash:

```sh
python3 /home/quang-hoang/fu2_fpga/uart_boot_loader.py --port /dev/serial/by-id/usb-Xilinx_VEK280_262502175696-if02-port0 --image /home/quang-hoang/merge_neon_mdf/CDC_NEON_MDF_merged/examples/UART_ECHO_Polling/build/FU2/uart_echo_polling_image.bin --baud 9600 --byte-gap-ms 2 --no-monitor
```

Run the loader first and hold SW5 for about 2 seconds according to the guideline. Baud BootROM 9600
in the above command is independent of baud app 19200; change if the PDI in use has a different
configuration.

After the loader is complete and the port is closed:

```sh
python3 -m serial.tools.miniterm /dev/serial/by-id/usb-Xilinx_VEK280_262502175696-if02-port0 19200 --raw
```

Do not add `--echo` to disable local echo. Type each character or short string and observe two
labels USER/FPGA. Exit with `Ctrl+]`. Banners may have passed before the monitor opens; two-way logs
still appear when typing.

Log increases the number of TX bytes, so type slowly and avoid sending long bursts that fill up RX
FIFO. Return stream has both echo and log bytes, not for comparing whole stream raw.

## The status

`g_uart_echo_stats` in the debugger holds the counter bytes RX/echo TX success, RX/TX timeout, error
and last error code. Do not count printf log bytes to `tx_bytes`. TX success means that the byte has
been written to THR; UART may still be playing that byte. Driver TX only waits for LSR[5] THRF=0,
does not wait for LSR[6] TEMT. RX timeout without data only increases the counter then resumes
polling, does not print log continuously. RX error and TX error are printed with label `[ERROR]`.

Only include `<stdio.h>` to use printf; do not call `neon_console_attach_uart()`. The first boot log
initializes the UART0 console at 19200 baud. The app waits for LSR[6] TEMT for the banner to finish
transferring before resetting the UART; this is the preparatory step for changing configuration,
which is not included in the TX polling line. Then the app reconfigures UART0 according to the
menuconfig and enables FIFO; the next echo and log share this hardware configuration. If the baud
app is changed, the first boot log remains at 19200 baud, while the banner ready and log RX/TX are
at the selected baud app. Log is the best-effort diagnostic; the counter TX monitors the main echo
byte, not the print error of each part of the log. Do not manually re-send data after the TX
timeout: with a multibyte buffer, previous bytes may have been written to the THR.

Buffer a `uint32_t` full-word owner byte, compliant with FU2 word-only SRAM. Build checks driver
archive and ELF to deny access to byte/halfword. Unverified transmission on real FPGA.
