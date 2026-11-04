# Phụ thuộc giữa các khối trong neon-mdf

Tài liệu này vẽ **đồ thị phụ thuộc thật** giữa các khối (CMake target +
include-dependency ở mức source) — dựa trên `target_link_libraries()`/
`add_subdirectory()`/`#include` thật trong repo, không phải sơ đồ lý
thuyết. Dùng khi cần biết "sửa khối X có ảnh hưởng khối Y không" hoặc "muốn
dùng API của Z thì phải link thêm gì".

## 1. Sơ đồ layer (thấp → cao)

```
Layer 0  mcu-description/tools/          (Python, chạy lúc CMake configure — không phải runtime dependency)
              │ sinh ra
              ▼
Layer 1  arch/{CortexM,RISCV}/Device/<Family>/   → target neon_mdf_device_<TARGET>
         arch/{CortexM,RISCV}/Core/ (RISC-V: soft-math, libc shim, CSR macro)
              │
              ▼
Layer 2  Cortex-M: CMSIS-Core            (NVIC/PRIMASK dùng trực tiếp từ device header)
         arch/RISCV/Port/                (PFIC — SDK_IRQ_*, compile chung vào target device RISC-V)
         cmake/kconfig.cmake             → target neon_mdf_kconfig (INTERFACE)
              │
              ▼
Layer 3  components/driver/              → target neon_mdf_driver
              │
              ├──────────────┬───────────────────┐
              ▼               ▼                    ▼
Layer 4  components/       cmake/flashmap.cmake   (app tự dùng thẳng
         bootloader/       → neon_mdf_flashmap      neon_mdf_driver, vd
         → neon_mdf_       (INTERFACE)              app không bootloader)
           bootloader_uart,      │
           neon_mdf_crc32        ▼
              │            components/ota/
              │            → neon_mdf_ota
              │            (link: neon_mdf_driver,
              │             neon_mdf_crc32, neon_mdf_flashmap)
              │                  │
              └──────┬───────────┘
                     ▼
Layer 5  executable "bootloader" (compile trực tiếp neon_bootloader_main.c —
         không có target CMake trung gian) — link neon_mdf_bootloader_uart,
         neon_mdf_driver, neon_mdf_crc32, [neon_mdf_ota nếu có flashmap.csv]

Layer 6  app executable (Src/main.c của từng ví dụ) — link neon_mdf_driver,
         [neon_mdf_bootloader_uart, neon_mdf_ota nếu app có OTA],
         [freertos_kernel nếu dùng FreeRTOS]
```

`rtos/FreeRTOS-Kernel/` (target `freertos_kernel`/`freertos_kernel_port`) và
`vscode-extension/`/`tools/neon_mdf.py` **không nằm trong đồ thị link** —
FreeRTOS chỉ nhận cờ kiến trúc (`-march`/`-mcpu`) qua
`neon_mdf_apply_{cortex_m,riscv}_arch()`, không link `neon_mdf_device_<TARGET>`;
extension/CLI là công cụ dev-time gọi CMake từ bên ngoài, không phải code
compile chung vào firmware.

## 2. Bảng phụ thuộc từng CMake target (đã xác nhận trong source, không suy đoán)

