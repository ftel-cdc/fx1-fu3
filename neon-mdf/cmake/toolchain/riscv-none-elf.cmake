# Toolchain file for GCC RISC-V (used for CH32V003F4P6, see arch/RISCV/README.md + plan RISC-V in
# .claude/plans/).
#
# Note the FILE name: keeps the name "riscv-none-elf.cmake" (convention written before in
# arch/RISCV/README.md/backends/riscv/README.md from the initial scaffold, but the actual toolchain is
# `riscv64-elf-gcc` (Homebrew, not xPack `riscv-none-elf-gcc`) - verified true `riscv64-elf-gcc`
# translator uses normal rv32ec/ilp32e (this toolchain supports multilib both 32 and 64-bit, the
# name "riscv64" is just a Homebrew name, it does not mean only build to 64-bit) - uses
# TOOLCHAIN_PREFIX to change if you want to use another one later.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv32)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(NOT DEFINED TOOLCHAIN_PREFIX)
  set(TOOLCHAIN_PREFIX riscv64-elf-)
endif()

find_program(RISCV_GCC_PATH ${TOOLCHAIN_PREFIX}gcc)
if(NOT RISCV_GCC_PATH)
  message(FATAL_ERROR
    "${TOOLCHAIN_PREFIX}gcc not found in PATH. Passing Homebrew: "
    "`brew install riscv64-elf-gcc riscv64-elf-gdb` and mail it back.")
endif()

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy CACHE FILEPATH "")
set(CMAKE_OBJDUMP      ${TOOLCHAIN_PREFIX}objdump CACHE FILEPATH "")
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size    CACHE FILEPATH "")

set(CMAKE_EXECUTABLE_SUFFIX ".elf")

# -ffreestanding must be in COMPILE flags (not just linker flags) - missing no,
# GCC's own <stdint.h>, without newlib, uses include_next <stdint.h> to look for another
# stdint.h "common pine" (usually from libc, e.g. newlib) - with this compiler alone there is no
# libc so include_next fails together ("stdint.h: No such file or directory") - true confirmation
# via independent variable test before/after adding this one (error disappears as soon as it
# -ffreestanding in COMPILE flags, not just link flags).
set(CMAKE_C_FLAGS_INIT   "-ffunction-sections -fdata-sections -Wall -ffreestanding")
set(CMAKE_CXX_FLAGS_INIT "-ffunction-sections -fdata-sections -Wall -fno-rtti -fno-exceptions -ffreestanding")
set(CMAKE_ASM_FLAGS_INIT "-x assembler-with-cpp")

# do not use -specs=nano.specs/nosys.specs like ARM - riscv64-elf-gcc (Homebrew) is a custom THUAN
# compiler, does not include newlib (true verified - find libc.a in all Cellar, not found) -
# bare-metal safe build.
#
# libgcc: rv32ec doesn't have extension M (multiply/divide hardware) - enable division/receipt of
# 32-bits in C (e.g. SystemCoreClock/1000 in neon_bootloader_main.c) all need libgcc's software
# functions (__udivsi3, __umodsi3, __muldi3...) - -nostdlib all default messages (use, don't want
# automatic libc/crt0) but still need libgcc separately (this is the compiler runtime helper, not
# libc). don't set -lgcc here (CMAKE_EXE_LINKER_FLAGS_INIT CMake set SOM in link command, before
# SDK's own .a messages - ld only quets 1 times, som setting won't resolve symbols needed from the
# following .a quets - true verified) - see how to link via target_link_libraries(... PUBLIC gcc) on
# target device (see cmake/mcu_variants.cmake) so CMake can use differential (and male in
# --start-group/--end-group of the bootloader, see tools/cmake/project.cmake).
set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib -ffreestanding -Wl,--gc-sections -Wl,--print-memory-usage")
