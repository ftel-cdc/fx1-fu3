# Kiến trúc neon-mdf

Tài liệu này mô tả **thiết kế hiện tại** của SDK và lý do đằng sau từng quyết
định — không phải nhật ký các lần thay đổi.

Xem thêm trong `guide/`: `FirstApp.md` (tutorial tạo app đầu tiên),
`AddNewMCU.md` (từng bước thêm target mới),
`ArchitectDetail.md` (đồ thị phụ thuộc thật giữa các khối), `DeviceYAML.md`
(tham chiếu đầy đủ từng field khi khai `mcu-description/devices/<TARGET>.yaml`),
`DriverImplement.md` (cách viết + wire driver mới vào CMake), `MenuConfig.md`
(cách khai tuỳ chọn `menuconfig`/`sdkconfig`), `BootloaderOTA.md` (hướng dẫn
thực hành thêm OTA vào app), `FreeRTOSPort.md` (cách wire FreeRTOS cho kiến
trúc CPU mới), `Debugging.md` (cơ chế debug qua VS Code + gotcha thật).

## 1. Tổng quan pipeline

```
mcu-description/devices/<TARGET>.yaml
        │  (generate.py, chạy tự động lúc CMake configure)
        ▼
header thiết bị + linker script + startup + .svd
        │
        ▼
components/driver (API arch-agnostic, 1 implementation/family)
        │
        ▼
app (Src/main.c) ──link──> neon_mdf_driver, [neon_mdf_bootloader_uart, neon_mdf_ota, freertos_kernel]
```

Nguyên tắc cốt lõi: **1 file YAML mô tả 1 con chip thật (địa chỉ/bit thanh
ghi thật, không đoán)**, mọi thứ CMSIS-style/RISC-V-style sinh ra từ đó bằng
Python. Driver/app chỉ cần biết **API chung** (`NEON_GPIO_*`, `NEON_FLASH_*`)
— implementation thật khác nhau theo family được CMake chọn lúc build, code
app không cần `#ifdef` theo peripheral register.

## 2. `mcu-description/` — nguồn sự thật duy nhất

- `schema/common.schema.yaml` + `schema/arch/{cortex-m,riscv}.schema.yaml` —
  định nghĩa shape hợp lệ của 1 file `.yaml` (field bắt buộc/tuỳ chọn).
- `devices/<TARGET>.yaml` — 1 file/chip: `vendor`/`family`/`arch`/`cpu`,
  `memory` (flash/ram, địa chỉ+size thật), `peripherals` (base address +
  register offset/reset value thật, có comment trích nguồn Reference Manual),
  `interrupts` (số IRQ thật), và tuỳ chọn `bootloader:` (`size`,
  `image_header_size`, `uart:` — bật cơ chế bootloader/OTA cho target đó).
- `tools/mcu_desc/loader.py`/`model.py` — parse YAML thành object Python
  (`Device`/`Peripheral`/`Register`/...), validate theo schema. Peripheral
  cùng `type:` (vd `GPIO`) dùng chung 1 struct C sinh ra — driver viết theo
  `type`, không theo tên instance, tái dùng được cho device khác cùng IP.
- `tools/backends/{cortex_m,riscv}/` — sinh code THẬT từ object đó:
  - `gen_cmsis_header.py` / `gen_riscv_header.py` — header C (struct thanh
    ghi kiểu CMSIS, `#define <NAME>_BASE`, IRQn enum).
  - `gen_linker_script.py` — linker script GNU ld (MEMORY/SECTIONS), và nếu
    có `bootloader:`, sinh thêm 2 bản: 1 cho partition bootloader, 1 cho
    partition app (`generate_bootloader_partitions()`).
  - `gen_startup_vector.py` (Cortex-M) / `gen_startup.py` (RISC-V) — vector
    table + reset handler (copy `.data`, zero `.bss`, gọi `SystemInit()` rồi
    `main()`).
  - `gen_svd.py` — file `.svd` cho debugger (dùng chung cho cả 2 kiến trúc,
    thuần generic từ danh sách peripheral/register).
