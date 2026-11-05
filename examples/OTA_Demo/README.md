# ⁇  OTA_Demo on STM32F103CB Blue Pill

## Connectivity

- ST-Link: `SWDIO`, `SWCLK`, `GND`, `3V3`.
- USB-UART to view log: `PA9 (TX) -> RX`, `PA10 (RX) -> TX`, General GND,
  `115200 8N1`.
- LED onboard PC13 shows firmware running; UART log indicates correct
  version 1/2/3.

## Build and flash factory v1

Run from folder `examples/OTA_Demo`:

```sh
cmake --preset STM32F103CB
cmake --build --preset STM32F103CB
cmake --build --preset STM32F103CB --target flash_jtag
```

`flash_jtag` write bootloader, factory v1 and put `otadata` to factory. After reset, bootloader wait
UART 3 seconds then LED PC13 flash and USART1 in `OTA_Demo v1`.

## flash OTA v2 valid

```sh
cmake --build --preset STM32F103CB --target flash_ota
```

Target only records slot `app_ota` and `otadata`, then resets. V2 calls `neon_ota_confirm_valid()`
after 10 iterations; subsequent resets continue to run v2.

## Test rollback by v3

```sh
cmake --build --preset STM32F103CB --target flash_ota_bad
```

V3 intentionally does not confirm. The next boot after flash is attempt 1; reset three more times to
the next attempt exceeds threshold 3. Bootloader will re-select factory and log back
`OTA_Demo v1`.

Open log on Linux, for example:

```sh
python3 -m serial.tools.miniterm /dev/ttyUSB0 115200
```

`flash_ota`/`flash_ota_bad` is an OTA emulator via SWD. UART `0x55` update protocol uses the
following two targets (default port `/dev/ttyACM0`):

```sh
# OTA v2 Relay
cmake --build --preset STM32F103CB --target flash_ota_uart

# OTA v3 does not confirm, use test rollback
cmake --build --preset STM32F103CB --target flash_ota_uart_bad
```

after client print `is for bootloader`, reset the board so that SYNC byte `0x55` falls into windows
3 seconds of bootloader. Must close Serial Monitor before running. Need to press button
`RESET/NRST`, do not unplug power please unplug USB cable. `/dev/ttyACM0` must be USB-UART/ST-Link
VCP connected to `PA9/PA10`; direct USB port of Blue Pill does not network this USART1 protocol. Can
change the port without reconfiguring:

```sh
NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 \
  cmake --build --preset STM32F103CB --target flash_ota_uart
```
