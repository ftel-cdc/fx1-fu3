# Toolchain file cho GCC RISC-V (dung cho CH32V003F4P6, xem
# arch/RISCV/README.md + plan RISC-V trong .claude/plans/).
#
# LUU Y TEN FILE: giu ten "riscv-none-elf.cmake" (quy uoc da ghi truoc trong
# arch/RISCV/README.md/backends/riscv/README.md tu luc RESERVED) nhung
# TOOLCHAIN THAT dung la `riscv64-elf-gcc` (Homebrew, KHONG PHAI xPack
# `riscv-none-elf-gcc`) - da xac nhan THAT `riscv64-elf-gcc` bien dich dung
# rv32ec/ilp32e binh thuong (toolchain nay ho tro multilib ca 32 va 64-bit,
# ten "riscv64" chi la ten goi Homebrew, khong co nghia CHI build duoc
# 64-bit) - dung TOOLCHAIN_PREFIX de doi neu sau nay muon dung ban khac.

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv32)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES TOOLCHAIN_PREFIX)

if(NOT DEFINED TOOLCHAIN_PREFIX)
  set(TOOLCHAIN_PREFIX riscv64-elf-)
endif()

find_program(RISCV_GCC_PATH ${TOOLCHAIN_PREFIX}gcc)
if(NOT RISCV_GCC_PATH)
  message(FATAL_ERROR
    "${TOOLCHAIN_PREFIX}gcc khong tim thay trong PATH. Cai qua Homebrew: "
    "`brew install riscv64-elf-gcc riscv64-elf-gdb` roi thu lai.")
endif()

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy CACHE FILEPATH "")
set(CMAKE_OBJDUMP      ${TOOLCHAIN_PREFIX}objdump CACHE FILEPATH "")
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size    CACHE FILEPATH "")

set(CMAKE_EXECUTABLE_SUFFIX ".elf")

# -ffreestanding PHAI o COMPILE flags (khong chi linker flags) - thieu no,
# <stdint.h> rieng cua GCC (khong kem newlib) lam `include_next <stdint.h>`
# mong doi tim them 1 stdint.h "he thong" (thuong tu libc, vd newlib) - VOI
# rieng compiler nay KHONG co libc nao ca nen include_next that bai cung
# ("stdint.h: No such file or directory") - da xac nhan that qua test bien
# dich doc lap TRUOC/SAU khi them co nay (loi bien mat ngay khi co
# -ffreestanding o COMPILE flags, khong phai chi link flags).
set(CMAKE_C_FLAGS_INIT   "-ffunction-sections -fdata-sections -Wall -ffreestanding")
set(CMAKE_CXX_FLAGS_INIT "-ffunction-sections -fdata-sections -Wall -fno-rtti -fno-exceptions -ffreestanding")
set(CMAKE_ASM_FLAGS_INIT "-x assembler-with-cpp")

# KHONG dung -specs=nano.specs/nosys.specs nhu ban ARM - riscv64-elf-gcc
# (Homebrew) la compiler THUAN TUY, KHONG kem newlib (da xac nhan that -
# tim libc.a trong toan bo Cellar, khong thay) - build hoan toan bare-metal.
#
# libgcc: rv32ec KHONG co extension M (nhan/chia phan cung) - BAT KY phep
# chia/nhan 32-bit nao trong C (vd SystemCoreClock/1000 trong
# neon_bootloader_main.c) deu can ham phan mem cua libgcc (__udivsi3,
# __umodsi3, __muldi3...) - -nostdlib tat CA thu vien mac dinh (dung, KHONG
# muon libc/crt0 tu dong) nhung VAN can libgcc rieng (day la runtime helper
# cua compiler, KHONG PHAI libc). KHONG dat -lgcc o day (CMAKE_EXE_LINKER_FLAGS_INIT
# duoc CMake dat SOM trong lenh link, TRUOC cac thu vien .a cua chinh SDK -
# ld chi quet 1 lan, dat som se KHONG resolve duoc symbol can tu cac .a
# quet SAU - da xac nhan that) - thay vao do link qua
# target_link_libraries(... PUBLIC gcc) tren target device (xem
# cmake/mcu_variants.cmake) de CMake tu dat dung vi tri (VA nam trong
# --start-group/--end-group cua bootloader, xem tools/cmake/project.cmake).
set(CMAKE_EXE_LINKER_FLAGS_INIT "-nostdlib -ffreestanding -Wl,--gc-sections -Wl,--print-memory-usage")