| Target | Kiểu | `target_link_libraries(... PUBLIC ...)` | Định nghĩa tại |
|---|---|---|---|
| `neon_mdf_device_<TARGET>` | STATIC | *(không phụ thuộc target SDK nào khác — nền tảng thấp nhất)* | `cmake/mcu_variants.cmake` |
| `neon_mdf_kconfig` | INTERFACE | *(chỉ include dir tới `sdkconfig.h`)* | `cmake/kconfig.cmake` |
| `neon_mdf_driver` | STATIC | `neon_mdf_device_<TARGET>`, `neon_mdf_kconfig` | `components/driver/CMakeLists.txt` |
| `neon_mdf_crc32` | STATIC | `neon_mdf_device_<TARGET>` | `components/bootloader/CMakeLists.txt` |
| `neon_mdf_bootloader_uart` | STATIC | `neon_mdf_device_<TARGET>` | `components/bootloader/CMakeLists.txt` |
| `neon_mdf_flashmap` | INTERFACE | *(chỉ include dir tới `flashmap.h` sinh ra)* | `cmake/flashmap.cmake` |
| `neon_mdf_ota` | STATIC | `neon_mdf_driver`, `neon_mdf_crc32`, `neon_mdf_flashmap` | `components/ota/CMakeLists.txt` |
| `freertos_config` | INTERFACE | *(chỉ include dir tới `FreeRTOSConfig.h` của app)* | `cmake/freertos.cmake` |
| `freertos_kernel`, `freertos_kernel_port` | STATIC/OBJECT | *(từ `rtos/FreeRTOS-Kernel/`, không link `neon_mdf_device_<TARGET>` — chỉ nhận cờ `-march`/`-mcpu` qua `neon_mdf_apply_*_arch()`)* | `cmake/freertos.cmake` |
| executable `bootloader` | EXE | `neon_mdf_bootloader_uart`, `neon_mdf_driver`, `neon_mdf_crc32`, + `neon_mdf_ota` nếu app có `flashmap.csv` | `tools/cmake/project.cmake` (`neon_mdf_add_bootloader_executable`) |
| app executable | EXE | `neon_mdf_driver` (+ tuỳ app: `neon_mdf_bootloader_uart`, `neon_mdf_ota`, `freertos_kernel`) | `CMakeLists.txt` của từng app |

**`PUBLIC`** ở mọi target trên là chủ đích — 1 target link `neon_mdf_driver`
tự động có luôn include dir + compile flags (`-mcpu`/`-march`) của
`neon_mdf_device_<TARGET>` mà không cần link tay 2 lần. Đây là lý do
`components/driver/CMakeLists.txt` chỉ cần `target_link_libraries(neon_mdf_driver PUBLIC ${NEON_MDF_DEVICE_TARGET} ...)`
1 dòng, không phải lặp lại `target_compile_options` ở mọi nơi khác.

## 3. Include-dependency ở mức source (đã grep xác nhận, không suy đoán)

Bất biến layering: **khối thấp không bao giờ `#include` khối cao hơn nó**.

- `components/driver/` — **không** include gì từ `components/bootloader/`
  hay `components/ota/`. Chỉ include `mcu_device.h` (Layer 1) +
  `sdkconfig.h` (Layer 2).
- `arch/RISCV/Port/` — không include gì từ `components/driver/`; cung cấp
  API `SDK_IRQ_*` cho PFIC/CSR. Backend Cortex-M include device/CMSIS header
  và gọi trực tiếp NVIC/PRIMASK, không có dependency vào Port.
- `components/bootloader/neon_bootloader_main.c` — điểm DUY NHẤT trong SDK
  include ngược lên `components/ota/` (`flashmap.h`, `neon_ota.h`), và CHỈ
  khi `NEON_MDF_FLASHMAP_ENABLED` được định nghĩa (app có `flashmap.csv`) —
  không phải include vô điều kiện. Đây là ngoại lệ có chủ đích: bootloader
  cần đọc/ghi otadata để quyết định boot slot nào, logic đó thuộc về
  `components/ota` (dùng chung với app, không viết lại).
- `components/ota/neon_ota.c` — include `flash/neon_flash.h` (Layer 3, cùng
  cấp `components/driver`) + `neon_bootloader_crc32.h` (Layer 4,
  `components/bootloader`) — **không** include `neon_bootloader_main.c` hay
  bất kỳ file `_main`/`_arch` nào của bootloader (chỉ cần CRC32, không cần
  logic main loop).

## 4. Thứ tự `add_subdirectory()` thật (`neon-mdf/CMakeLists.txt`)

