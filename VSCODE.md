# Dùng neon-mdf với VS Code

Extension `neon-mdf-tools` (`vscode-extension/`) làm toàn bộ
build/flash/debug/monitor/menuconfig qua status bar + Command Palette — mỗi
app không cần `.vscode/tasks.json`/`launch.json` riêng.

## 0. Cài đặt trước

| Công cụ | Dùng để làm gì |
|---|---|
| Extension `ms-vscode.cmake-tools` | Configure/build qua `CMakePresets.json` |
| Extension `ms-vscode.cpptools` | IntelliSense C/C++ |
| Extension `marus25.cortex-debug` | Debug qua OpenOCD (Cortex-M) |
| Extension `neon-mdf-tools` (`vscode-extension/`) | Build/Flash/Debug/Monitor/menuconfig/Tạo App — cài từ `.vsix` (`npm run package && npm run vsix` trong `vscode-extension/`) |
| `arm-none-eabi-gcc` | Toolchain build FU2/WEACT_F412 |
| `openocd` | Nạp/debug bằng debugger trên board có cấu hình hỗ trợ; nạp UART FU2 không cần công cụ này |
| `riscv64-elf-gcc` (Homebrew) | Toolchain cho `CH32V003F4P6` |
| `cmake` ≥ 3.25, `ninja`, `python3` + `pyyaml` + `kconfiglib` | Build system, generator và menuconfig |
| `pyserial` | Tool UART STM32 và nạp FU2 trên Windows; FU2 Linux/macOS không cần gói này |

## 1. Mở project

Mở đúng thư mục app cụ thể (vd `examples/Blinky/`) — **không mở thư mục
`neon_mdf/` gốc**, giống hệt việc không ai mở cả `esp-idf/` làm workspace.
`.vscode/settings.json` của mỗi app đã set sẵn `cmake.environment` nên không
bắt buộc `source export.sh` trước.

## 2. Chọn target

Status bar dưới cùng có mục **"NEON-MDF: <target>"** (hoặc lệnh
**NEON-MDF: Chọn Target**) — danh sách đọc thẳng từ `CMakePresets.json` của
app đang mở, không hardcode. Chọn xong CMake Tools tự re-configure.

## 3. Build

Lệnh **NEON-MDF: Build** hoặc nút Build ở status bar — chạy
`cmake --build --preset <target>`. Mỗi app build ra 1-3 executable tuỳ
target (`bootloader`, app factory, app OTA nếu có `flashmap.csv`).

**menuconfig** (bật/tắt driver UART/I2C, chỉ áp dụng family FU2): lệnh
**NEON-MDF: menuconfig** — mở UI trong Terminal, lưu vào `sdkconfig` của app,
CMake tự re-configure để áp dụng.

## 4. Flash

Lệnh **NEON-MDF: Flash**, hoặc **NEON-MDF: Chọn Transport (UART/JTAG)** trước
nếu muốn đổi cách nạp:

Với **FU2**, chọn **UART**: extension gọi target `flash_uart_fu2` của CMake,
truyền cổng đang chọn và setting `neonMdf.fu2FlashBaudRate` (mặc định 19200)
lúc chạy. Chọn boot mode **100**, chờ terminal báo UART sẵn sàng rồi nhấn
**RESET trên board**. Tool hiển thị version **1.0** và xác nhận hoàn tất theo
ROMCODE; **Build + Flash + Log** mở monitor sau thành công. Baud monitor dùng
`neonMdf.baudRate`, độc lập với baud nạp, mặc định FU2 là 19200 khi chưa đặt.
Xem [hướng dẫn nạp FU2](guide/FU2Flashing.md). OpenOCD FU2 chưa có cấu hình nạp.

Các thao tác dưới đây áp dụng cho board STM32 có cấu hình tương ứng:

- `jtag` (mặc định) — nạp bootloader+app trong 1 phiên OpenOCD/SWD, không
  thao tác tay. Dùng được với `WEACT_F412`.
- `uart` — nạp qua **System Memory Bootloader thật** của chip (ROM, AN3155)
  qua UART thuần, không cần debug probe cho việc ghi flash (SWD chỉ dùng để
  trigger reset). Cần thao tác tay: nối `PB2`→GND + giữ nút `B0` lúc reset
  (WEACT_F412 dùng chung `PB2` cho cả LED lẫn BOOT1 — 2 chân này chập nhau
  trên board, phải cô lập bằng dây nối tay).

`CH32V003F4P6` **chưa có Flash/Debug thật qua extension** — chip này cần
`wlink` (không phải OpenOCD/SWD), chưa được wire vào `flash.ts`/`debug.ts`.
Chọn transport nào cũng sẽ chạy nhầm cơ chế ARM (thất bại rõ ràng ở bước gọi
`openocd`, không có rủi ro ghi nhầm dữ liệu).

