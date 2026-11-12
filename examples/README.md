# FPGA peripheral examples

Each folder below is a separate FU2 application with its own `Src/main.c`, `CMakeLists.txt`,
`CMakePresets.json`, `Kconfig`, `sdkconfig` and firmware image. Each firmware boots into a UART
terminal in its own `Src/main.c` with the prompt `fpga:>>`. Press `r` without Enter to call
that example's run function.
When it returns, the terminal waits for `r` again. Build and load a different directory to change
applications; each terminal controls only its own example.

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
| [ADC_Sampling](ADC_Sampling/README.md) | TODO skeleton | `fu2_adc_sampling` |
| [AES_Known_Vector](AES_Known_Vector/README.md) | TODO skeleton | `fu2_aes_known_vector` |
| [CRC_Known_Vector](CRC_Known_Vector/README.md) | TODO skeleton | `fu2_crc_known_vector` |
| [Flash_Read_Write](Flash_Read_Write/README.md) | Implemented | `fu2_flash_read_write` |
| [FreeRTOS_Peripheral_Queue](FreeRTOS_Peripheral_Queue/README.md) | TODO skeleton | `fu2_freertos_peripheral_queue` |
| [GPIO_Input_Output](GPIO_Input_Output/README.md) | Implemented | `fu2_gpio_input_output` |
| [GPIO_Interrupt](GPIO_Interrupt/README.md) | Implemented | `fu2_gpio_interrupt` |
| [I2C_DMA](I2C_DMA/README.md) | TODO skeleton | `fu2_i2c_dma` |
| [I2C_EEPROM](I2C_EEPROM/README.md) | Implemented | `fu2_i2c_eeprom` |
| [PWM_Output](PWM_Output/README.md) | Implemented | `fu2_pwm_output` |
| [QSPI_Flash_Read_Write](QSPI_Flash_Read_Write/README.md) | Implemented | `fu2_qspi_flash_read_write` |
| [QSPI_XIP_Read](QSPI_XIP_Read/README.md) | Implemented | `fu2_qspi_xip_read` |
| [RTC_Calendar](RTC_Calendar/README.md) | TODO skeleton | `fu2_rtc_calendar` |
| [SPI_DMA](SPI_DMA/README.md) | Implemented | `fu2_spi_dma` |
| [SPI_W5500](SPI_W5500/README.md) | Implemented | `fu2_spi_w5500` |
| [TIM_Periodic_Interrupt](TIM_Periodic_Interrupt/README.md) | Implemented | `fu2_tim_periodic_interrupt` |
| [TRNG_Read](TRNG_Read/README.md) | TODO skeleton | `fu2_trng_read` |
| [UART_DMA](UART_DMA/README.md) | Implemented | `fu2_uart_dma` |
| [UART_ECHO_Polling](UART_ECHO_Polling/README.md) | Implemented | `fu2_uart_echo_polling` |
| [UART_RX_Interrupt](UART_RX_Interrupt/README.md) | Implemented | `fu2_uart_rx_interrupt` |

TODO skeletons print their status when started with `r`. `Flash_Read_Write` accesses internal
FU2 Flash. `QSPI_Flash_Read_Write` tests an external NOR by indirect Single/Dual/Quad transfers;
`QSPI_XIP_Read` programs a known pattern and verifies it through the memory-mapped AHB window.
Both QSPI apps run from internal Flash and reserve an external NOR sector for destructive tests.
Their READMEs document the hardware connections, reserved region and menu keys.

## Build one application

Run from the repository root:

```sh
cd examples/SPI_W5500
cmake --preset FU2
cmake --build --preset FU2
```

This creates `build/FU2/fu2_spi_w5500` (ELF), `fu2_spi_w5500.bin` (raw payload) and
`fu2_spi_w5500_image.bin` (128-byte FU2 header plus payload). Other directories use the target
name listed in the table. Build directories, configurations and output images are independent.

```sh
cmake --build --preset FU2 --target menuconfig
cmake --build --preset FU2
```

Each application keeps its SDK setup, build preset and menu options in its own
`CMakeLists.txt`, `CMakePresets.json` and `Kconfig`.
The default fixed FPGA clock is 10 MHz. `CONFIG_FU2_EXAMPLE_FIXED_CLOCK_HZ` must match the actual
bitstream; choosing a software value does not change the physical FPGA clock.

## Load and run

Close the serial monitor before loading. Select the board's UART download straps and reset it,
then run inside the chosen application directory:

```sh
cmake -E env \
  NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 \
  NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 \
  NEON_MDF_UART_LOG=full \
  cmake --build --preset FU2 --target flash_uart_fu2
```

Use the packaged `_image.bin` when selecting a file manually. Restore the normal boot straps
and reset if the loader/board workflow requires it. Open UART0 at 19200 baud, 8N1, no local echo
and no flow control. Wait for `fpga:>>`, then press `r` without Enter. After `[RUN]`, type the
keys/data shown by that example.
See [FU2 build and boot](../guide/FU2.md) for the ROM boot contract and supported loading methods.

Interactive examples return on their documented stop key; one-shot examples return after completion.
The application then prints `fpga:>>` again. Press `r` to repeat without resetting the board.
While a run is active, its own keys apply: for example, `r` reads SRAM in the SPI/W5500 menu and
arms RX in the UART DMA menu. At `fpga:>>`, Enter starts a new line and displays the prompt again.
CR and LF each work as Enter; a CRLF pair produces only one new prompt. Other unused keys are ignored.

## Hardware verification

Read each application's README for pins and remaining hardware limitations. UART DMA retains
its existing word-oriented peripheral profile and timeout-recovery limitations. The TIM1 example
retains its UPDATE-flag acknowledgement behavior, which needs confirmation on the FPGA.
A successful build does not validate physical timing, wiring, interrupt acknowledgement or DMA
handshakes. The SPI/W5500 example needs a matching bitstream/pin route and an attached W5500.
