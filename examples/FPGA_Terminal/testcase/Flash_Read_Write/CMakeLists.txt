target_sources(fpga_terminal PRIVATE ${CMAKE_CURRENT_LIST_DIR}/flash_read_write.c)

# The INSERT script must precede the SDK linker script on the command line.
target_link_options(fpga_terminal BEFORE PRIVATE
  -T${CMAKE_CURRENT_LIST_DIR}/flash_read_write.ld)
set_property(TARGET fpga_terminal APPEND PROPERTY LINK_DEPENDS
  ${CMAKE_CURRENT_LIST_DIR}/flash_read_write.ld)
