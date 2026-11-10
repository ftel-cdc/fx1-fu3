OUTPUT_ARCH("riscv")
ENTRY(_start)
MEMORY
{
    IMAGE (rwx) : ORIGIN = 0x80000000, LENGTH = 0x00400000
    STACK (rw) : ORIGIN = 0x80400000, LENGTH = 0x00010000 * 2
}
SECTIONS
{
    .text : {
        KEEP(*(.text.init))
        *(.text .text.*)
    } > IMAGE
    .rodata : ALIGN(8) {
        *(.rodata .rodata.* .srodata .srodata.*)
    } > IMAGE
    .data : ALIGN(8) {
        *(.data .data.*)
        __global_pointer$ = . + 0x800;
        *(.sdata .sdata.*)
    } > IMAGE
    .bss (NOLOAD) : ALIGN(8) {
        __bss_start = .;
        *(.sbss .sbss.* .bss .bss.* COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > IMAGE
    .eth_dma (NOLOAD) : ALIGN(4096) {
        __eth_dma_start = .;
        KEEP(*(.eth_dma .eth_dma.*))
        . = ALIGN(4096);
        __eth_dma_end = .;
    } > IMAGE
    .stack (NOLOAD) : {
        __stack_base = .;
        . += 0x00010000 * 2;
        __stack_end = .;
    } > STACK
    /DISCARD/ : { *(.note* .comment .riscv.attributes) }
}
ASSERT(0x80000000 + 0x00400000 <= 0x80400000, "fx1: image region overlaps the stacks")
ASSERT(0x80400000 + 0x00010000 * 2 <= 0x80500000,
       "fx1: stacks overlap the shared region")
ASSERT(0x80500000 + 0x00B00000 <= 0x81000000, "fx1: shared region overlaps RAW buffers")
ASSERT(0x81000000 < 0x88000000, "fx1: RAW and YUV buffer regions out of order")
ASSERT(0x88000000 < 0x90000000, "fx1: YUV buffers and the test area out of order")
ASSERT(0x90000000 < 0x80000000 + 0x20000000, "fx1: test area outside DDR")