- `tools/generate.py` — điểm vào, dispatch theo `device.arch` (`cortex-m*` →
  `generate_cortex_m()`, `riscv*` → `generate_riscv()`), chạy tự động qua
  `cmake/mcu_variants.cmake` mỗi lần configure (không commit output vào
  git — `arch/*/Device/*/Include/*.h`, `.../Source/gcc/*` là sinh ra, chỉ
  `system_<TARGET>.c/.h` là viết tay).

`_peripheral_struct()` (sinh struct C từ danh sách register) là hàm THUẦN,
không phụ thuộc kiến trúc CPU — `gen_riscv_header.py` import thẳng từ
`gen_cmsis_header.py` thay vì viết lại.

## 3. `arch/` — tách biệt theo kiến trúc CPU

```
arch/
  CortexM/
    CMSIS/CMSIS_5/          # vendor ARM, chỉ Core/Include (core_cm0.h, core_cm4.h, ...)
    CMSIS/Device/<FAMILY>/  # sinh tự động (Include/, Source/gcc/) + system_<TARGET>.c/.h viết tay
  RISCV/
    Core/                   # CSR macro (riscv_csr.h), soft-math (__udivsi3/__mulsi3/__udivdi3/...), libc shim tối thiểu
    Device/<VENDOR>/<FAMILY>/  # sinh tự động + system_<TARGET>.c/.h viết tay
    Port/                   # PFIC và CSR mstatus: SDK_IRQ_* cho RISC-V/WCH
```

Backend Cortex-M dùng CMSIS trực tiếp: `NVIC_*` để điều khiển từng IRQ và
`__get_PRIMASK()`/`__disable_irq()`/`__set_PRIMASK()` cho critical section.
Các API này có sẵn qua device header/CMSIS; không có source IRQ wrapper
riêng được thêm vào target Cortex-M.

RISC-V/WCH dùng `arch/RISCV/Port/` với API `SDK_IRQ_*` cho PFIC và CSR
`mstatus`. CMake chỉ thêm source/include của Port trong nhánh RISC-V.

RISC-V không có CMSIS — không cần `core_cmX.h`, `__I`/`__O`/`__IO` (qualifier
CMSIS dùng cho struct thanh ghi) được `gen_riscv_header.py` tự định nghĩa lại
giống hệt giá trị CMSIS gốc, để dùng chung logic `_peripheral_struct()`.

## 4. `mcu_device.h` — alias tên cố định

Mỗi device sinh ra 1 file `mcu_device.h` chỉ có `#include "<DEVICE>.h"` —
code arch-generic (`components/driver`, `components/bootloader`) include
`"mcu_device.h"` thay vì hardcode tên device, để không cần `#ifdef` theo tên
target khi chỉ cần biết "device header hiện tại".

## 5. CMake: 1 lần build = 1 target, chọn qua `NEON_MDF_TARGET`

```
tools/cmake/project.cmake        # app include() đúng 1 file này
  └─ cmake/mcu_variants.cmake     # đăng ký target, chạy generate.py, set biến ARCH/FAMILY/LINKER_SCRIPT/...
       └─ add_subdirectory(neon-mdf/)
            ├─ components/driver/     # add_library(neon_mdf_driver), chọn .c theo FAMILY
            ├─ components/bootloader/ # (nếu NEON_MDF_BOOTLOADER_ENABLED) neon_mdf_crc32, neon_mdf_bootloader_uart
            ├─ components/ota/        # (nếu app có flashmap.csv) neon_mdf_ota
            └─ cmake/kconfig.cmake    # sinh sdkconfig.h/.cmake từ Kconfig + sdkconfig của app
```

`neon_mdf_add_executable()`/`neon_mdf_add_app_executable()`/
`neon_mdf_add_bootloader_executable()` (`tools/cmake/project.cmake`) là 3
helper app dùng để tạo executable:

