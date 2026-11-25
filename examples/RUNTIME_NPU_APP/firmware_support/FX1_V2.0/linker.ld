OUTPUT_ARCH(riscv)
/* SPDX-License-Identifier: Apache-2.0 */

ENTRY(_start)

/* FW/RTOS window of RAM0 (CDC_FW_BASE .. +16 MiB). Task stacks and the
 * FreeRTOS heap live in .bss inside this window; the fixed NPU demo buffers
 * (VPU_OUT0 / NPU_WEIGHTS0 / NPU_WORK0) are outside it by construction.
 * Test vectors (test_input, test_output_tflite) are placed in TEST_DATA at
 * 0x8C000000 (the unallocated 64 MiB window above NPU_WORK0) so that large
 * single-op verification vectors do not overflow the 16 MiB firmware window. */
MEMORY
{
    RAM (rwx)     : ORIGIN = 0x80000000, LENGTH = 16M
    TEST_DATA (r) : ORIGIN = 0x8C000000, LENGTH = 64M
}

SECTIONS
{
    .test_vectors : {
        *test_input*(.rodata*)
        *test_output_tflite*(.rodata*)
    } > TEST_DATA

    . = ORIGIN(RAM);

    .text : {
        *(.text.init)
        *(.text*)
    } > RAM

    .rodata : { *(.rodata*) *(.srodata*) } > RAM
    .data   : { *(.data*) *(.sdata*) } > RAM

    .bss (NOLOAD) : {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss*)
        *(.sbss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > RAM

    /* Boot stack for _start/main before the scheduler runs. Tasks use their
     * own heap-allocated stacks; ISRs use the port's static xISRStack. */
    . = ALIGN(16);
    _stack_top = ORIGIN(RAM) + LENGTH(RAM);
}