```
1. cmake/mcu_variants.cmake       → neon_mdf_device_<TARGET>, biến ARCH/FAMILY/LINKER_SCRIPT/...
2. cmake/flashmap.cmake           → (NẾU app có flashmap.csv) neon_mdf_flashmap,
                                     add_subdirectory(components/ota) → neon_mdf_ota
                                     -- PHẢI chạy TRƯỚC bước 4, vì components/bootloader
                                        cần link neon_mdf_ota nếu có flashmap
3. cmake/kconfig.cmake            → neon_mdf_kconfig, target menuconfig
4. add_subdirectory(components/driver)     → neon_mdf_driver
5. add_subdirectory(components/bootloader) → (NẾU NEON_MDF_BOOTLOADER_ENABLED)
                                     neon_mdf_crc32, neon_mdf_bootloader_uart,
                                     biến NEON_MDF_BOOTLOADER_MAIN_SOURCES
```

Toàn bộ `add_subdirectory(neon-mdf/)` này tự nó nằm trong
`tools/cmake/project.cmake`, được `include()` bởi `CMakeLists.txt` của
**từng app** — mỗi app tự chạy lại cả chuỗi này độc lập (không có 1 lần
build SDK dùng chung nhiều app, giống hệt cách mỗi ví dụ `esp-idf/examples/`
tự build lại toàn bộ IDF).

## 5. Ví dụ cụ thể: app có OTA (vd `OTA_Demo`, target `WEACT_F412`)

```
OTA_Demo/CMakeLists.txt
  └─ include(tools/cmake/project.cmake)
       └─ add_subdirectory(neon-mdf/)
            ├─ neon_mdf_device_WEACT_F412        [Layer 1]
            ├─ neon_mdf_flashmap + neon_mdf_ota  [Layer 3-4, vì có flashmap.csv]
            ├─ neon_mdf_kconfig                  [Layer 2]
            ├─ neon_mdf_driver                   [Layer 3]
            └─ neon_mdf_crc32, neon_mdf_bootloader_uart  [Layer 4]
  ├─ neon_mdf_add_bootloader_executable(bootloader)
  │     link: neon_mdf_bootloader_uart, neon_mdf_driver, neon_mdf_crc32, neon_mdf_ota
  └─ 3× add_executable(ota_demo[_ota][_ota_bad] Src/main.c)
        link: neon_mdf_driver, neon_mdf_bootloader_uart, neon_mdf_ota
```

4 executable cuối cùng (`bootloader`, `ota_demo`, `ota_demo_ota`,
`ota_demo_ota_bad`) đều link `neon_mdf_ota`+`neon_mdf_driver` — nhưng
**khác linker script** (bootloader dùng `NEON_MDF_BOOTLOADER_LINKER_SCRIPT`,
3 app dùng 3 file `.ld` khác nhau sinh từ `flashmap.csv`, mỗi partition 1
file) nên vẫn là 4 file `.bin` độc lập dù cùng "cây phụ thuộc" CMake.

## 6. Vì sao layering này quan trọng khi sửa code

- Sửa `components/driver/gpio/platform/<family>/neon_gpio.c` — chỉ ảnh hưởng
  executable link `neon_mdf_driver` trở lên (mọi thứ). An toàn để sửa mà
  không lo phá `arch/RISCV/Port/`.
- Sửa `arch/RISCV/Port/Src/sdk_irq.c` — ảnh hưởng các build RISC-V sử dụng
  lớp PFIC/CSR này; source được compile chung vào `neon_mdf_device_<TARGET>`.
  Kiểm tra các ứng dụng RISC-V gọi `SDK_IRQ_*` khi sửa file này.
- Sửa `components/ota/neon_ota.c` — ảnh hưởng cả bootloader (đọc otadata
  lúc boot) LẪN app (ghi otadata lúc confirm) — 2 nơi dùng CHUNG 1
  implementation, không phải 2 bản riêng, nên sửa logic ở đây tự động nhất
  quán giữa 2 phía (đây là lý do `neon_ota.c` tồn tại như 1 component riêng
  thay vì code riêng trong `neon_bootloader_main.c`).
- Sửa `components/bootloader/neon_bootloader_main.c` — **không** ảnh hưởng
  `components/driver`/`components/ota` (chiều phụ thuộc 1 hướng), nhưng là
  file rủi ro cao nhất khi sửa (duy nhất chạy trước khi có app nào khác kịp
  chạy — lỗi ở đây có thể khiến board không boot được gì cả).
