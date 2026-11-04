# Toolchain file cho GCC ARM Embedded (arm-none-eabi-*).
# Dùng: cmake --preset <variant> (xem CMakePresets.json) hoặc
#       cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/arm-none-eabi.cmake ..

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

if(NOT DEFINED TOOLCHAIN_PREFIX)
  set(TOOLCHAIN_PREFIX arm-none-eabi-)
endif()

find_program(ARM_GCC_PATH ${TOOLCHAIN_PREFIX}gcc)
if(NOT ARM_GCC_PATH)
  message(FATAL_ERROR
    "${TOOLCHAIN_PREFIX}gcc khong tim thay trong PATH. Cai GNU Arm Embedded "
    "Toolchain (vd `brew install arm-none-eabi-gcc` tren macOS - formula, KHONG PHAI cask - xem SETUP.md) roi thu lai.")
endif()

set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy CACHE FILEPATH "")
set(CMAKE_OBJDUMP      ${TOOLCHAIN_PREFIX}objdump CACHE FILEPATH "")
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size    CACHE FILEPATH "")

set(CMAKE_EXECUTABLE_SUFFIX ".elf")

set(CMAKE_C_FLAGS_INIT   "-ffunction-sections -fdata-sections -fno-common -Wall")
set(CMAKE_CXX_FLAGS_INIT "-ffunction-sections -fdata-sections -fno-common -Wall -fno-rtti -fno-exceptions")
set(CMAKE_ASM_FLAGS_INIT "-x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-specs=nano.specs -specs=nosys.specs -Wl,--gc-sections -Wl,--print-memory-usage")