- `neon_mdf_add_executable(name src...)` — build nguyên flash, dùng khi
  target không có `bootloader:` (vd `FU2`), hoặc app không cần chạy dưới
  bootloader.
- `neon_mdf_add_app_executable(name src...)` — link với linker script
  partition "app" (`NEON_MDF_APP_LINKER_SCRIPT`, sinh từ
  `generate_bootloader_partitions()`), yêu cầu target có `bootloader:`.
- `neon_mdf_add_bootloader_executable(name)` — không nhận source, SDK tự
  cung cấp qua `NEON_MDF_BOOTLOADER_MAIN_SOURCES` (xem mục 7).

Các helper tạo executable không tự thêm syscall UART cho `WEACT_F412` hoặc
`STM32F103CB`. `printf()` trên hai target này dùng stub newlib/nosys, không xuất log qua UART.
UART bootloader vẫn phục vụ giao thức bootloader qua API riêng.

Biến CMake từ `mcu_variants.cmake`/`generate.py` cần **2 lần `PARENT_SCOPE`**
để tới được scope của app: 1 lần từ hàm trong `mcu_variants.cmake` ra
`neon-mdf/CMakeLists.txt`, 1 lần nữa từ đó ra app (băng qua
`add_subdirectory(${NEON_MDF_PATH} neon-mdf-build)` trong
`tools/cmake/project.cmake`) — thiếu 1 trong 2 tầng, biến sẽ rỗng ở scope
app mà không báo lỗi rõ ràng (CMake coi biến không tồn tại là rỗng).

Khác ESP-IDF thật 1 điểm: ESP-IDF cho `include()` trước `project()` rồi tự
"shadow" `project()` bằng macro riêng để chèn logic vào giữa; ở đây app gọi
`project()` trước, `include(tools/cmake/project.cmake)` sau — tránh CMake
Warning (dev) khi gọi `project()` từ trong macro qua `include()`, đổi lại
mất đúng thứ tự y hệt ESP-IDF nhưng số dòng boilerplate app phải gõ gần như
không đổi.

## 6. `components/driver` — arch-agnostic, 1 file/family

```
components/driver/
  neon_types.h        # neon_status_t dùng chung
  gpio/neon_gpio.h  # API chung: neon_gpio_clock_enable/config/WritePin/ReadPin/TogglePin
    platform/fu2/neon_gpio.c / platform/weact_f412/neon_gpio.c / platform/ch32v003/neon_gpio.c
  flash/neon_flash.h  # FU2 main array/NVR; backend khác phục vụ bootloader
  crm/, syscsr/      # FU2 clock/reset và pinmux nội bộ
  uart/, i2c/, spi/  # FU2 HAL; UART còn có backend STM32F103 polling
  adc/, dma/, crc/, aes/, trng/, rtc/, tim/, pwm/, qspi/, wdt/
  efuse/             # OTP map cũ, đang tắt trên FU2 hiện tại
```

`components/driver/CMakeLists.txt` chọn đúng file `.c` theo
`NEON_MDF_TARGET_<TARGET>_FAMILY` (không phải theo tên target chính xác) —
nhiều `device:` cùng 1 `family:` (nếu có sau này) dùng chung 1 implementation
driver. GPIO bắt buộc mọi family phải có. FU2 luôn có Flash, CRM và SYSCSR;
Flash của family khác chỉ được thêm khi `NEON_MDF_BOOTLOADER_ENABLED`.
Driver FU2 dùng aligned word 32-bit cho CPU và DMA. Critical section của
backend FU2 lưu/khôi phục PRIMASK trực tiếp qua CMSIS; helper của Flash và
routine reset giữ trong SRAM khi Flash chưa sẵn sàng.

