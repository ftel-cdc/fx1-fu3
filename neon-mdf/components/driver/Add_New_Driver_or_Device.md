# Add device and driver

`components/driver` creates a single CMake target `neon_mdf_driver`. Each peripheral has its own
directory and adds its own source to the target via `target_sources()`. The backend selection is
based on family, not device name:

```text
NEON_MDF_TARGET = STM32F103CB
             └─ NEON_MDF_TARGET_STM32F103CB_FAMILY = STM32F103
                                                        └─ choose platform/stm32f103/neon_gpio.c
```

It is necessary to clearly distinguish the two cases below.

## Add device with existing family

For example, add another STM32F103 variant, still using the same register layout and
backend `STM32F103`.

1. Create `mcu-description/devices/<DEVICE>.yaml`.
2. Registration of `<DEVICE>` in `cmake/mcu_variants.cmake`:

   ```cmake
   list(APPEND NEON_MDF_KNOWN_TARGETS <DEVICE>)
   set(NEON_MDF_TARGET_<DEVICE>_ARCH cortex-m3)
   set(NEON_MDF_TARGET_<DEVICE>_FAMILY STM32F103)
   ```

3. declare device specific properties if needed, e.g. FPU, vendor, ABI or `bootloader:` in YAML.
4. Configure and build app with `-DNEON_MDF_TARGET=<DEVICE>` or the corresponding preset.

Do not fix CMake in `components/driver` if the new device actually uses the same backend as the old
family. If register layout is significantly different, creating a new family will be seen because it
tries to re-use the old family.

## Add new family

For example the family `MY_MCU` has its own GPIO and UART.

1. Register device with `NEON_MDF_TARGET_<DEVICE>_FAMILY MY_MCU` in
   `cmake/mcu_variants.cmake`.
2. Adding `MY_MCU` to guard allows drivers in `neon-mdf/CMakeLists.txt`.
3. Create a source backend in the appropriate peripheral, for example:

   ```text
   gpio/platform/my_mcu/neon_gpio.c
   uart/platform/my_mcu/neon_uart.c
   ```

4. Add mapping to `CMakeLists.txt` of only peripherals that family supports. Example in
   `gpio/CMakeLists.txt`:

   ```cmake
   neon_mdf_driver_select_family_source(
     TARGET neon_mdf_driver
     PERIPHERAL gpio
     REQUIRED
     SOURCES
       FU2     ${CMAKE_CURRENT_LIST_DIR}/platform/fu2/neon_gpio.c
       MY_MCU  ${CMAKE_CURRENT_LIST_DIR}/platform/my_mcu/neon_gpio.c
   )
   ```

`REQUIRED` for GPIO: missing backend, configuration must be failed. With optional drivers such as
ADC, SPI, I2C, do not use `REQUIRED`; unsupported family will have no source added.

5. If device has bootloader, add Flash mapping in `flash/CMakeLists.txt`. This file only requires
   backend when `NEON_MDF_BOOTLOADER_ENABLED` is `ON`.
6. Build at least one app for the new family. Check that the build log contains compile
   `platform/my_mcu/neon_<peripheral>.c` files.

## Add new driver/peripheral

Additional examples require drivers.

1. Create folder `components/driver/can/`, put header and source here:

   ```text
   can/neon_can.h
   can/platform/<family>/neon_can.c
   can/CMakeLists.txt
   ```

2. Add `can` to the list of `foreach(... add_subdirectory(...))` in `components/driver/CMakeLists.txt`.
   This is a compulsory change to let CMake know read CMake's new folder.
3. If the driver can be disabled to reduce Flash, add symbol Kconfig to `components/driver/Kconfig`,
   for example `NEON_MDF_DRIVER_CAN`.
4. Write `can/CMakeLists.txt` and call helper. Examples need only on STM32F103 and WEACT_F412:

   ```cmake
   if(CONFIG_NEON_MDF_DRIVER_CAN)
     neon_mdf_driver_select_family_source(
       TARGET neon_mdf_driver
       PERIPHERAL can
       SOURCES
         STM32F103  ${CMAKE_CURRENT_LIST_DIR}/platform/stm32f103/neon_can.c
         WEACT_F412 ${CMAKE_CURRENT_LIST_DIR}/platform/weact_f412/neon_can.c
     )
   endif()
   ```

5. Include header by qualified path from other app/source:

   ```c
   #include "can/neon_can.h"
   ```

Do not add `target_include_directories()` for each peripheral: include path public is already root
`components/driver`, so the above include method works for all
driver.

In driver, public API uses `neon_<module>_<action>()`, private helper
`static ... <module>_<action>()`, type `_t`, field `snake_case`. Header guard
and the presentation follows [convention V1.3](../../../guide/NEON_Coding_Convention_V1.3.md). The
backend is in `platform/<family>/` with name `neon_<module>.c`. UART/I2C separates initialize handle
and configuration hardware. Interface pinmux internal FU2 is in
`syscsr/platform/fu2/neon_syscsr_internal.h`.

## `neon_mdf_driver_select_family_source()`

Helper in `components/cmake/driver_helpers.cmake` has the form:

```cmake
neon_mdf_driver_select_family_source(
  TARGET <target>
  PERIPHERAL <peripheral-name>
  [REQUIRED]
  SOURCES
    <FAMILY_1> <source_1>
    <FAMILY_2> <source_2>
)
```

It takes the family of the target being built, finds the corresponding `FAMILY source` pair and
calls `target_sources(<target> PRIVATE <source>)`. Use `${CMAKE_CURRENT_LIST_DIR}` for source so
that the path is always a directory containing the current `CMakeLists.txt`.

## Check before sending see change

```sh
cmake --preset <DEVICE>
cmake --build --preset <DEVICE>
```

It is recommended to check both an old device and family (to make sure not to damage the existing
backend) and the newly added device/family. when disabling Kconfig of the optional driver, confirm
its source does not appear in the log build.
