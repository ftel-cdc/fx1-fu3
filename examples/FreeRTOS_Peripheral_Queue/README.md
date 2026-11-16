# FreeRTOS_Peripheral_Queue

UART RX interrupt and a periodic FreeRTOS task send events to one queue. The terminal
task receives the events during a run, controls **GPIOB7 (PB7)** and prints the results.

This is an independent FU2 firmware. Its UART terminal waits at boot; press `r` without Enter
to start the example at the `fpga:>>` prompt.
The first `r` is received before RTOS initialization. After flashing through UART0,
open the serial monitor and press Enter to redisplay the prompt, then `r`. Startup
logs show peripheral initialization, scheduler startup and terminal task entry.
Console: UART0, 19200 baud, 8N1. FPGA system clock: 10 MHz by default.

Source files are in `Src/`, with the terminal entry in `Src/main.c`. Example headers are in `Inc/`.

`main.c` follows the other FU2 examples, including `GPIO_Input_Output`: print the
banner to initialize the default UART0, borrow its handle with
`neon_console_get_uart()` and check for initialization failure. The terminal loop
is also in `main.c`: print `fpga:>>`, handle Enter/CRLF, wait for `r`, print `[RUN]`,
call `free_rtos_peripheral_queue_run(uart)` and print the prompt again on return.
The first wait for `r` runs before the scheduler starts. Subsequent waits and all
demo runs execute in the terminal task, allowing blocking queue APIs during a run.
There is no separate UART handle or explicit console attachment.

Before starting the scheduler, `free_rtos_peripheral_queue_init(uart)` initializes
PB7, enables the UART FIFO and creates the queue and periodic task once. It waits
for the banner's TX output to finish before updating the existing UART configuration,
as in `UART_RX_Interrupt`. Baud rate and frame format are preserved.

At the prompt the terminal polls UART RX. Once the scheduler is running it yields
for one tick when no byte is available. On `r`, the run function takes over RX using interrupts and consumes the
queue in the same task. Ctrl+C sets PB7 low, disables RX interrupts, discards pending
events and returns to the terminal loop. Polling and ISR never consume RX together.

## Hardware

- Connect the board's UART0 console to a serial terminal at 19200 baud, 8N1.
- Observe PB7 with a logic analyzer, or connect an LED with a suitable series resistor.
  Share ground with the board. `1`/`0` mean electrical high/low; LED polarity depends on wiring.
- PB7 is configured as a GPIO output, initially low. Starting or stopping a run also sets it low.
  Periodic events report its state without changing it.

## FreeRTOS integration

The build follows `../FreeRTOS_Blinky`: include `neon-mdf/cmake/freertos.cmake`,
call `neon_mdf_add_freertos()` with this application's `Inc/` directory, and link
`freertos_kernel`. The helper selects `GCC_ARM_CM0` and `heap_4` for FU2.
`Inc/FreeRTOSConfig.h` uses `SystemCoreClock`, a 1 kHz tick, an 8 KiB heap and stack
overflow/allocation failure hooks. Software timers are disabled.
`configCHECK_HANDLER_INSTALLATION=0` is required for FU2: Cortex-M0 has no VTOR,
and BootROM v5 forwards interrupts to the application's vector table. The port's
default direct-routing check reads VTOR and can halt before the first task.

```text
UART0 RX FIFO -> USART0_IRQHandler -> xQueueSendFromISR --+
                                                       +-> Event Queue -> run() in terminal task -> PB7 / UART log
Periodic task -> vTaskDelayUntil -> xQueueSend ----------+
```

The queue defaults to 16 events, each containing three aligned `uint32_t` fields:
event type, value and tick timestamp. Values are copied into the queue, which meets
FU2's word-access SRAM requirements without passing temporary buffer pointers.

The periodic task runs at priority 2 with a 128-word stack. It sends an event every
1000 ms while the demo is active; the first event follows the task's existing cadence.
The terminal task runs at priority 1 with a 512-word stack. During a run it blocks
in `xQueueReceive()` when no events are available. Only this task writes console
output after startup.
The producer briefly suspends scheduling around its run-state check and nonblocking
send, preventing a stale periodic send across stop/restart. RX interrupts remain enabled
during this short scheduling suspension when the demo is running.

