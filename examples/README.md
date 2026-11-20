# GT32U2086 examples

Each folder is a separate GT32U2086 application with its own `Src/main.c`, `CMakeLists.txt`,
`CMakePresets.json`, `Kconfig`, `sdkconfig` and firmware image. The peripheral examples listed below boot into a UART
terminal in its own `Src/main.c` with the prompt `fpga:>>`. Press `r` without Enter to call
that example's run function.
When it returns, the terminal waits for `r` again. Build and load a different directory to change
applications; each terminal controls only its own example.

## Start here

| Application | Purpose |
|---|---|
| [Printf_Log](Printf_Log/README.md) | First build and continuous UART logging |
| [Blinky](Blinky/) | Minimal GPIO toggle; verify LED routing for your board |
| [FreeRTOS_Blinky](FreeRTOS_Blinky/) | Two GPIO tasks; see [FreeRTOS guide](../guide/FreeRTOSPort.md) |
| [GPIO_Input_Output](GPIO_Input_Output/README.md) | PB5 button controls PB7; interactive UART terminal |

`Printf_Log`, `Blinky` and `FreeRTOS_Blinky` start directly without the peripheral
terminal's `r` command. Finish [environment setup](../guide/Setup.md) first.

## Directory layout

The standalone examples follow the `Printf_Log` application layout, with headers in `Inc/`
when needed:

```text
SPI_W5500/
├── CMakeLists.txt
├── CMakePresets.json
├── Kconfig
├── sdkconfig
├── README.md
├── Inc/
│   └── spi_w5500.h
└── Src/
    ├── main.c
    └── spi_w5500.c
```

`Flash_Read_Write` also keeps its scratch-sector linker script at the project root.

## Examples

| Directory | Status | Firmware target |
|---|---|---|
| [ADC_Sampling](ADC_Sampling/README.md) | Implemented; board verification required | `gt32u2086_adc_sampling` |
| [AES_Known_Vector](AES_Known_Vector/README.md) | Implemented; board verification required | `gt32u2086_aes_known_vector` |
| [CRC_Known_Vector](CRC_Known_Vector/README.md) | Implemented; board verification required | `gt32u2086_crc_known_vector` |
| [Flash_Read_Write](Flash_Read_Write/README.md) | Implemented | `gt32u2086_flash_read_write` |
| [FreeRTOS_Peripheral_Queue](FreeRTOS_Peripheral_Queue/README.md) | Implemented | `gt32u2086_freertos_peripheral_queue` |
| [GPIO_Input_Output](GPIO_Input_Output/README.md) | Implemented | `gt32u2086_gpio_input_output` |
| [GPIO_Interrupt](GPIO_Interrupt/README.md) | Implemented | `gt32u2086_gpio_interrupt` |
| [I2C_DMA](I2C_DMA/README.md) | Implemented | `gt32u2086_i2c_dma` |
| [I2C_EEPROM](I2C_EEPROM/README.md) | Implemented | `gt32u2086_i2c_eeprom` |
| [PWM_Output](PWM_Output/README.md) | Implemented | `gt32u2086_pwm_output` |
| [QSPI_Flash_Read_Write](QSPI_Flash_Read_Write/README.md) | Implemented | `gt32u2086_qspi_flash_read_write` |
| [QSPI_XIP_Read](QSPI_XIP_Read/README.md) | Implemented | `gt32u2086_qspi_xip_read` |
| [RTC_Calendar](RTC_Calendar/README.md) | Implemented; board verification required | `gt32u2086_rtc_calendar` |
| [SPI_DMA](SPI_DMA/README.md) | Implemented | `gt32u2086_spi_dma` |
| [SPI_W5500](SPI_W5500/README.md) | Implemented | `gt32u2086_spi_w5500` |
| [TIM_Periodic_Interrupt](TIM_Periodic_Interrupt/README.md) | Implemented | `gt32u2086_tim_periodic_interrupt` |
| [TRNG_Read](TRNG_Read/README.md) | Implemented; board verification required | `gt32u2086_trng_read` |
| [UART_DMA](UART_DMA/README.md) | Implemented | `gt32u2086_uart_dma` |
| [UART_ECHO_Polling](UART_ECHO_Polling/README.md) | Implemented | `gt32u2086_uart_echo_polling` |
| [UART_RX_Interrupt](UART_RX_Interrupt/README.md) | Implemented | `gt32u2086_uart_rx_interrupt` |

Implemented; board verification requireds print their status when started with `r`. `Flash_Read_Write` accesses internal
GT32U2086 Flash. `QSPI_Flash_Read_Write` tests an external NOR by indirect Single/Dual/Quad transfers;
`QSPI_XIP_Read` programs a known pattern and verifies it through the memory-mapped AHB window.
Both QSPI apps run from internal Flash and reserve an external NOR sector for destructive tests.
Their READMEs document the hardware connections, reserved region and menu keys.

## Build one application

Run from the repository root:

```sh
cd examples/SPI_W5500
cmake --preset GT32U2086
cmake --build --preset GT32U2086
```

This creates `build/GT32U2086/gt32u2086_spi_w5500` (ELF), `gt32u2086_spi_w5500.bin` (raw payload) and
`gt32u2086_spi_w5500_image.bin` (128-byte GT32U2086 header plus payload). Other directories use the target
name listed in the table. Build directories, configurations and output images are independent.

```sh
cmake --build --preset GT32U2086 --target menuconfig
cmake --build --preset GT32U2086
```

Each application keeps its SDK setup, build preset and menu options in its own
`CMakeLists.txt`, `CMakePresets.json` and `Kconfig`.
The default fixed FPGA clock is 10 MHz. `CONFIG_GT32U2086_EXAMPLE_FIXED_CLOCK_HZ` must match the actual
bitstream; choosing a software value does not change the physical FPGA clock.

## Load and run

Close the serial monitor before loading. Run inside the chosen application directory,
select boot mode `100`, wait for the loader to report UART ready, then press RESET:

```sh
cmake -E env \
  NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 \
  NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 \
  NEON_MDF_UART_LOG=full \
  cmake --build --preset GT32U2086 --target flash_uart_gt32u2086
```

Use the packaged `_image.bin` when selecting a file manually. After a successful transfer,
ROMCODE hands off to the application. Select boot mode `000` for future resets. Open UART0 at 19200 baud, 8N1, no local echo
and no flow control. Wait for `fpga:>>`, then press `r` without Enter. After `[RUN]`, type the
keys/data shown by that example.
See [GT32U2086 build and boot](../guide/GT32U2086.md) for the ROM boot contract and supported loading methods.

Interactive examples return on their documented stop key; one-shot examples return after completion.
The application then prints `fpga:>>` again. Press `r` to repeat without resetting the board.
While a run is active, its own keys apply: for example, `r` reads SRAM in the SPI/W5500 menu and
arms RX in the UART DMA menu. At `fpga:>>`, Enter starts a new line and displays the prompt again.
CR and LF each work as Enter; a CRLF pair produces only one new prompt. Other unused keys are ignored.

## Hardware verification

Read each application's README for pins and remaining hardware limitations. UART DMA retains
its existing word-oriented peripheral profile and timeout-recovery limitations. The TIM1 example
uses live UPDATE status and disables its IRQ if the bounded wait expires; measure timing on the FPGA.
A successful build does not validate physical timing, wiring, interrupt acknowledgement or DMA
handshakes. The SPI/W5500 example needs a matching bitstream/pin route and an attached W5500.

See [hardware support and acceptance checks](../guide/HardwareValidation.md) for capability limits and
required board evidence. The source and automated build checks do not establish hardware validation.
