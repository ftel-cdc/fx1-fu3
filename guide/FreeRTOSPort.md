# FreeRTOS on GT32U2086

The SDK integrates FreeRTOS-Kernel with the **`GCC_ARM_CM0` port** and **`heap_4`**.
Each application owns its `Inc/FreeRTOSConfig.h`; the integration helper is
`neon-mdf/cmake/freertos.cmake`.

## CMake integration

After including `tools/cmake/project.cmake`:

```cmake
include(${NEON_MDF_PATH}/cmake/freertos.cmake)
neon_mdf_add_freertos(${NEON_MDF_TARGET} ${CMAKE_CURRENT_SOURCE_DIR}/Inc)
neon_mdf_add_executable(my_app Src/main.c)
target_link_libraries(my_app PRIVATE freertos_kernel)
```

The helper creates `freertos_config`, adds the application configuration include
path and links the device target so the kernel/port inherit its word-only SRAM
policy. Cortex-M0 flags are applied to both kernel and port. Preserve this
integration when adding applications.

## Clock, heap and stack

`configCPU_CLOCK_HZ` uses `SystemCoreClock`; the clock must match the hardware.
FPGA peripheral examples typically set
`CONFIG_GT32U2086_EXAMPLE_FIXED_CLOCK_HZ=10000000`.
`configMINIMAL_STACK_SIZE` and stack sizes passed to `xTaskCreate` are measured
in **words**, whereas `configTOTAL_HEAP_SIZE` is in **bytes**. Check the linker
map and actual stack usage before increasing task/queue counts.

## Exception handlers and BootROM

GT32U2086 uses Cortex-M0 without VTOR. BootROM v5 forwards exceptions to the
application vectors through the SRAM IRQ relay. Startup must preserve this
relay; do not substitute linker scripts or startup code that bypass the device profile.

Set `configCHECK_HANDLER_INSTALLATION=0`: the port's VTOR-based vector check is
incompatible with the BootROM relay. Cortex-M0 has no CLZ instruction, so keep
`configUSE_PORT_OPTIMISED_TASK_SELECTION=0`. Set `configENABLE_MPU=0`.
The port provides SVC/PendSV/SysTick handlers using the names expected by startup.

BootROM hands off with `PRIMASK=1`. The FreeRTOS port enables interrupts when
starting the scheduler. Before scheduler startup, only enable interrupts once
handlers and interrupt sources are ready.

## Data and console

GT32U2086 requires aligned 32-bit SRAM word access. The kernel uses the device
target's word-only memory runtime. Queue items should have sizes that are
multiples of four and own every accessed word. Do not pass pointers to local
buffers between tasks after their lifetime has ended.

The console does not automatically serialize concurrent task output. Use a
single logging task or protect sequences of console operations with a mutex
when multiple producers are present.

## Examples

- [FreeRTOS_Blinky](../examples/FreeRTOS_Blinky/): two tasks toggle GPIO at different periods.
- [FreeRTOS_Peripheral_Queue](../examples/FreeRTOS_Peripheral_Queue/README.md):
  transfers peripheral events through a queue, with a UART terminal to run the example.
- `neon-mdf/tools/templates_freertos/`: a single-task template for new applications.

Successful builds/links verify kernel integration. Board execution is needed
to verify interrupts, tick timing, preemption, response time and stack/heap margins.