**NEON-MDF: Build + Flash + Log** làm cả 3 bước (build → flash → mở Serial
Monitor) theo đúng transport đang chọn.

## 5. Debug

Lệnh **NEON-MDF: Debug** dùng cấu hình OpenOCD của board; FU2 hiện chưa có
cấu hình hoạt động. Với WEACT_F412, tự động flash trước bằng đúng target CMake
(`flash_jtag`/`flash_ota`), dừng ở `main()`, hiện cây thanh ghi peripheral từ
`.svd` sinh tự động (`mcu-description/generated/<TARGET>.svd`).

`CH32V003F4P6` **chưa wire debug** — cần OpenOCD fork riêng của WCH (chưa xác
nhận hoạt động với board thật). Không debug target này qua extension cho tới
khi Phase 5 (xem `project_riscv_ch32v003_plan` nếu có ghi chú nội bộ) hoàn
thành.

## 6. Serial Monitor

Lệnh **NEON-MDF: Serial Monitor**, hoặc **NEON-MDF: Chọn Cổng Serial** trước
nếu cần đổi cổng (có tự động dò cổng qua giao thức SYNC/READY của
bootloader).

## 7. Tạo app mới

Lệnh **NEON-MDF: Tạo App Mới**:

1. Nhập tên app (dùng làm tên thư mục + tên CMake target).
2. Chọn **App thường (bare-metal)** hoặc **App dùng FreeRTOS** — 2 nguồn
   template khác nhau (`neon-mdf/tools/templates/` và
   `tools/templates_freertos/`), cả 2 đều tối giản (không GPIO/LED demo, chỉ
   proof-of-life), khác nhau ở có scheduler hay không.
3. Chọn nơi tạo — mặc định `examples/` cạnh các app có sẵn (mở nguyên thư
   mục gốc `neon_mdf/` hoặc mở 1 app cụ thể đều tự nhận diện đúng vị trí
   `examples/`), hoặc duyệt thư mục khác.

App mới build được ngay cho cả 3 target, có bootloader nếu target đó khai
`bootloader:` trong YAML (không có OTA — muốn OTA thật, thêm `flashmap.csv`
theo mẫu `examples/Blinky/flashmap.CH32V003F4P6.csv`/`OTA_Demo/`).

## 8. `neonMdf.sdkPath` — SDK không nằm ở vị trí quy ước

Mặc định app suy ra vị trí `neon-mdf/` SDK từ vị trí của chính nó
(`<app>/../../neon-mdf`, đúng nếu app nằm trong `examples/`). Nếu SDK ở nơi
khác: lệnh **NEON-MDF: Chọn Đường Dẫn SDK...** (setting Global, giống
`idf.espIdfPath`).

## IntelliSense

CMake Tools tự sinh `compile_commands.json` cho `cpptools` — không cần cấu
hình include path tay. Nếu báo lỗi "file not found" (vd `mcu_device.h`) ngay
sau khi mở project chưa build lần nào — bình thường, file đó do generator
sinh lúc configure, build 1 lần là hết.

## Lỗi thường gặp

| Triệu chứng | Nguyên nhân | Cách sửa |
|---|---|---|
| "NEON-MDF: không tìm thấy target nào trong CMakePresets.json" | Đang mở workspace không phải app cụ thể lẫn không phải repo gốc hợp lệ | Mở `examples/<App>/` hoặc nguyên `neon_mdf/` (extension tự nhận diện cả 2) |
| `core_cm0.h`/`mcu_device.h`: No such file | Chưa build lần nào (generator chưa chạy) | Build 1 lần |
| `arm-none-eabi-gcc`/`riscv64-elf-gcc`: command not found | Thiếu toolchain hoặc không có trong PATH | Cài theo mục 0 |
| Configure lỗi `ModuleNotFoundError: No module named 'yaml'` | Thiếu `pyyaml` | `pip3 install pyyaml kconfiglib` |
| Flash/Debug báo lỗi kết nối OpenOCD trên `CH32V003F4P6` | Chưa wire — xem mục 4/5 | Không dùng Flash/Debug qua extension cho target này, chờ Phase 4/5 |
| Extension có vẻ chạy hành vi cũ sau khi sửa `vscode-extension/src/` | `dist/extension.js` là artifact đã compile, không tự rebuild | `npm run package && npm run vsix` trong `vscode-extension/`, cài lại `.vsix`, Reload Window |

## Tham khảo thêm

- `README.md` — tổng quan SDK, danh sách target, bootloader/OTA/FreeRTOS.
- `ARCHITECTURE.md` — thiết kế chi tiết pipeline sinh code + build system.
