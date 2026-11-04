# neon-mdf

FU2 uses a single `FU2` target/preset with the RTL BootROM v5 layout. Application
vectors are at `0x00020080`; builds produce `<app>_image.bin` containing the
128-byte header to load at `0x00020000`. See [FU2 build and boot contract](guide/FU2.md).


SDK phong cách ESP-IDF cho vi điều khiển embedded: **1 file mô tả MCU
(YAML) sinh tự động toàn bộ header/linker script/startup**, driver arch-agnostic
dùng chung API trên nhiều family chip, bootloader + OTA thật, và app dùng SDK
qua đúng 1 dòng `include()` — mỗi app là 1 project CMake độc lập hoàn toàn,
không nằm trong SDK.

Hỗ trợ cả **Arm Cortex-M** (CMSIS) và **RISC-V** (tự viết CSR/startup, không
CMSIS) từ cùng 1 pipeline sinh code.

**Cài đặt môi trường trước khi bắt đầu**: `SETUP.md` (Windows/macOS/Linux).

Quy ước code: [NEON Coding Convention V1.3](guide/NEON_Coding_Convention_V1.3.md)
và [cách áp dụng trong Neon MDF](guide/NEON_MDF_Source_Format_Guideline.md).

## Target hiện có

| Target | Vendor | Core | Flash | RAM | Bootloader/OTA | Trạng thái |
|---|---|---|---|---|---|---|
| `FU2` | FPT Telecom | Cortex-M0, tối đa 60MHz | 512KB | 128KB | RTL BootROM v5 | Register map v8; build/link và MMIO emulation, timing FPGA cần đo thực tế |
| `WEACT_F412` | STMicroelectronics (STM32F412RET6) | Cortex-M4F, 100MHz | 512KB | 256KB | Có | **Hardware-verified** — board thật, dùng làm test rig chính |
| `CH32V003F4P6` | WCH | RISC-V QingKe V2A (RV32EC), 48MHz | 16KB | 2KB | Có (tuỳ app, xem dưới) | Build/link-verified, **chưa chạy trên board thật** |

## Cấu trúc thư mục

```
neon_mdf/
  neon-mdf/                  # === SDK — độc lập hoàn toàn, không app nào bên trong ===
    export.sh                  # set NEON_MDF_PATH (giống export.sh của ESP-IDF)
    CMakeLists.txt              # build neon_mdf_driver + neon_mdf_device_<TARGET>
    Kconfig                     # mainmenu, source Kconfig của components + app

    mcu-description/            # nguồn sự thật — MỌI header/linker/startup sinh ra từ đây
      schema/                    # shape của .yaml (common + theo arch: cortex-m/riscv)
      devices/<TARGET>.yaml      # mô tả 1 MCU: memory map, peripheral, IRQ, bootloader
      tools/                     # generator Python (loader, model, backends/{cortex_m,riscv})
      generated/                 # output phụ (.svd cho debugger, fragment .cmake) — gitignored

    arch/
      CortexM/
        CMSIS/CMSIS_5/            # vendor ARM CMSIS-Core (chỉ Core/Include)
        CMSIS/Device/<FAMILY>/    # header/linker/startup sinh tự động + system_<TARGET>.c viết tay
      RISCV/
        Core/                    # CSR macro, soft-math (__udivsi3/...), libc shim tối thiểu
        Device/<VENDOR>/<FAMILY>/ # header/linker/startup sinh tự động + system_<TARGET>.c viết tay
        Port/                    # PFIC (IRQ enable/disable/priority, critical section)

    components/
      console/                   # FU2 printf/_write qua UART; biên dịch khi bật driver UART
      driver/                    # HAL theo family; FU2 clock/pinmux, serial, timer, DMA, Flash, WDT...
      bootloader/                # bootloader 2-stage: main.c dùng chung + glue arch-specific/family-specific
      ota/                       # đọc/ghi otadata, dùng chung bởi bootloader và app

    rtos/FreeRTOS-Kernel/        # vendor FreeRTOS chính thức + 2 patch nhỏ cho port RISC-V (RV32E-safe)

    cmake/                       # toolchain, arch flags, mcu_variants, flashmap, freertos, flash_targets, kconfig
    tools/
      cmake/project.cmake        # file DUY NHẤT app cần include
      templates/                 # nguồn "Tạo App Mới" — app thường (bare-metal)
      templates_freertos/        # nguồn "Tạo App Mới" — app dùng FreeRTOS
      neon_mdf.py                # CLI kiểu idf.py (build/flash/monitor/menuconfig)
      gen_flashmap.py, build_*.py # sinh partition table + đóng gói ảnh flash

  examples/
    Blinky/                     # 1 pin GPIO toggle — build cả 3 target, có bootloader+OTA (WEACT_F412/CH32V003F4P6)
    FreeRTOS_Blinky/            # 2 task FreeRTOS toggle 2 pin — build cả 3 target
    OTA_Demo/                   # 3 executable (factory/ota/ota-lỗi-cố-ý) minh hoạ verify+auto-rollback
    Printf_Log/                 # FU2: printf word-only qua _write/UART0, chỉ cần stdio.h
    UART_ECHO_Polling/          # FU2: UART0 polling echo từng byte, menuconfig baud/clock/timeout
    GPIO_Input_Output/          # FU2: nhấn PB5 bật LED PB1, nhả nút tắt LED, log UART0
    FPGA_Terminal/             # FU2: terminal UART0, help/info/clear; thư mục testcase chờ bổ sung

  vscode-extension/             # extension VS Code: build/flash/monitor/debug/menuconfig, xem VSCODE.md
```

