# GT32U2086 board and clock configuration

The compiler target and preset are `GT32U2086`. FU2 is an older product name,
not a CMake preset or CLI target. The BootROM signing domain `FU2-IMAGE-V1`
is a fixed protocol constant and must retain its spelling.

## Select the actual clock

GT32U2086 is rated up to 60 MHz. FPGA peripheral examples normally describe a
10 MHz bitstream; assigning a software frequency does not change its clock.
An incorrect `SystemCoreClock` affects UART baud, SPI/QSPI rates, timer periods
and the FreeRTOS tick.

| Application | Clock setting |
|---|---|
| Peripheral examples | `CONFIG_GT32U2086_EXAMPLE_FIXED_CLOCK_HZ` in menuconfig/sdkconfig |
| `Printf_Log` | CMake `NEON_PRINTF_FIXED_CLOCK_HZ`, default `10000000` |
| New bare-metal/FreeRTOS template | CMake `NEON_APP_FIXED_CLOCK_HZ`, default `0` |

`0` selects CRM clock detection in `SystemCoreClockUpdate()`. For a template
application on a known 10 MHz FPGA, run from the app directory:

```sh
cmake --preset GT32U2086 -DNEON_APP_FIXED_CLOCK_HZ=10000000
cmake --build --preset GT32U2086
```

Existing `Blinky`/`FreeRTOS_Blinky` use the CRM policy unless the device target
receives `GT32U2086_FPGA_FIXED_CLOCK_HZ`. A hand-written application can add
this after including `tools/cmake/project.cmake` for a fixed board clock:

```cmake
target_compile_definitions(${NEON_MDF_DEVICE_TARGET} PUBLIC
  GT32U2086_FPGA_FIXED_CLOCK_HZ=10000000UL)
```

Use the actual measured/configured frequency. Verify UART bit timing, a TIM/PWM
period and the FreeRTOS tick on the board after changing its bitstream.

## Pins and connected devices

Default driver `AUTO` routes refer to package pins and do not establish
FPGA/PMOD connector wiring. Record board revision, bitstream, connector,
voltage, pin mux and attached device before running a peripheral test.

| Function | SDK default |
|---|---|
| UART0 console | TX package 50, RX package 51; 19200 baud, 8N1 |
| GPIO input/output | PB5 button to GND with pull-up; PB7 output |
| SPI0/W5500 | SCK=60, MISO=61, MOSI=62, SS0=63; verify PMOD mapping |
| I2C0 | SDA=18, SCL=19; external pull-ups required |
| QSPI | SCK=68, CS0=72, IO0=71, IO1=70, IO2=69, IO3=64 |

Use the example README and board schematic together. `KEEP` leaves routing to
board code; release an `AUTO` route before another driver takes ownership.
Boot values `100`, `000`, `101` and `111` are logical SoC values, not physical
switch positions; see [boot modes](GT32U2086BootModes.md).

## Storage reservations

`Flash_Read_Write` reserves internal sector `0x0009FE00..0x0009FFFF`.
The two QSPI NOR tests default to external offsets `0x0003C000..0x0003CFFF`.
Reserve these regions before destructive tests. QSPI test applications require
the normal profile and execute from internal Flash; mapped reads are separate
from booting firmware directly from QSPI.
