# FPGA_Terminal

The command-line terminal runs on FU2 FPGA via UART0. A firmware at the same level prompts `fpga> `
to call testcases. There is now integration of UART echo polling from example `UART_ECHO_Polling`,
button/LED from `GPIO_Input_Output` with output changed to PB7, interrupt `GPIO_Interrupt` on PB5,
flash LED with `TIM_Periodic_Interrupt` on PB7 and erase/write/read Flash with `Flash_Read_Write`.
UART RX interrupt, UART DMA and PWM also have code. `SPI_W5500` shows two stubs `SPI_Loopback` and
`SPI_Master_Slave` in the original plan: there are currently 21 testcase, 9 coded post and 12 post
waiting for deployment.

Default: UART0, 19200 baud, 8N1, FIFO off, 10 MHz FPGA clock. Terminal receives commands by polling;
testcase `gpio_irq` enables GPIO interruption, `tim_irq` enables timer interruption. Testcase
`uart_rx_irq` temporarily enables FIFO and RX interruption on UART0, then restores console
configuration when exiting. `uart_dma` temporarily uses UART0 FIFO and DMA0, then restores the
terminal UART configuration. FreeRTOS is not used.

## Structures

```text
FPGA_Terminal/
├── CMakeLists.txt
├── CMakePresets.json
├── Kconfig
├── sdkconfig
├── README.md
├── Inc/
│   └── main.h                # Include for main.c
├── Src/
│   └── main.c                # UART, line receiving, command panel and dispatch
└── testcase/
    ├── CMakeLists.txt        # Register the directory of each testcase
    ├── UART_ECHO_Polling/
    ├── GPIO_Input_Output/
    ├── GPIO_Interrupt/
    ├── TIM_Periodic_Interrupt/
    ├── PWM_Output/
    ├── I2C_Register_Read_Write/
    ├── SPI_W5500/
    ├── Flash_Read_Write/
    ├── UART_RX_Interrupt/
    ├── UART_DMA/
    ├── ADC_Sampling/
    ├── DMA_Memory_Copy/
    ├── RTC_Calendar/
    ├── CRC_Known_Vector/
    ├── AES_Known_Vector/
    ├── TRNG_Read/
    ├── QSPI_Flash_Indirect/
    ├── I2C_Slave/
    ├── SPI_DMA/
    ├── I2C_DMA/
    └── FreeRTOS_Peripheral_Queue/
```

`Inc/`, `Src/` and `testcase/` are of the same level. `main.c` include `Inc/main.h`, which combines
the include of the terminal. When adding a testcase, each post has its own directory under
`testcase/`, containing the header and source. Add the post's include to `Inc/main.h` and handler to
the table `commands` in `main.c`; `help` reads the same table to list commands. Source `.c` is added
via `target_sources(fpga_terminal PRIVATE ...)` in the CMake of the testcase, not including directly
`.c` into `main.c`.

The handler is in the form of `void handler(void)`. Terminal takes the first word as its command
name and ignores any additional input after it. Handler returns to prompt terminal again. Wrapper of
commands `uart_echo`, `gpio_io`, `gpio_irq`, `tim_irq`, `uart_dma` and `spi_w5500` pass the current UART0 handle
to testcase.

The command `flash_rw` runs one turn and then returns, no need to receive UART.