`neon_mdf_add_executable()` (`tools/cmake/project.cmake`) chỉ link thẳng 1
target `neon_mdf_driver` gộp — SDK hiện chưa có hệ thống component/dependency
graph nhiều tầng như ESP-IDF thật (mỗi component 1 `CMakeLists.txt` khai
`REQUIRES` riêng) vì hiện chỉ có đúng 1 component thật (`components/driver`,
`components/bootloader`/`components/ota` được link thẳng khi cần thay vì qua
graph); thêm hệ thống đó khi số lượng component thật sự cần khai phụ thuộc
chọn lọc lẫn nhau.

## 7. `components/bootloader` — bootloader 2-stage qua UART

```
components/bootloader/
  neon_bootloader_main.c        # logic CHUNG cho MỌI kiến trúc/family — KHÔNG có #ifdef theo arch
  neon_bootloader_arch.h        # interface arch-specific: SystickStart1ms/Systick1msElapsed/JumpToApp/SystemReset
    platform/weact_f412/neon_bootloader_arch.c   # CMSIS SysTick + SCB->VTOR
    platform/ch32v003/neon_bootloader_arch.c     # STK (QingKe SysTick) + mtvec
  neon_bootloader_uart.h        # interface UART: config/SendByte/RecvByte/TryRecvByte
    platform/weact_f412/neon_bootloader_uart.c   # USART1 thật (STM32)
    platform/ch32v003/neon_bootloader_uart.c     # USART1 thật (CH32V003, thanh ghi khác STM32 dù cùng tên)
  image_header.h                # struct image_header_t: magic/version/size/crc32
```

`neon_bootloader_main.c` compile TRỰC TIẾP làm source của executable
`bootloader` (không qua thư viện trung gian) — object file trên executable
luôn được linker kéo vào vô điều kiện, tránh vòng lặp phụ thuộc archive giữa
`main.c` (cần `SystemCoreClock` của device) và `startup.s` (cần symbol
`main`).

**Giao thức UART** (giống nhau mọi family, implement bởi
`neon_bootloader_uart_<family>.c`):
1. Boot → đợi ~3s 1 byte SYNC (`0x55`). Không nhận được → kiểm tra app hiện
   có hợp lệ, nhảy vào nếu hợp lệ.
2. Nhận được SYNC → gửi `0x52` (READY) → nhận 16-byte `image_header_t` →
   kiểm tra `magic` → ACK (`0x06`)/NAK (`0x15`).
3. Xoá vùng `[app_base, app_vector_base+size)` (bao nhiêu sector cần thiết,
   không giới hạn 1 sector cố định).
4. Nhận data theo chunk 256 byte, ghi flash ngay, ACK sau mỗi chunk.
5. Tính CRC32, so với header — khớp thì nhảy vào app qua
   `neon_bootloader_arch_jump_to_app()`, không khớp thì NAK + dừng (không bao
   giờ nhảy vào ảnh sai).

**2 mô hình song song trong cùng `neon_bootloader_main.c`**, chọn bằng macro
compile-time `NEON_MDF_FLASHMAP_ENABLED`:
- **Không có** (app không có `flashmap.csv`) — 1 slot cố định
  (`NEON_MDF_APP_BASE`/`APP_VECTOR_BASE`/`APP_SIZE`, tính thẳng từ
  `bootloader.size`/`image_header_size` trong YAML) — UART ghi đè lên chính
  slot đó, không có A/B/rollback.
- **Có** (`components/ota/flashmap.h` sinh từ `flashmap.csv`) — 3 partition
  `app`/`app_ota`/`otadata`, verify+rollback (mục 8).

Phạm vi cố ý KHÔNG làm: không có chữ ký số (chỉ CRC32 — phát hiện lỗi truyền,
không chống giả mạo), không có downgrade protection, single-bank (không có
staging A/B đối xứng).

## 8. `components/ota` + `flashmap.csv` — partition table + OTA

`flashmap.csv` (tuỳ chọn, đặt cạnh `CMakeLists.txt` của app, cú pháp giống
`partitions.csv` của ESP-IDF: `name,type,subtype,size`) được
`tools/gen_flashmap.py` đọc lúc configure (`cmake/flashmap.cmake`), tính địa
chỉ tuần tự, validate mỗi partition rơi đúng ranh giới sector erase thật
(`erase_sectors` trong device YAML), sinh `flashmap.h`/`flashmap.cmake` +
linker script cho mỗi partition `type=app`.