Mỗi thư mục trong `examples/<App>/` tự có `CMakeLists.txt` + `CMakePresets.json`
+ `.vscode/` riêng — configure/build **độc lập hoàn toàn** với nhau và với
`neon-mdf/`, giống mỗi ví dụ trong `esp-idf/examples/` là 1 project riêng.

FU2 Flash Tool **1.0** cung cấp target CMake `flash_uart_fu2`, truyền port/baud
lúc chạy và chờ người dùng nhấn RESET sau khi chọn boot mode 100. Xem
[Nạp firmware FU2 qua UART](guide/FU2Flashing.md).
Quy trình chuẩn bị image và chạy firmware cho từng chế độ nằm trong
[Chế độ khởi động FU2](guide/FU2BootModes.md).

Đồ thị phụ thuộc thật giữa các khối trên (target CMake nào link target
nào): `guide/ArchitectDetail.md`.

## Build thử

Đây là ví dụ có sẵn — muốn tạo app mới từ đầu (wizard VS Code hoặc copy tay),
xem tutorial đầy đủ: `guide/FirstApp.md`.

```sh
. neon-mdf/export.sh              # set NEON_MDF_PATH — 1 lần/terminal session
cd examples/Blinky
cmake --preset WEACT_F412
cmake --build --preset WEACT_F412
# -> build/WEACT_F412/{bootloader,blinky,blinky_ota}.{elf,bin,map}
```

Đổi `WEACT_F412` thành `FU2` hoặc `CH32V003F4P6` để build cho target khác —
cùng 1 `Src/main.c`, driver thật khác nhau theo family được CMake tự chọn.

## Bootloader + OTA

`WEACT_F412`, `STM32F103CB` và `CH32V003F4P6` có khai `bootloader:` trong YAML → mỗi app build
ra thêm executable `bootloader` (SDK tự cung cấp `main.c`), nạp qua UART
(giao thức riêng: SYNC → header 16-byte (magic/size/crc32) → chunk 256 byte →
verify CRC32) hoặc qua SWD/JTAG.

Nếu app có file `flashmap.csv` (khai `app`/`otadata`/`app_ota`, kiểu
`partitions.csv` của ESP-IDF) — bootloader chạy mô hình OTA **bất đối xứng**:
slot `app` (factory) không bao giờ bị OTA đụng tới, slot `app_ota` là nơi duy
nhất OTA ghi vào, `otadata` lưu trạng thái xác nhận (`active_slot`/`state`/
`boot_attempts`). App phải tự gọi `neon_ota_confirm_valid()` sau khi chạy ổn
định — không xác nhận + reset lặp lại quá `NEON_OTA_MAX_BOOT_ATTEMPTS` (mặc
định 3) sẽ **tự động rollback về factory**. Đã hardware-verify đầy đủ
(factory boot, OTA switch, auto-rollback) trên `WEACT_F412`.

Hướng dẫn thực hành thêm OTA vào app mới (viết `flashmap.csv`, sửa
`CMakeLists.txt`, gọi API, test factory→OTA→rollback thật): `guide/BootloaderOTA.md`.