Each new frame directory has `CMakeLists.txt`, the file `.h` declares the function `Run(void)` and
the file `.c` contains the stub of that function. The script calls the private wrapper
`static_fpga_terminal_command_*` in `main.c`, the wrapper calls the post's `Run()`. Stub in `[TODO]
<testcase> is not implemented yet.` then returns to prompt. Peripheral settings, data transfer and
verification will be filled in later. DMA is enabled for UART_DMA; the DMA memory-copy, SPI DMA,
I2C DMA and FreeRTOS testcase stubs remain unimplemented.

## UART DMA (`uart_dma`)

Enter `uart_dma`, then press single keys without Enter:

| Key | Test |
|---|---|
| `t` | DMA sends `0123456789ABCDEF` four times: exactly 64 UART characters |
| `r` | After `[READY]`, paste exactly `0123456789ABCDEF`, with no Enter; DMA receives 16 characters and compares them |
| `q` / Ctrl+C | Return to the terminal when no transfer is running |

The testcase uses the existing UART0 baud and pins, FIFO mode, DMA0 channel 0 and UART0 request
slot 2. It requires a PDI with the native FU2 DMA and UART DMA handshake interface. DMA0 is
exclusive to this testcase; entry refuses an already active DMA channel. TX and RX run separately.
The main loop counts iterations while DMA moves data and polls completion; no DMA ISR is needed.
TX completion waits for both DMA/UART completion and the last serial bit before printing status.
Compare the TX text on the PC; RX reports `[PASS]` only for the complete expected pattern.

Buffers use `uint32_t` elements, one character per low byte. A 64-character TX uses 256 RAM bytes
and a 16-character RX uses 64 RAM bytes. DMA memory addresses increment by four; the UART FIFO
address stays fixed, with one 32-bit access per single request. UART DMA counts characters,
while the DMA controller counts memory bytes.

CPU RX is suspended during RX DMA, including `q`/Ctrl+C, which are payload at that point.
`CONFIG_UART_DMA_TIMEOUT_POLLS` bounds the wait (default 10000000 iterations, not milliseconds).
Timeout or DMA errors discard the partial result and reset the exclusively owned DMA0 before the
buffer can be reused. The DMA no-progress watchdog is advisory for peripheral-paced commands;
bus/FIFO/AXI timeout errors still fail the test. UART receive errors also prevent RX PASS.
On exit the UART configuration is restored. Configuration lives
in the driver; the testcase contains only pattern setup, transfer setup and the menu loop.

## UART RX Interrupt (`uart_rx_irq`)

The minimal example has only two functions: `USART0_IRQHandler()` receives data and
`uart_rx_interrupt_run()` sets up UART, runs main and returns terminal. Data stream: PC → UART RX
FIFO → ISR → ring buffer → main → echo to PC.

Enter `uart_rx_irq`, wait for `[READY]` and send the string. ISR reads existing bytes in FIFO into
array `g_rx_buffer`; main takes each byte out, prints log for `[PC -> FPGA]` and echoes whole bytes
with label `[FPGA -> PC]`. Example sending `A`:

```text
[PC -> FPGA] 'A' (0x41)
[FPGA -> PC]  A  (0x41)
```

Log is in the main, after byte has been removed from FIFO and ring; does not indicate data remaining
in FIFO. Unprinted characters are displayed with `.` at the receiving line; hex value indicates
actual byte. The sending line still transmits original echo bytes, so CR/LF can do downstream. Log
each byte increases the amount of TX data, should send each character or short burst for easy
observation. Turn off local echo on PC.

Array uses element `uint32_t` according to FU2 memory requirements. `g_rx_head` is the next write
position, `g_rx_tail` is the next read position, `g_rx_count` is the number of bytes pending. Main
key break in short segment update ring.

The ring size is set in `CONFIG_UART_RX_INTERRUPT_RING_BUF_SIZE`, default 128 slots. When the ring
is full, remove new bytes and continue to retrieve data from FIFO. There are no statistical structs,
subfunctions, error logs or initialization conditions checks in the example. For example, use
terminal-configured 8-bit UART0; tests inside the UART driver are still performed by the driver.

The `g_main_work` variable increases with each main pass, which can be observed in the debugger to
show that the main is still running when no data has been sent. Ctrl+C (`0x03`) sets a stop flag
right in the ISR, even when the ring is full. The main echoes all previously queued bytes, disables
RX interrupts and restores UART configuration to render `fpga> ` again. Residual LF of CRLF command
is ignored. Residual data received after Ctrl+C is ignored; wait for prompt before sending next
command.

TX uses polling. `neon_uart_try_receive()` allows the ISR to receive RX while the main is
transmitting on the same handle. Before changing configuration, for example call
`neon_uart_wait_tx_complete()` to wait for the last byte to run out of wires. The FIFO setup has a
UART reset, so only send payload after `[READY]`.

Test: send `ABC123` and observe the echo in the correct order, press Ctrl+C then try `help`; re-run
`uart_rx_irq` to test multiple entry/exit. Confirm IRQ/FIFO operation on actual FPGA.

## SPI W5500 (`spi_w5500`)

FU2 is SPI0 master, W5500 is slave. Testcase tests the SPI communication by W5500's register and
SRAM; no IP, socket or Ethernet transmission configured yet. No LAN cable plug is needed to run
these tests.

Wire according to the table below, connect GND together, power up the module as required and ensure
RSTn has been pulled up / reset circuit is active. Signal W5500 uses 3,3 V level; INTn is not used.

| SPI0 | PMOD pin | Package FU2 | Die pad/SYSCSR group |
|---|---|---|---|
| SCK | IO5 | 60 | PIN_70 / right[21] |
| MOSI | IO4 | 62 | PIN_74 / top[1] |
| MISO | IO1 | 61 | PIN_73 / top[0] |
| SS0 → W5500 SCSn | IO0 | 63 | PIN_75 / top[2] |

The SPI driver manually selects mux and configuration pad when `pin_config=AUTO` (default). DeInit
restores saved pad/mux bits; the testcase does not write SYSCSR directly. `KEEP` is for applications
that have configured pins themselves. Package and die pad numbers vary. UART0 remains for command
entry and log printing.

Enter `spi_w5500` and then Enter. Default mode 3, MSB first, SCK 1 MHz, hardware SS0; at system
clock 10 MHz, divider by 4. Testcase wait for 560 ms for circuit reset module, read `VERSIONR` (must
be equal to `0x04`), write MR=`0x80` to soft reset and wait for MR to return to `0x00`. Reset passes
only after VERSIONR is also read back as `0x04`. RCR test writes require a correct ID.
After `[READY]`, press each key, no Enter needed:

| Keys | Operations |
|---|---|
| `i` | Running again waiting for boot and soft reset W5500 |
| `s` | Stop auto, reinitialize SPI0 and attempt W5500 software reset without a VERSIONR precheck |
| `t` | Read VERSIONR, MR, PHYCFGR, RTR, RCR and LINK status |
| `p` | Save RCR; write/read `0x55`, `0xAA`; rewrite and check the original value |
| `r` | Continuous reading 32 bytes socket 0 TX SRAM at offset `0x0000` |
| `b` | Turn off auto; sweep B2B 10..0 in modes 0/3 at exact SCK 1/3/5/10 MHz, with CRC32/ID/SPI error statistics |
| `d` | Read only VERSIONR/RCR/PHYCFGR in mode 0/3, 1 MHz/100 kHz, then return the selected mode/speed |
| `0`–`4` | Mode 0: 100 kHz, 500 kHz, 1 MHz, 2,5 MHz, 5 MHz |
| `5`–`9` | Mode 3: same five speed levels above |
| `a` | Turn on/off ping/pong automatically every 3 seconds |
| `h` / `?` | Print instruction |
| Ctrl+C / `q` | Turn off SPI0, restore pad and SysTick, return to `fpga> ` |

Auto default OFF. Select a preset `0`–`9` that will run ping/pong immediately and enable auto.
`CONFIG_SPI_W5500_SCK_HZ` set initial speed; divider rounds up so the real SCK does not exceed the
required value. `CONFIG_SPI_W5500_TIMEOUT_POLLS` is the number of wait rings per transaction, not
milliseconds.

The `s` command keeps the selected SPI mode, speed and timing, resets the SPI0 controller to release
CS and clear its FIFOs, then writes W5500 MR=`0x80`. It polls for MR=`0x00` and checks VERSIONR=`0x04`
before reporting `[PASS]`. Unlike startup/`i`, it attempts the reset even if the previous ID read
failed. This is a software reset over SPI, so it still needs a working SPI connection. This example
does not configure a GPIO for RSTn; hardware reset requires the module's reset circuit or a power cycle.

Driver FU2 defaults to 10 SCK cycles between bytes, including bytes in 32-bit word. Adjust
`NEON_SPI_FU2_B2B_SCK_CYCLES` in `spi/neon_spi.h` and rebuild to change the default. Driver
calculates `T_B2B = between_frames * 2 * (clock_divider + 1)` when the unit is
`NEON_SPI_B2B_SCK_CYCLES`, even if changing the divider at runtime. At core 10 MHz/SCK 1 MHz, the
default value of `T_B2B` is 100, corresponding to 10 μs. Register value must be 16 bits;
configuration exceeding the limit is rejected.

Log `[FU2 -> W5500]` shows command/TX; `[W5500 -> FU2]` shows raw RX and low data bytes. Example
VERSIONR reading sends `0x00390000`: two bytes address, one byte control and one byte dummy. Three
RX bytes of the header are ignored. RX when written is not ACK; write results are confirmed with a
subsequent read. Ping/pong indicates `[PASS]` if both patterns are correctly read and RCR has been
restored correctly.

Burst sends the `00 00 10` header (BSB=2, read, VDM), then 32 bytes dummy. Hold SS0 LOW with manual
hold: eight 32-bit words containing the header and 29 bytes of data, then a 24-bit word receives the
last three bytes and release CS. hardware creates B2B between bytes in the word. Then returns to
normal 32-bit words and SS. The SRAM content after reset is not fixed, so this dump is not used to
conclude PASS/FAIL or as a memory test sample.

The key `b` tests mode 0 followed by mode 3, each at requested SCK 1, 3, 5 and 10 MHz.
Each supported case sweeps B2B from 10 down to 0 SCK periods. Only exact frequencies are tested;
the log reports `SKIP` with the requested and achievable frequencies for other cases. The divider
uses `SCK = core_clock / (2 * (divider + 1))`. With the default 10 MHz FPGA clock, 1 and 5 MHz are
tested in both modes; 3 MHz (rounded to 2.5 MHz) and 10 MHz (limited to 5 MHz) are skipped. Testing
all four frequencies exactly requires a suitable actual SPI source clock, such as 60 MHz. The
configured clock must match the hardware.

The sweep only reads VERSIONR and the same 32 bytes of socket 0 TX SRAM. No sockets run during
the test, so the SRAM contents remain unchanged. The first read in mode 0 / 1 MHz / B2B=10 supplies
the CRC32 reference, which is validated over all attempts at this gap. If this baseline fails,
the entire sweep stops. All later gaps and mode/speed cases use the same reference. If another
case fails at gap 10, its lower gaps are skipped and the next case runs. Each attempt counts as
failed only once; CRC/ID/IO counters indicate the causes and may overlap.

`CONFIG_SPI_W5500_B2B_TRIES` sets the attempts per gap per mode/speed case (default 1000).
Logs include mode, actual SCK, gap, failed attempts, percentage and the smallest gap with zero
errors in each completed case. Individual transactions are not printed during the sweep.
Press `q`/Ctrl+C to cancel and return to the menu. The test attempts to restore the full original
SPI configuration and selected mode/speed on every exit; auto remains OFF. Results describe this
sample count and do not change the default B2B. CRC32 checks read repeatability, while VERSIONR
provides a known expected value; neither establishes that the SRAM holds a known test pattern.

SPI uses polling, not DMA/IRQ. SysTick runs cycle counting, does not enable interrupt; if SysTick is
on, the testcase says busy and returns. Timeout SPI resets controller before next transaction; burst
error does not print dump unfinished.

Check on board: read ID `04`, run `p` see PASS and RCR remain the same, try presets, `r`, `d`, turn
on/off auto then Ctrl+C and re-run `help`. Use FPGA/logic analyzer to confirm pin, timing, CS and
actual MISO/MOSI line.

## Existing commands

| Orders | Testcase/Function | Priorities | The status |
|---|---|---|---|
| `help` | List commands | — | Availability |
| `info` | Terminal Information | — | Availability |
| `clear` | Delete Screen | — | Availability |
| `uart_echo` | UART_ECHO_Polling | P1 | With code |
| `gpio_io` | GPIO_Input_Output | P1 | With code |
| `gpio_irq` | GPIO_Interrupt | P1 | With code |
| `tim_irq` | TIM_Periodic_Interrupt | P1 | With code |
| `pwm_output` | PWM_Output | P1 | With code |
| `i2c_rw` | I2C_Register_Read_Write | P1 | Khung `[TODO]` |
| `spi_w5500` | SPI_W5500 | P1 | With code |
| `flash_rw` | Flash_Read_Write | P1 | With code |
| `uart_rx_irq` | UART_RX_Interrupt | P2 | With code |
| `uart_dma` | UART_DMA | P2 | With code |
| `adc_sampling` | ADC_Sampling | P2 | Khung `[TODO]` |
| `dma_memcpy` | DMA_Memory_Copy | P2 | Khung `[TODO]` |
| `rtc_calendar` | RTC_Calendar | P2 | Khung `[TODO]` |
| `crc_vector` | CRC_Known_Vector | P2 | Khung `[TODO]` |
| `aes_vector` | AES_Known_Vector | P2 | Khung `[TODO]` |
| `trng_read` | TRNG_Read | P2 | Khung `[TODO]` |
| `qspi_flash` | QSPI_Flash_Indirect | P2 | Khung `[TODO]` |
| `i2c_slave` | I2C_Slave | P3 | Khung `[TODO]` |
| `spi_dma` | SPI_DMA | P3 | Khung `[TODO]` |
| `i2c_dma` | I2C_DMA | P3 | Khung `[TODO]` |
| `freertos_queue` | FreeRTOS_Peripheral_Queue | P3 | Khung `[TODO]` |

“Having code” only describes the deployment status, does not confirm that the paper has passed
hardware tests. Items `[TODO]` have not performed peripheral operations.

The above commands are case sensitive and ignore the extra text after the command name. Enter the
command and press Enter; CR, LF and CRLF are supported. For example:

```text
FU2 FPGA terminal
Type 'help' to list commands.
fpga> help
Available commands:
  help               Show available commands
  info               Show terminal configuration
  clear              Clear the terminal screen (ANSI)
  uart_echo          UART0 polling echo; Ctrl+C returns to terminal
  gpio_io            PB5 button -> PB7 output; Ctrl+C returns to terminal
  gpio_irq           PB5 rising-edge IRQ counter; 'i' injects; Ctrl+C returns
  tim_irq            TIM1 500 ms IRQ -> PB7 LED blink 1 Hz; Ctrl+C returns
  flash_rw           Erase/program/verify reserved Flash sector at 0x0009FE00
  uart_rx_irq        UART0 rx interrupt; Ctrl+C returns to terminal
  uart_dma           UART0 DMA: t=TX64, r=RX16, q=return
  pwm_output         TIM0_CH0 PWM output; Ctrl+C returns to terminal
  i2c_rw             [TODO] I2C register read/write
  spi_w5500          SPI0 W5500 register read/write + burst; Ctrl+C returns
  adc_sampling       [TODO] ADC sampling
  dma_memcpy         [TODO] DMA memory copy
  rtc_calendar       [TODO] RTC calendar
  crc_vector         [TODO] CRC known vector
  aes_vector         [TODO] AES known vector
  trng_read          [TODO] TRNG random data
  qspi_flash         [TODO] QSPI Flash indirect access
  i2c_slave          [TODO] I2C slave
  spi_dma            [TODO] SPI DMA
  i2c_dma            [TODO] I2C DMA
  freertos_queue     [TODO] FreeRTOS peripheral queue
