# GPIO_Input_Output — testcase 2 for FU2

Read buttons at GPIOB_5 (PB5) and LED control at GPIOB_1 (PB1):
**press on, release off**. Main loop reads GPIO directly. `printf` reports status via UART0, 19200
baud, 8N1.

## Connection and operation

| Signals | configuration in main.c |
|---|---|
| PB5 | Input pull-up, press=0, release=1 |
| PB1 | Output active-high: press=1, release=0; start at 0 |
| UART0 | Log `printf`, keep PC0/PC1 for TX/RX |

The button connects the PB5 to the GND. If FPGA/PDI does not pull-up of the PAD, use a pull-up
resistor outside in accordance with the bank voltage. The PB1 can connect the LED to the resistor or
measuring device; the header pin depends on the mapping of the flashing PDI. Pins and logic levels
are written directly in `Src/main.c` for easy readability and correction.

PB1 starts at 0 level. when main read PB5 by 0, PB1 to 1 to turn on the LED; when PB5 by 1, PB1 to 0
to turn off the LED. Log only prints when the input level sees a change. For example, without
anti-rebound, so the mechanical button can create multiple logs at press / release.

## Build and configuration

```sh
cd /home/quang-hoang/merge_neon_mdf/CDC_NEON_MDF_merged/examples/GPIO_Input_Output
cmake --preset FU2
cmake --build --preset FU2
```

Change clock by:

```sh
cmake --build --preset FU2 --target menuconfig
cmake --preset FU2
cmake --build --preset FU2
```

configuration clock is in the `GPIO_Input_Output` menu. Default FPGA clock 10 MHz; true clock
matching required for correct UART baud. Clock value 0 uses SDK startup clock. Keep UART driver on
to use `printf` with `<stdio.h>`; application does not need to call `neon_console_attach_uart()` or
manually initialize UART. Change pin, pull or active level by editing `Src/main.c` directly.

## flash and view log

The flash file is `build/FU2/gpio_input_output_image.bin`, which has a 128 byte boot header.
`gpio_input_output.bin` is only payload raw, not directly used with the loader.

On an FPGA connector, replace the serial port and path for that machine:

```sh
python3 /home/quang-hoang/fu2_fpga/uart_boot_loader.py --port /dev/serial/by-id/usb-Xilinx_VEK280_262502175696-if02-port0 --image /home/quang-hoang/merge_neon_mdf/CDC_NEON_MDF_merged/examples/GPIO_Input_Output/build/FU2/gpio_input_output_image.bin --baud 9600 --byte-gap-ms 2 --no-monitor
```

Run the loader and reset it according to the board procedure. When the flash is complete, open the terminal:

```sh
python3 -m serial.tools.miniterm /dev/serial/by-id/usb-Xilinx_VEK280_262502175696-if02-port0 19200 --raw
```

Press and release the PB5 button. PB1 must be up 1 when pressed and down 0 when released, and log
output:

```text
[INFO] PB5 button -> PB1 LED
[GPIO] RELEASED -> LED OFF
[GPIO] PRESSED -> LED ON
[GPIO] RELEASED -> LED OFF
```

Banner and initial status may have passed before opening terminal; press/release button to receive
new log. Exit miniterm with `Ctrl+]`.

## Notes

Do not use SysTick, interrupt GPIO, delay, DMA or FreeRTOS. Main reads the node and writes output
with `neon_gpio_read_pin()` / `neon_gpio_write_pin()`. While UART prints log, main waits; loop
continues to read the node after printf returns.

Keep PC0/PC1 for UART0 if fixing GPIO pins. Confirm on real FPGA with log same state PB1/LED; build
results do not confirm line and pad.