`CH32V003F4P6` **có OTA thật** cho app bare-metal — `examples/Blinky` build 3
executable (`bootloader`/`blinky`/`blinky_ota`) qua
`flashmap.CH32V003F4P6.csv` (12KB còn lại sau bootloader 4KB, đủ chỗ cho 2
bản ảnh). Riêng `examples/FreeRTOS_Blinky` trên **cùng chip** này KHÔNG có
OTA (không có file `flashmap.CH32V003F4P6.csv` cho app đó) — FreeRTOS-Kernel
tối thiểu đã chiếm ~8.5KB flash/~2KB RAM (do that 2026-08-11: text+data=8488B, bss=1936B), không đủ chỗ cho 2 bản ảnh
(factory+OTA) trong 16KB flash/2KB RAM thật của chip, chỉ build 1 executable
qua `neon_mdf_add_app_executable()`. Bootloader trên target này luôn thật
(verify header/CRC32, nhận firmware mới qua UART) ở cả 2 trường hợp — chỉ
khác là `FreeRTOS_Blinky` không có A/B/rollback, ghi đè lên đúng 1 slot cố
định.

`FU2` không khai `bootloader:` → build nguyên flash, 1 executable duy nhất,
không OTA.

## FreeRTOS

`FreeRTOS_Blinky` build cho cả 3 target, dùng port chính thức của
FreeRTOS-Kernel (không tự viết port):

- `FU2` (Cortex-M0): port `GCC_ARM_CM0`.
- `WEACT_F412` (Cortex-M4F): port `GCC_ARM_CM4F`.
- `CH32V003F4P6` (RISC-V, không có CLINT): port `GCC_RISC_V_GENERIC` +
  chip-extension `RISCV_no_extensions` — app tự cấu hình tick timer (STK của
  QingKe) và chuyển `mtvec` sang Direct mode trong `vPortSetupTimerInterrupt()`
  (xem comment đầu `Src/main.c`). 2 patch nhỏ, có giới hạn phạm vi rõ ràng,
  được áp vào `neon-mdf/rtos/FreeRTOS-Kernel/portable/GCC/RISC-V/` để port
  chạy đúng trên RV32E (register file rút gọn 16 thanh ghi).

Cách wire FreeRTOS cho 1 kiến trúc CPU mới (cơ chế dispatch, pattern
"Direct Routing", pattern "CLINT-less"): `guide/FreeRTOSPort.md`.

## Log bằng printf trên FU2

Với FU2, bật `CONFIG_NEON_MDF_DRIVER_UART=y` là có cả UART và `printf/_write`,
không cần cờ printf riêng. Tắt UART sẽ loại cả driver và console khỏi build.
App chỉ cần `<stdio.h>` và `printf("counter=%u hex=0x%08lX\n", 1U, 0x1234UL)`.
Output đi qua `_write`, tự khởi tạo UART0 ở 19200 baud, 8N1 trong lần ghi đầu
tiên. `neon_console_attach_uart()` là API tùy chọn nếu cần UART/baud khác.
Bộ formatter dùng word-only memory access, hỗ trợ chuỗi/số nguyên/hex, chưa
hỗ trợ float hoặc toàn bộ stdio. STM32F103 và WEACT_F412 không có retarget
printf sang UART; ứng dụng dùng API UART trực tiếp hoặc tự cung cấp console.

Example: [Printf_Log](examples/Printf_Log/README.md). API, cách nối UART, xem log
và kiểm chứng: [guide/PrintfLogging.md](guide/PrintfLogging.md).

## menuconfig (Kconfig)

Mỗi app có file `sdkconfig` riêng quyết định driver nào được compile (cách
khai option mới: `guide/MenuConfig.md`):

```sh
cmake --build --preset WEACT_F412 --target menuconfig   # giống idf.py menuconfig
cmake --preset WEACT_F412                                 # re-configure để áp dụng
```

## VS Code

Mở đúng thư mục app cụ thể (vd `examples/Blinky/`), **không mở
`neon_mdf/` gốc**. Cài extension `neon-mdf-tools` (xem `vscode-extension/`) —
status bar chọn target/transport/port, lệnh Build/Flash/Debug/Serial
Monitor/menuconfig, wizard "Tạo App Mới" (chọn bare-metal hoặc FreeRTOS). Chi
tiết: `VSCODE.md`. Cơ chế debug (flash-trước-rồi-attach, 2 gotcha thật đã
gặp và fix trên phần cứng): `guide/Debugging.md`.

## CLI (không cần VS Code)

```sh
. neon-mdf/export.sh
cd examples/Blinky
python3 $NEON_MDF_PATH/tools/neon_mdf.py build flash-jtag monitor
```