Có thể override theo device: `flashmap.<TARGET>.csv` được ưu tiên trước
`flashmap.csv` mặc định — cần thiết khi 1 app build cho nhiều chip có flash
size chênh lệch lớn (vd `WEACT_F412` 512KB vs `CH32V003F4P6` 16KB), hoặc khi
1 target không đủ chỗ cho OTA (không tạo `flashmap.<TARGET>.csv` cho device
đó → app tự rơi về mô hình "1 slot cố định" ở mục 7, không cần đổi code C
nào).

Mô hình OTA **bất đối xứng** (không phải A/B ping-pong đối xứng):
- `app` (factory) — chỉ ghi lúc nạp ban đầu (JTAG/UART qua bootloader), OTA
  không bao giờ đụng tới → luôn là fallback an toàn.
- `app_ota` — slot duy nhất OTA ghi vào.
- `otadata` — `neon_ota_data_t` (`active_slot`, `state`, `boot_attempts`,
  `magic`+`crc32` để phát hiện hỏng): `components/ota/neon_ota.c` cung cấp
  `neon_ota_read_data/WriteData/ConfirmValid()`, dùng chung bởi cả bootloader
  (đọc để quyết định boot slot nào) và app (gọi `ConfirmValid()` sau khi chạy
  ổn định).

An toàn: app KHÔNG tự confirm + reset lặp lại vượt quá
`NEON_OTA_MAX_BOOT_ATTEMPTS` (mặc định 3) → bootloader tự ghi lại
`active_slot=factory` rồi nhảy vào factory (auto-rollback).

`neon_mdf_crc32` tách thành thư viện riêng (không nằm trong bootloader) để
`components/ota` link được CRC32 mà không kéo theo `main()` của bootloader.

## 9. FreeRTOS (`rtos/FreeRTOS-Kernel/`, `cmake/freertos.cmake`)

Vendor FreeRTOS-Kernel chính thức, dùng port có sẵn — không tự viết port cho
kiến trúc nào. `neon_mdf_add_freertos(<target> <config_dir>)` chọn
`FREERTOS_PORT` theo `arch`:

| arch | Port | Ghi chú |
|---|---|---|
| `cortex-m0*` | `GCC_ARM_CM0` | Priority PendSV/SysTick cố định thấp nhất, không cần `configPRIO_BITS` |
| `cortex-m4` | `GCC_ARM_CM4F` | "Direct Routing" — app phải tự alias `SVC_Handler`/`PendSV_Handler`/`SysTick_Handler` sang `vPortSVCHandler`/`xPortPendSVHandler`/`xPortSysTickHandler` (vector table sinh chung không tự đặt đúng tên port mong đợi) |
| `riscv*` | `GCC_RISC_V_GENERIC` + extension `RISCV_no_extensions` | Không có CLINT thật (chip WCH dùng PFIC) — app tự cấu hình tick timer + chuyển `mtvec` sang Direct mode trong `vPortSetupTimerInterrupt()`/`freertos_risc_v_application_interrupt_handler()` (weak, app override) |

**2 patch nhỏ trong `rtos/FreeRTOS-Kernel/portable/GCC/RISC-V/`** (có giới
hạn phạm vi rõ ràng, comment tại chỗ patch):
- `portContext.h`: 1 chỗ dùng thanh ghi `t3` (x28) làm scratch khi khôi phục
  `mstatus` — không tồn tại trên RV32E (chỉ có x0-x15). Patch: dùng `t0` thay
  thế **chỉ khi** `__riscv_32e` được định nghĩa, giữ nguyên `t3` cho kiến
  trúc RISC-V không phải E-variant.