fpga> info
Target: FU2 FPGA
Core clock: 10000000 Hz
Terminal: UART0, 19200 baud, 8N1, polling
Command limit: 127 characters
fpga>
```

## Testcase UART_ECHO_Polling

Enter `uart_echo`, then Enter. The testcase uses the console's default UART0 handle, baud, clock and
FIFO; do not reset or re-create the UART when entering/exiting. Each received byte will print `[USER
-> FPGA]` information, send back the same byte with `neon_uart_transmit`, and then print `[FPGA ->
USER]` results, as in the original example:

```text
fpga> uart_echo
[INFO] UART echo ready. Type a character; Ctrl+C returns to terminal.
[USER -> FPGA] 'A' (0x41)
[FPGA -> USER] A (0x41)
```

Press Ctrl+C to stop the testcase and go back to `fpga> `. The `[STATS]` line prints the byte number
RX/TX, the timeout number RX/TX, the error number RX/TX, the latest error status and code. The count
numbers do not include diagnostic data, Ctrl+C and LF belong to Enter CRLF when calling commands.
The variable `g_uart_echo_stats` can be viewed in the debugger; statistics are deleted when running
the next testcase.

when the testcase is running, characters such as `help` are echo data. Exit with Ctrl+C before
entering another terminal command. RX timeout pending typing is normal and counted; error RX/TX is
written to receive and continues to receive, not manually re-send byte errors. Only successful echo
does not confirm the testcase achieved by itself; need to compare received/sent bytes and counter
errors on hardware.

Turn off local echo on your terminal because FPGA has already echoed the character. In prompt `fpga>
`, Backspace and DEL delete the last character; Tab to a blank; Ctrl+C cancels the input line. Arrow
key/function key is ignored without history or autocomplete. No need to re-enter firmware to run
another terminal command.

The default 128-byte buffer contains up to 127 characters and end characters. Lines that are too
long or have RX errors will be dropped entirely to Enter; Ctrl+C also allows re-entry. RX timeout
when no data is available is the normal idle state. This is an interactive terminal: type commands
or send each line, wait for prompt and then send again; there is no flow control to ensure
continuous long burst/paste.

## Testcase GPIO_Input_Output

Enter `gpio_io` and then Enter. Testcase keeps the original example's input GPIOB5 (PB5) and uses
GPIOB7 (PB7) as its active-high output:

| Feet | configuration and operation |
|---|---|
| PB5 | Input pull-up, synchronize input; GND downlink button, press=0, release=1 |
| PB7 | Low start-up output; press button=1, release button=0 |

Connect PB7 to LED via resistor or pin map of PDI. If PDI does not support pull-up PAD, use external
pull-up resistor for PB5. GPIO UART0 is kept for terminal.

```text
fpga> gpio_io
[INFO] PB5 button -> PB7 LED. Ctrl+C returns to terminal.
[GPIO] PB5=1 RELEASED -> PB7=0 (LED OFF)
[GPIO] PB5=0 PRESSED -> PB7=1 (LED ON)
[GPIO] PB5=1 RELEASED -> PB7=0 (LED OFF)
```

Log output when entering the testcase and when PB5 level is changed. loop read GPIO directly and
check UART do not wait to receive Ctrl+C; other characters are ignored. When exiting, PB7 is written
low then PB5/PB7 is DeInit, turning off output before returning to prompt. It is possible to re-run
`gpio_io` or `uart_echo` immediately afterwards. The testcase does not flush the button, so a single
press/release can create multiple logs.

## Testcase GPIO_Interrupt

Enter `gpio_irq` and then Enter. Testcase configuration PB5 input pull-up, enable input
synchronization and interrupt edge up. With the PB5 button connecting down to GND, press create edge
down;
**release the edge-up button and increase counter**. Can use an external signal source with a suitable
voltage bank instead of the button.

GPIOB connects to IRQ20 according to FU2 Common Specification v8 and wiring `gpioB.intr =>
armcortexM0.intisr[20]` in the `FVP_FU2.lisa` of the FU2 source. The current Header/startup of the
SDK names this path `EXTI1_IRQn` and `EXTI1_IRQHandler`. Testcase uses CMSIS
`NVIC_EnableIRQ`/`NVIC_DisableIRQ` directly. BootROM v5 forwards the interrupt to the application
vector as described in
[FU2 build/boot contract](../../guide/FU2.md).

ISR reads `neon_gpio_get_interrupt_status()`, handles PB5 separately, clears raw pending and
software injection via `neon_gpio_clear_interrupt()`, and then increases the variable
`g_gpio_interrupt_count`. The testcase loop reads counter and prints log; ISR does not call
`printf`. Counter is a 32-bit word, which is deleted each time the testcase is run.

```text
fpga> gpio_irq
[INFO] PB5 rising-edge interrupt on IRQ20 (EXTI1).
[INFO] Release the button to count; 'i' injects an IRQ; Ctrl+C returns.
[GPIO IRQ] PB5 events=1 (+1)
[GPIO IRQ] PB5 events=2 (+1)
[INFO] GPIO interrupt stopped. Total=2
fpga>
```

Press `i` to set the software injection of PB5; ISR must be run once and then delete the interrupt
source. This is GPIO path check interrupt/NVIC/ISR, do not confirm external PAD pins. Other
characters are ignored, Ctrl+C stops testcase. When exiting, code disables NVIC and mask PB5,
removes flag GPIO/NVIC and DeInit PB5. PRIMASK is restored to value before entering testcase. PB7 is
not used in this testcase. It is possible to re-run `gpio_irq` or switch to `gpio_io`/`uart_echo`
without flashing firmware again.

With a clean edge source, each time the ISR serves the PB5, it increases the counter by one unit. If
there are multiple IRQs during printing, the log line indicates a new total and an additional
`(+N)`. The testcase is not anti-backlash software yet; the mechanical node can create multiple
edges up. Edges that arrive too quickly when the flag is pending can be pooled, so this is not a
high speed pulse counter. Collate the number of edges and counters on the FPGA to confirm hardware.

## Testcase TIM_Periodic_Interrupt

Enter `tim_irq` and Enter. Testcase uses TIM1 CH0, IRQ13 (`TIM1_IRQHandler`) and GPIOB7 (PB7) as
outputs. Connect PB7 to LED via resistor according to PDI's map pin. PB7 starts low, then reverses
each level
500 ms right in ISR: brighten 0,5 sec, turn off 0,5 sec, flashing 1 Hz for visual observation. If
the LED on board is active-low then the light/off level reverses.

Timer counts down, enables auto-reload and interrupt update. TIM1 clock is obtained with
`SystemCoreClock`, must match the actual FPGA clock. default configuration 10 MHz:

```text
TIM_PERIODIC_TICK_HZ = 2
prescaler = SystemCoreClock / 1000 - 1 = 9999
auto_reload (CHL) = 1000 / TIM_PERIODIC_TICK_HZ - 1 = 499
T_update = (prescaler + 1) * (CHL + 1) / timer_clock = 0,5 s
```

Counter uses 1 ms tick, same configuration prescaler as the periodic interrupt of `timer_irq_wdt`.
On tried FPGA, STT read only. According to IP side, reading STT does not clear flags or clear IRQ
and UPDATE flags do not lower themselves at the next tick. The acknowledge mechanism on the trying
FPGA version has not been confirmed.

Testcase only enables CH0's UPDATE interrupt. ISR reads STT_CH0; when a new UPDATE flag is invoked,
it reverses PB7, increases `g_tim_periodic_ticks`, and then uses the `while` loop to read UPDATE
message to 0 before exiting to avoid repeat counting. The waiting loop only reads message, does not
acknowledge, and has no timeout. If the flag holds level 1, the waiting loop will not end; the IP
should confirm acknowledge operation. Log is printed in the main loop; ISR does not call `printf`.
The main loop checks UART for Ctrl+C and prints the total number of ticks per two ticks (about one
second). counter is the number of interrupts processed, does not measure time accuracy by itself.

```text
fpga> tim_irq
[INFO] TIM1 CH0 update interrupt on IRQ13, clock=10000000 Hz.
[INFO] PSC=9999, CHL=499; PB7 toggles every 500 ms (LED 1 Hz).
[INFO] Ctrl+C stops the timer and returns to terminal.
[TIM IRQ] ticks=2 (+2)
[TIM IRQ] ticks=4 (+2)
```

Press Ctrl+C to stop counter, disable interrupt timer/NVIC and erase pending. Testcase write low PB7
then DeInit pins before prompt return; tick number is erased when rerunning. PRIMASK is
saved/restored like `gpio_irq`. This post is dedicated to
**all TIM1** because at the time of entry there is pulse reset to clear the status of both CH0/CH1.
CH1 is DeInit so only CH0 can play interrupt. CHL is recorded after Start because the reference FPGA
flashes CNT from CHL only when the channel is running.

when measuring with scope/logic analyzer, skip the leading/ trailing edge at start/stop. Expected
two consecutive edge distances 500 ms; a full period is expected 1 s. The target of period checking
is ±1% so with set value when clock is correct; jitter needs to write receiving from actual
measurement. Check after Ctrl+C no longer reverses level and can run again `tim_irq`, `gpio_io`,
`gpio_irq`. Flash LED and log tick only confirm active interrupt stream; measuring device is needed
to evaluate period/jitter.

Keep `CONFIG_NEON_MDF_DRIVER_TIM=y` in `sdkconfig`. Want to change the blink rate, fix
`TIM_PERIODIC_TICK_HZ` in source; each interrupt reverses the LED once so the blink frequency is
half of the interrupt frequency.

Spec rev1.0 writes STT as RW but does not describe how to delete it; the FPGA has tried to report
bus errors when writing STT. The backend currently only reads STT through `GetActiveFlags()` and
`neon_tim_clear_flag()` returns `NEON_NOT_SUPPORTED` because there is no confirmed clearing for this
version of FPGA. It is necessary for the IP to provide the address and acknowledge operation
correctly, including the side effect of reading STT.

## Testcase Flash_Read_Write

Enter `flash_rw` and then Enter to run a turn and automatically return prompt. Delete command
**sector 512 bytes at 0x0009FE00–0x0009FFFF**, last sector of main Flash 512 KB according to current
FU2 map. All old data in this sector will be deleted. Testcase does not call chip erase and does not
access NVR.

`flash_read_write.ld` reserved this area with section `NOLOAD`, providing code address through
`__flash_rw_start__` and blocking links if the image reaches the test sector. The scratch area does
not contain initialization data in the image. Scripts are paired with SDK's linker scripts and
require FU2 normal profiles to run from internal Flash; do not modify SDK's automatically generated
linker scripts.

Test stream using Flash driver public APIs:

1. GetDefaultConfig and config driver.
2. EraseSector and ReadWord checked enough 128 words were about `0xFFFFFFFF`.
3. ProgramWord has enough 128 words by pattern 0/1, alternating bits and data with changes in address
   group to detect mistakenly written address.
4. ReadWord collates each word, indicating PASS or address/error data first.
5. DeInit driver before returning terminal; pattern is kept in sector.

In both `normal` and `xip` profiles, `flash_rw` tests on the Internal Flash sector `0x0009FE00`, not
writing to the QSPI Flash containing XIP image. Linker checks that the normal image area does not
overlap this sector.

```text
fpga> flash_rw
[INFO] Flash scratch: 0x0009FE00, 512 bytes; contents will be erased.
[INFO] Erasing scratch sector.
[PASS] Erase read-back: 128 words are 0xFFFFFFFF.
[INFO] Programming 128 pattern words.
[PASS] Flash_Read_Write: 128/128 words match.
fpga>
```

The address read/write is separated by 4 bytes and the data is `uint32_t`. The driver sets the
function erase/program and the command waiting ring in `.ramfunc`, which is startup copied to RAM
via `.data`; saves/restore PRIMASK when Flash operations are performed. The command timeout is
calculated by the number of poll rings, not milliseconds. Testcase follows the map and FU2 v8
command engine that the driver is using; it needs to run on an FPGA to confirm actual hardware
behavior.

According to Flash Controller Specification v1.4 entry 9.1, config wait BUSY and START together on 0
before reusing BootROM's INIT_DONE or play INIT. when playing command, driver write opcode with
START=0 then set START=1. Each waiting ring has a limit operation_timeout_polls; expiry pays
NEON_TIMEOUT see because of infinite waiting. neon_flash_init pays neon_status_t and does not set in
`.ramfunc`; the helper that plays command/wait for completion still runs from SRAM and keeps
interrupt locked during command operation.

## Note when using interrupt on FU2 FPGA

**BootROM v5 hands over the application with `PRIMASK=1`, i.e. global interrupt is locked.**
`NVIC_EnableIRQ()` only opens an IRQ path in NVIC, not PRIMASK. So the periphery may have emitted a
request and NVIC is pending but the CPU has not entered the ISR yet.

After the peripheral configuration, preparation of shared data with ISR and deletion of old pending
flags, it is necessary to open both NVIC and global interrupt. For example in `gpio_irq`:

```c
uint32_t saved_primask = __get_PRIMASK();
/* configuration GPIO, prepare counter and delete pending GPIO/NVIC. */
NVIC_EnableIRQ(EXTI1_IRQn);
__enable_irq();
```

when exiting the testcase, disable NVIC and peripheral interrupt sources, erase pending and call
`__set_PRIMASK(saved_primask)` to restore the initial state. The critical section helpers in PRIMASK
save/restore drivers; they do not unlock global interrupts for applications themselves.

If pressing `i` only shows `[INFO] Injecting a GPIO interrupt event.` and the counter does not
increase, check `__get_PRIMASK()`: the value `1` means that the CPU still locks the interrupt. Keep
the ISR short: read/erase flag and increase the counter; print `printf` in the main loop because the
console may be busy when the interrupt occurs.

## Build and configuration

```sh
cd /home/quang-hoang/github_neon_mdf/CDC_NEON_MDF/examples/FPGA_Terminal
cmake --preset FU2
cmake --build --preset FU2
```

Change clock, line size and RX/TX timeout to receive command/testcase:

```sh
cmake --build --preset FU2 --target menuconfig
cmake --preset FU2
cmake --build --preset FU2
```

Keep the UART driver on. Clock must match PDI in use; timeout by number of rounds poll, not
milliseconds. The first `printf` command initializes UART0 itself by SDK default, like for example
`Printf_Log`. Terminal takes handle via `neon_console_get_uart()` and uses it directly to receive
commands and testcases. `main()` does not have its own configuration/config UART and does not need
to call `neon_console_attach_uart`. Console uses baud 19200 and default timeout for each byte
`printf`. RX timeout applies to prompt characters. Testcase GPIO checks RX without waiting to
continue reading node. The `UART_ECHO_Polling testcase` entry in menuconfig has its own RX/TX
timeout for
testcase (`CONFIG_UART_ECHO_RX_TIMEOUT_POLLS`, `CONFIG_UART_ECHO_TX_TIMEOUT_POLLS`).

## flash and open terminal

suitable PDI flash first. UART0 must be wrapper connected to an external FTDI or USB-UART according
to the PDI's pin map; this firmware does not change the FPGA header pin.

With profile `normal`, the file to flash BootROM v5 is
`build/FU2/fpga_terminal_image.bin`, consists of 128 byte header and payload. Profile `xip` creates
`fpga_terminal_image_xip.bin` for boot from QSPI Flash. Two files `fpga_terminal.bin` and
`fpga_terminal_xip.bin` contain only raw payload. Select XIP with `cmake --preset FU2 --fresh
-DNEON_MDF_PROFILE=xip` and build. See [FU2 build/boot contract](../../guide/FU2.md).

On the machine that is connecting FPGA, replace the serial port and image path according to that machine:

```sh
python3 uart_boot_loader.py \
  --port /dev/ttyUSB2 \
  --image /home/quang-hoang/github_neon_mdf/CDC_NEON_MDF/examples/FPGA_Terminal/build/FU2/fpga_terminal_image.bin \
  --baud 9600 --byte-gap-ms 2 --no-monitor
```

Reset according to the board process for the loader to transmit images. The BootROM Baud of the
above command is 9600, independent of the application baud; use the actual PDI matching value. After
the loader closes the port, open the serial terminal at 19200 baud, 8N1, no local echo and no
hardware flow control. Can be used:

```sh
python3 -m serial.tools.miniterm /dev/ttyUSB2 19200 --raw
```

Exit the miniterm with `Ctrl+]`. Banner may have been transmitted before opening the monitor; press
Enter to get prompt, then enter `help`.

RX buffer and line buffer own full alignment words; character operations using `memcpy`, `memset`
and standard string functions provided by FU2 runtime. Build manually checks for byte/halfword
access in drivers and firmware. Successful build has not confirmed UART running on FPGA.