Hành động: `build`, `flash-jtag`, `flash-uart-stm32`, `flash-ota`, `monitor`,
`menuconfig`, `set-target` — chạy tuần tự nhiều hành động trong 1 lệnh.

## Thêm MCU target mới

Hướng dẫn chi tiết từng bước: `guide/AddNewMCU.md`. Tóm tắt:

1. Viết `neon-mdf/mcu-description/devices/<TARGET>.yaml` theo
   `schema/common.schema.yaml` + `schema/arch/<arch>.schema.yaml` (địa chỉ
   phải THẬT — không có register layout thật thì đánh dấu rõ PLACEHOLDER,
   không đoán). Tham chiếu đầy đủ từng field: `guide/DeviceYAML.md`.
2. Đăng ký trong `neon-mdf/cmake/mcu_variants.cmake`
   (`NEON_MDF_TARGET_<TARGET>_{ARCH,FAMILY,...}`).
3. Viết tay `system_<TARGET>.c/.h` (không sinh tự động — cấu hình clock thật
   của chip).
4. Nếu family mới: thêm implementation GPIO (`components/driver/gpio/`), và
   nếu cần bootloader: Flash + UART (`components/driver/flash/`,
   `components/bootloader/`) — cách viết + wire CMake: `guide/DriverImplement.md`.
5. Thêm preset trong `neon-mdf/cmake/presets/targets.json` (dùng chung cho
   mọi app qua `include`).

## Tài liệu chi tiết (`guide/`)

| File | Nội dung |
|---|---|
| `guide/FirstApp.md` | Tutorial nhập môn: tạo app mới, cấu trúc file, build/nạp/xem log, sửa code. |
| `guide/AddNewMCU.md` | Từng bước thêm 1 MCU target mới — YAML, đăng ký CMake, driver, bootloader/OTA tuỳ chọn, FreeRTOS tuỳ chọn, checklist. |
| `guide/DeviceYAML.md` | Tham chiếu đầy đủ từng field khi khai `mcu-description/devices/<TARGET>.yaml`. |
| `guide/DriverImplement.md` | Cách khai API driver dùng chung + viết implementation theo family + 3 kiểu wire vào CMake (bắt buộc/có điều kiện/qua Kconfig). |
| `guide/MenuConfig.md` | Cách khai tuỳ chọn `menuconfig`/`sdkconfig` (bool/int/hex/string), dùng trong C và CMake. |
| `guide/PrintfLogging.md` | FU2 printf qua UART: cấu hình, format hỗ trợ, Serial Monitor và kiểm chứng word-only. |
| `guide/ArchitectDetail.md` | Đồ thị phụ thuộc thật giữa các target CMake (`neon_mdf_driver`/`neon_mdf_ota`/...) + include-dependency giữa các layer. |
| `guide/BootloaderOTA.md` | Hướng dẫn thực hành thêm OTA vào app — `flashmap.csv`, `CMakeLists.txt`, API, test factory→OTA→rollback thật. |
| `guide/FreeRTOSPort.md` | Cách wire FreeRTOS cho 1 kiến trúc CPU mới — dispatch port, pattern "Direct Routing", pattern "CLINT-less". |
| `guide/Debugging.md` | Cơ chế debug qua VS Code + 2 gotcha thật đã gặp và fix trên phần cứng (header bị `load` xoá, reset không tin cậy). |

`ARCHITECTURE.md` giải thích **vì sao** thiết kế như vậy; `guide/` giải
thích **cách làm** cụ thể.

## Giới hạn hiện tại

- `FU2`: register map theo Common Specification v8, image theo RTL BootROM
  v5. MMIO emulation chưa thay thế đo timing và kiểm chứng trên FPGA; xem
  `guide/FU2.md` và `FU2_PORTING_STATUS.md`.
- `CH32V003F4P6`: toàn bộ (driver, bootloader, RISC-V port của FreeRTOS)
  build/link-verified nhưng **chưa chạy trên board thật** (board đặt mua
  chưa về) — flash qua VS Code extension/CLI cũng chưa wire (cần `wlink`,
  khác cơ chế OpenOCD/SWD của 2 target Cortex-M).
- Debug qua VS Code hiện chỉ hỗ trợ Cortex-M (OpenOCD/SWD) — RISC-V cần
  OpenOCD fork riêng của WCH, chưa xác nhận hoạt động.
- timeout của driver polling là số vòng chờ, không phải mili-giây. SRAM/DMA
  trên FU2 dùng aligned word 32-bit; buffer byte phải sở hữu trọn các word.