The scheduler, queue and tasks are created once after the first `r`. Ctrl+C returns from the
run function to its caller in the terminal task; the scheduler keeps running.
Starting again reuses the same objects and resets the run statistics. The Cortex-M0 port does not support
returning to `main()` through `vTaskEndScheduler()`.

## Build and load

```sh
cd examples/FreeRTOS_Peripheral_Queue
cmake --preset FU2
cmake --build --preset FU2
cmake -E env NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 NEON_MDF_UART_LOG=full \
  cmake --build --preset FU2 --target flash_uart_fu2
```

Load `build/FU2/fu2_freertos_peripheral_queue_image.bin` when selecting an image manually. This file includes the FU2
128-byte image header; `fu2_freertos_peripheral_queue.bin` is the raw payload. Open a serial monitor after the loader
releases the port. Use the board's UART-download/normal-boot straps as described in
[FU2 build and boot](../../guide/FU2.md).

## Run

At `fpga:>>`, press `r` without Enter to start a run.

| Key | Action |
| --- | --- |
| `1` | Set PB7 high. |
| `0` | Set PB7 low. |
| `t` | Toggle PB7. |
| `s` | Print processed events, queue-full drops, queued events, PB7 state, free heap and stack high-water marks. |
| Ctrl+C | Stop, discard pending events, set PB7 low and return to `fpga:>>`. |

Example output (tick values and counts depend on input timing):

```text
Press r to run FreeRTOS_Peripheral_Queue (no Enter).
fpga:>> r
[RUN] FreeRTOS_Peripheral_Queue
[READY] queue=16 events, period=1000 ms, output=GPIOB7=0
1=high, 0=low, t=toggle, s=stats, Ctrl+C=stop
[UART] key='1' -> PB7=1
[PERIODIC] seq=1 tick=1000 PB7=1
[UART] key='t' -> PB7=0
```

Both producers use nonblocking queue sends. When full, new events are dropped and
counted separately for UART and periodic events. Periodic sequence gaps can therefore
indicate dropped events. `processed` counts all received demo events, including command
characters and CR/LF; it excludes the start key and the stop signal. Queue-full drops
exclude events deliberately discarded during stop. Stats are snapshots, so new UART
data may arrive while a report is printing.

Ctrl+C is recorded in a separate ISR flag and attempts to enqueue a wake-up event.
If the queue is full the terminal task is already runnable, so stopping does not depend on
free queue space. An in-progress command/log may finish first. Bytes received after
Ctrl+C are drained and discarded until stop completes; wait for `fpga:>>` before `r`.
This protects against software queue overflow; hardware UART overruns can still lose input.

`r` only starts a run when the application is at `fpga:>>`.
At this prompt, Enter starts a new line and prints `fpga:>>` again. CR, LF and CRLF are
supported; CRLF produces only one new prompt.

## Configuration and checks

Configure with `cmake --build --preset FU2 --target menuconfig`, then build again.

| Setting | Default | Purpose |
| --- | --- | --- |
| `CONFIG_FU2_EXAMPLE_FIXED_CLOCK_HZ` | `10000000` | Must match the actual FPGA clock for UART and RTOS timing. |
| `CONFIG_FREERTOS_PERIPHERAL_QUEUE_LENGTH` | `16` | Queue capacity, from 1 to 128 events. |
| `CONFIG_FREERTOS_PERIPHERAL_QUEUE_PERIOD_MS` | `1000` | Periodic interval, from 100 to 60000 ms. |

Suggested hardware checks:

1. Confirm PB7 starts low, then use `r`, `1`, `0`, `t` and check both pin level and logs.
2. Leave the terminal idle during a run: periodic logs continue at the configured interval.
3. Send `10t` in one short burst: commands execute in arrival order if the queue has space.
4. Send a long burst to fill the queue, then `s`: dropped events should be reported.
   Send Ctrl+C during a burst and confirm PB7 returns low and the prompt returns with RX IRQ disabled.
5. Repeat stop/start and `s`: free heap should remain constant; stack-free values are
   lifetime high-water marks and may decrease as new code paths execute.
6. At the prompt, check CR, LF and CRLF; CRLF should produce only one new prompt.

Build success validates compilation, linking and image packaging. Pin routing, UART
interrupt delivery, timing and stack margins still need verification on FU2 hardware.
