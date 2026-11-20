# GT32U2086 support and hardware acceptance

The SDK contains implemented examples and automated host build checks.
Hardware acceptance requires measurements on the specific board and bitstream.
Retain these with the firmware revision; a build, local image check or timeout
path alone does not establish peripheral hardware validation.

| Module | Example or API | Acceptance evidence and limits |
|---|---|---|
| GPIO | Input/output and IRQ | PB5/PB7 routing, pull-up, edge count and debounce |
| UART/console | Polling, IRQ, printf | Baud/frame timing, RX/TX data, errors and idle |
| UART DMA | TX/RX menu | Word-per-character peripheral profile and timeout recovery remain incomplete; compare against the hardware USART DMA contract |
| I2C | EEPROM, DMA and slave APIs | Unshifted addresses, repeated START, NACK, pull-ups and DMA requests; no standalone slave example |
| SPI | W5500 polling and DMA | VERSIONR, restored register patterns, 32-byte SRAM comparison, CS hold and request routing |
| TIM/PWM | Periodic IRQ and PWM | Output period and live STT duration; software flag clear unsupported on the current FPGA backend |
| ADC | 1/4/16-sample batches | Input range, channel route, sample count and saturation; CPU processing uses whole words |
| RTC | Calendar menu | Readback, five second increments, minute/leap-day/year rollover, invalid inputs and RTC source/prescaler |
| CRC | Six vector/boundary tests | CRC-8 polynomial `0x07`, all 32 bits per input word MSB first; length counts words |
| AES | ECB encrypt/decrypt | FIPS-197 vector and native word order; CBC/CTR need protocol-specific vectors |
| TRNG | Polling and digital BIST | FPGA timeouts and LFSR BIST do not validate silicon entropy quality |
| Internal Flash | Reserved-sector read/write | Erase/program/readback and overlap protection; NVR provisioning is separate |
| QSPI | Indirect NOR and mapped reads | JEDEC ID, QE/status, scratch region and original-pattern comparison; DDR/DMA/automatic status polling unsupported |
| WDT | Public driver APIs | Refresh and deliberate expiry/reset; no dedicated standalone example |
| FreeRTOS | Blinky and peripheral queue | Tick, preemption, IRQ forwarding, stack/heap margins and ownership |
| JTAG | Attach integration point | OpenOCD is a placeholder; use a verified board/probe configuration |

There is no `FPGA_Terminal`, `DMA_Memory_Copy`, `I2C_Register_Read_Write`,
`I2C_Slave` or `QSPI_Flash_Indirect` project. The [example index](../examples/README.md)
lists actual project and firmware names.

## Record a board result

Retain the SDK revision, board/bitstream, toolchain, profile, clock source,
`sdkconfig`, image version/hash, boot mode, wiring, UART log and scope/logic
captures. Record expected/observed values, recovery and final pass/fail.
For DMA, verify completion and bus retirement before reusing buffers. SPI DMA
parks incomplete commands and requires reset before another run; parking and
timeout are not acknowledged aborts.

## Automated checks

Install normal Python requirements plus `cryptography`, then run:

```sh
python .github/scripts/validate_sdk.py
```

The CI runner copies applications to a temporary directory and checks all
examples, both templates, image header/size/CRC/vectors, selected Release builds,
CLI/CMake configuration refresh, concurrent normal/XIP directories and signing.
It never opens UART or provisions NVR. Build logs remain in the printed location;
`--work-dir build/validation` selects an empty destination.

Its conservative instruction audit rejects byte/halfword load/store candidates
in CMake firmware objects, including the vendored kernel. Legitimate Flash/MMIO
accesses require explicit review. It does not prove address ranges or inspect
prebuilt toolchain libraries. `.github/workflows/sdk.yml` runs these checks and
the VS Code TypeScript build; validation scripts are kept outside SDK tools.
