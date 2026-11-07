/* Reserve the last FU2 main-array sector; it is not part of the image. */
MEMORY
{
  FLASH_RW_SCRATCH (rw) : ORIGIN = 0x0009FE00, LENGTH = 0x200
}

SECTIONS
{
  .flash_rw_scratch (NOLOAD) :
  {
    __flash_rw_start__ = .;
    . += LENGTH(FLASH_RW_SCRATCH);
    __flash_rw_end__ = .;
  } > FLASH_RW_SCRATCH
}
INSERT AFTER .data;

ASSERT(ORIGIN(FLASH) == 0x00020080 || ORIGIN(FLASH) == 0x30000080,
       "Flash_Read_Write requires the FU2 normal or XIP image profile")
ASSERT(ORIGIN(FLASH) == 0x30000080 || LOADADDR(.data) + SIZEOF(.data) <= __flash_rw_start__,
       "Flash_Read_Write image overlaps the Flash_Read_Write scratch sector")
