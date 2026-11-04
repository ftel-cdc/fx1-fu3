# Cài đặt môi trường phát triển

Hướng dẫn cài dependency trước khi dùng neon-mdf — cho cả build/chạy ví dụ
có sẵn lẫn viết driver/thêm target mới. **Chỉ macOS (Homebrew) đã được xác
nhận thật trong dự án này** (mọi build/flash/debug thật trong lịch sử dự án
đều chạy trên macOS) — phần Linux/Windows dưới đây là hướng dẫn tốt nhất
theo hiểu biết chung, **chưa được test thật**, đánh dấu rõ ở từng mục.

## 1. Bảng tổng quan — cần gì tuỳ mục đích

| Mục đích | Cần cài |
|---|---|
| Chỉ build ví dụ có sẵn (`examples/`) | CMake, Ninja, Python3 + `pyyaml`/`kconfiglib`, toolchain compiler theo target muốn build |
| Nạp FU2 qua UART ROMCODE | Linux/macOS dùng thư viện chuẩn Python; Windows cần `pyserial`. Chọn boot mode 100 và reset thủ công; xem `guide/FU2Flashing.md` |
| Flash/debug Cortex-M qua debugger (board có cấu hình hỗ trợ) | + `openocd`; cấu hình FU2 hiện là placeholder |
| + Build cho `CH32V003F4P6` (RISC-V) | + toolchain `riscv64-elf-gcc` (xem mục 4) |
| + Flash `CH32V003F4P6` thật | + `wlink` (Rust) — **chưa wire vào CMake target nào, xem `README.md` mục giới hạn** |
| Viết driver/thêm target mới (`guide/AddNewMCU.md`) | Như "chỉ build" + trình đọc PDF Reference Manual thật của chip (không có công cụ nào trong repo hỗ trợ bước này, đọc tay) |
| Dùng VS Code (khuyến nghị) | + VS Code, extension `cmake-tools`/`cpptools`/`cortex-debug`, Node.js (build extension `neon-mdf-tools` từ source) |

## 2. Công cụ chung (mọi platform)

- **CMake ≥ 3.25** — build system chính (`cmake/presets/targets.json` và
  các example dùng định dạng CMakePresets phiên bản 6, cần CMake từ 3.25).