- `port.c`: 1 biến global dùng `configCPU_CLOCK_HZ` làm initializer —
  SDK này định nghĩa macro đó là biến runtime (`SystemCoreClock`, dùng chung
  mọi target vì clock có thể cấu hình khác nhau lúc chạy), không phải hằng
  compile-time như hầu hết demo FreeRTOS giả định → bọc khai báo trong đúng
  `#if` mà nó thực sự được dùng tới (khi có CLINT thật).

RISC-V toolchain (`riscv64-elf-gcc`, xem mục 10) không có multilib khớp ABI
`ilp32e` → tự cung cấp `__udivdi3`/`__umoddi3` (`arch/RISCV/Core/Src/riscv_soft_math.c`)
cho phép chia 64-bit mà `pdMS_TO_TICKS()` của FreeRTOS luôn dùng nội bộ.

## 10. Toolchain

| Kiến trúc | Toolchain | File | Đặc thù |
|---|---|---|---|
| Cortex-M | `arm-none-eabi-gcc` | `cmake/toolchain/arm-none-eabi.cmake` | newlib nano/nosys.specs — có libc thật |
| RISC-V | `riscv64-elf-gcc` (Homebrew) | `cmake/toolchain/riscv-none-elf.cmake` | `-nostdlib -ffreestanding`, KHÔNG có libc — mọi thứ libc-dependent phải tự cung cấp |

RISC-V không có multilib khớp `-march=rv32ec_zicsr -mabi=ilp32e` (chip
CH32V003 thật: RV32EC, không có extension M — không có nhân/chia phần
cứng) → SDK tự cung cấp:
- `arch/RISCV/Core/Src/riscv_soft_math.c` — `__udivsi3`/`__umodsi3`/
  `__mulsi3`/`__udivdi3`/`__umoddi3` (bit-serial, ưu tiên đúng hơn nhanh).
- `arch/RISCV/Core/Inc/libc_shim/{string,stdlib}.h` +
  `arch/RISCV/Core/Src/riscv_libc_shim.c` — `memcpy`/`memset`/`memmove`/
  `strlen` tối thiểu, chỉ wire vào include path của `freertos_kernel` (SDK
  code tự dùng `__builtin_memcpy()` thay vì `<string.h>`).

**Cố ý KHÔNG** dùng `-march=rv32imac`/ABI `ilp32` (dù toolchain có multilib
sẵn) — sẽ khiến compiler phát ra lệnh `mul`/`div` phần cứng thật không tồn
tại trên silicon CH32V003, lỗi chỉ lộ ra khi chạy thật.

## 11. Kconfig/`sdkconfig`

Giống ESP-IDF: `neon-mdf/Kconfig` (mainmenu) → `rsource
components/driver/Kconfig` (option của SDK) + `osource
"$(NEON_MDF_APP_KCONFIG)"` (Kconfig riêng của app, tuỳ chọn — không lỗi nếu
app không có). `cmake/kconfig.cmake` gọi `tools/kconfig/gen_config_header.py`
(dùng `kconfiglib`) sinh `sdkconfig.h`/`sdkconfig.cmake` từ `sdkconfig` của
app + Kconfig gộp — mọi option gộp chung 1 `menuconfig`/1 file `sdkconfig`,
không phân biệt option SDK hay option app.

Các driver tùy chọn có `CONFIG_NEON_MDF_DRIVER_*` riêng (UART, I2C, SPI,
ADC, DMA, CRC, RTC, TIM, PWM, TRNG, AES, QSPI, WDT). Tắt option loại source
khỏi build. Mapping CMake quyết định family nào có backend cho peripheral đó.

## 12. VS Code extension

`vscode-extension/` — chi tiết dùng ở `VSCODE.md`. Điểm thiết kế chính:
- Đọc target/flash target/executable name **động** qua CMake File API
  (`src/cmakeFileApi.ts`) và `CMakePresets.json` (`src/presets.ts`) — không
  hardcode tên target/executable nào, tự thấy target/app mới không cần sửa
  extension.