- **Ninja** — generator dùng trong mọi `CMakePresets.json` của SDK.
- **Python 3** + pip package:
  ```sh
  pip3 install pyyaml kconfiglib
  ```
  - `pyyaml` — bắt buộc, `mcu-description/tools/generate.py` đọc file `.yaml`.
  - `kconfiglib` — bắt buộc nếu dùng `menuconfig` (xem `guide/MenuConfig.md`).
  - `pyserial` — cần cho tool UART STM32, tự dò cổng của CLI và nạp/monitor
    FU2 trên Windows. Cài thêm bằng `python3 -m pip install pyserial` vào đúng
    interpreter đang dùng. Với Python hệ thống Ubuntu/Debian có thể dùng
    `sudo apt install python3-serial` cho các chức năng cần thư viện này.
    Nạp/monitor FU2 trên Linux/macOS với port chỉ định dùng thư viện chuẩn
    Python, không cần cài thêm `pyserial`.
    Xem [phần chuẩn bị nạp FU2](guide/FU2Flashing.md#chuẩn-bị).
- **Git** — không bắt buộc để build (repo hiện chưa `git init`), nhưng cần
  nếu cài `wlink` qua `cargo install --git ...` (mục 4).

## 3. Toolchain Cortex-M (`FU2`, `WEACT_F412`)

Compiler `arm-none-eabi-gcc` + (muốn flash/debug board thật) `openocd`.

**macOS** (Homebrew, đã xác nhận thật — verify lại trực tiếp trên máy dùng
để viết guide này, `arm-none-eabi-gcc` 10.3-2021.07):
```sh
brew install arm-none-eabi-gcc cmake ninja openocd
```

**Linux** (Debian/Ubuntu — tên gói theo distro, chưa test thật trong dự án
này):
```sh
sudo apt install gcc-arm-none-eabi cmake ninja-build openocd python3-pip
```
Nếu bản trong repo distro cũ, cân nhắc tải trực tiếp "Arm GNU Toolchain"
từ trang chính thức ARM (developer.arm.com) thay vì gói hệ thống.

**Windows** (chưa test thật trong dự án này):
- "Arm GNU Toolchain" — tải installer chính thức từ developer.arm.com,
  hoặc `winget install Arm.GnuArmEmbeddedToolchain`.
- `winget install Kitware.CMake` + `winget install Ninja-build.Ninja`
  (hoặc cài kèm qua Visual Studio Installer, mục "C++ CMake tools").
- `openocd` — tải binary release chính thức (openocd.org) hoặc
  `winget install OpenOCD.OpenOCD` (nếu có trong nguồn winget), thêm vào PATH tay.
- **Xem mục 6 (bash) — 2 target CMake cụ thể cần shell kiểu Unix, không
  chạy được bằng `cmd`/PowerShell thuần trên Windows.**

Verify:
```sh
arm-none-eabi-gcc --version
openocd --version
```

## 4. Toolchain RISC-V (`CH32V003F4P6`)

**macOS** (Homebrew, đã xác nhận thật — toolchain THẬT dùng trong SDK này
là `riscv64-elf-gcc`, KHÔNG PHẢI gói `riscv-none-elf-gcc` của xPack dù tên
file `cmake/toolchain/riscv-none-elf.cmake` gợi ý vậy, xem comment đầu file
đó):
```sh
brew install riscv64-elf-gcc riscv64-elf-gdb
```

**Linux/Windows** (CHƯA xác nhận thật trong dự án này — tự kiểm tra lại
trước khi tin dùng): dùng toolchain nào cũng được **miễn compile đúng**
`-march=rv32ec_zicsr -mabi=ilp32e` (RV32EC — chip CH32V003 thật KHÔNG có
extension M, dùng `rv32imac`/ABI `ilp32` thường thấy hơn sẽ khiến compiler
phát lệnh nhân/chia phần cứng không tồn tại trên silicon này, xem
`ARCHITECTURE.md` mục 10). xPack `riscv-none-elf-gcc` (đa nền tảng, cài qua
`npm install -g xpm` rồi `xpm install @xpack-dev-tools/riscv-none-elf-gcc`)
là lựa chọn hợp lý để thử — sau khi cài, kiểm tra multilib hỗ trợ đúng ABI
trên:
```sh
<compiler> -print-multi-lib | grep ilp32e
```
Không thấy dòng nào chứa `ilp32e` — vẫn build được (SDK tự cung cấp
`__udivsi3`/... qua `arch/RISCV/Core/Src/riscv_soft_math.c`, không cần
`-lgcc` khớp ABI, xem `ARCHITECTURE.md` mục 10) nhưng nên xác nhận lại
`-march=rv32ec_zicsr -mabi=ilp32e` thật sự được compiler chấp nhận (không
tự rơi về ISA/ABI khác) trước khi tin dùng cho build thật.

**`wlink`** (công cụ nạp firmware cho `CH32V003F4P6` qua WCH-LinkE — hiện
**chưa wire vào target CMake nào**, chỉ cần nếu tự thử nạp tay):
```sh
# Can Rust/cargo truoc (https://rustup.rs)
cargo install --git https://github.com/ch32-rs/wlink
```

## 5. VS Code (khuyến nghị, không bắt buộc)

1. Cài VS Code.
2. Cài 3 extension: `ms-vscode.cmake-tools`, `ms-vscode.cpptools`,
   `marus25.cortex-debug` (VS Code tự đề xuất khi mở 1 app trong `examples/`).
3. Build extension `neon-mdf-tools` từ source (chưa có bản `.vsix` phát
   hành sẵn):
   ```sh
   # Can Node.js (>=18 khuyen nghi) + npm
   cd vscode-extension
   npm install
   npm run typecheck && npm run package && npm run vsix
   ```
   Cài `.vsix` sinh ra qua **Extensions: Install from VSIX...** trong VS
   Code. Xem `VSCODE.md` mục "Lỗi thường gặp" nếu sửa code extension mà
   hành vi không đổi (phải build+cài lại `.vsix`, không tự rebuild).

## 6. Lưu ý riêng Windows — 2 target CMake cần `bash`

`serial_monitor` và `flash_uart_stm32` (`cmake/flash_targets.cmake`) chạy
qua `COMMAND bash -c "..."` — **không chạy được bằng `cmd.exe`/PowerShell
thuần trên Windows**. Cần 1 trong 2:

- **Git for Windows** (`git-scm.com`) — đã kèm sẵn Git Bash, `bash` tự có
  trong PATH sau khi cài, không cần thêm gì.
- **WSL** (Windows Subsystem for Linux) — nặng hơn nhưng cho môi trường
  Linux đầy đủ, hợp lý nếu đã dùng WSL cho việc khác.

Các target còn lại (`flash_jtag`, `flash_ota`, `menuconfig`, build thường)
không phụ thuộc `bash`, chạy bình thường trên Windows thuần.

## 7. Verify cài đặt xong — build thử

```sh
cd examples/Blinky
cmake --preset WEACT_F412
cmake --build --preset WEACT_F412
```

Thấy `Memory region .../%age Used` cho cả `bootloader`/`blinky`/`blinky_ota`
là cài đặt đã đủ để phát triển ứng dụng. Muốn phát triển driver/thêm target
mới, đọc tiếp `guide/AddNewMCU.md` + `guide/DeviceYAML.md` +
`guide/DriverImplement.md`.