- `dist/extension.js` là artifact đã bundle (esbuild) — sửa
  `vscode-extension/src/` cần `npm run package && npm run vsix` rồi cài lại
  `.vsix` để có hiệu lực (không tự rebuild khi mở VS Code).
- "Tạo App Mới" copy từ `neon-mdf/tools/templates/` hoặc
  `tools/templates_freertos/` (2 thư mục nằm trong SDK, không bundle vào
  extension — sửa template không cần rebuild extension), thay token
  `__APP_NAME__`/`__NEON_MDF_RELATIVE_PATH__` trong nội dung file. Thư mục
  đích tự nhận diện được cả 2 trường hợp mở workspace: đúng 1 app cụ thể
  (dùng thư mục cha làm `examples/`), hoặc nguyên thư mục gốc repo (dò
  `<gốc>/examples/` + `<gốc>/neon-mdf/` có marker SDK thật).

## 13. Thêm MCU target mới

Hướng dẫn thực hành từng bước (checklist, bảng file cần sửa): `guide/AddNewMCU.md`.

1. `mcu-description/devices/<TARGET>.yaml` — địa chỉ/bit PHẢI thật (trích
   nguồn Reference Manual trong comment), không đoán. Không có TRM thật →
   đánh dấu rõ PLACEHOLDER; không giả vờ là register map đã được xác nhận.
2. `cmake/mcu_variants.cmake` — đăng ký `NEON_MDF_TARGET_<TARGET>_ARCH`/
   `_FAMILY` (+ `_VENDOR`/`_ABI` nếu RISC-V, do đường dẫn output
   `arch/RISCV/Device/<Vendor>/<Family>/` sâu hơn 1 cấp so với Cortex-M).
3. Viết tay `system_<TARGET>.c/.h` (không sinh tự động — cấu hình clock thật
   sau reset, khác nhau mỗi chip).
4. Family mới (chưa có driver): thêm
   `components/driver/gpio/neon_gpio_<family>.c` (bắt buộc), và nếu cần
   bootloader: `flash/neon_flash_<family>.c`,
   `components/bootloader/neon_bootloader_uart_<family>.c` +
   `neon_bootloader_arch_<family>.c` — cùng dispatch theo FAMILY trong
   `components/driver/CMakeLists.txt`/`components/bootloader/CMakeLists.txt`.
5. `cmake/presets/targets.json` — thêm 1 preset (`include` dùng chung bởi
   mọi app, không cần sửa từng `CMakePresets.json`).
6. Muốn FreeRTOS trên arch mới: thêm nhánh trong `cmake/freertos.cmake`
   (chọn `FREERTOS_PORT` phù hợp, xem mục 9).

**Nhiều `device:` cùng 1 `family:`** — dùng chung 1 thư mục
`arch/*/Device/<Family>/` (mỗi device chỉ khác tên file bên trong) và dùng
lại nguyên `components/driver` (guard theo FAMILY, không phải theo tên
target chính xác).

## 14. Giới hạn/rủi ro đã biết

- `FU2`: register map theo Common Specification v8 và hợp đồng BootROM v5;
  compile/link và MMIO emulation chưa thay thế việc đo timing trên FPGA.
  PAD index 12..23 chưa có đầy đủ encoding cấu hình điện ở mức field.
- `CH32V003F4P6`: build/link-verified nhưng chưa từng chạy trên board thật
  (board chưa về) — mọi giả định về hành vi runtime (bootloader UART, FreeRTOS
  tick qua STK, GPIO) dựa trên đọc kỹ Reference Manual thật, chưa có xác nhận
  bằng đo đạc thật.
- Flash/Debug qua VS Code extension chỉ hoạt động thật cho Cortex-M
  (OpenOCD/SWD) — `CH32V003F4P6` cần `wlink` (flash) + OpenOCD fork riêng của
  WCH (debug), chưa wire.
- Bootloader OTA của STM32/CH32 chỉ kiểm tra CRC32. FU2 RTL BootROM v5 có
  đường image ký số qua `fu2_image.py`; ví dụ mặc định tạo image non-secure.
