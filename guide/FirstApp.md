# Tạo app đầu tiên

Tutorial từ đầu — tạo, build, nạp, và sửa 1 app mới. Đã cài đặt xong môi
trường chưa? Xem `SETUP.md` trước.

## 1. Tạo app mới

**Qua VS Code** (khuyến nghị): mở nguyên thư mục `neon_mdf/` (thư mục gốc)
hoặc 1 app có sẵn trong VS Code, `Cmd/Ctrl+Shift+P` → **NEON-MDF: Tạo App
Moi** → nhập tên (chỉ chữ/số/underscore, không bắt đầu bằng số, vd
`my_app`) → chọn **App thường (bare-metal)** hoặc **App dùng FreeRTOS** →
chọn tạo trong `examples/` (mặc định). Xong sẽ hỏi mở thư mục app mới
trong cửa sổ riêng — chọn có.

**Qua dòng lệnh** (không cần VS Code): copy tay 1 trong 2 thư mục nguồn,
đổi 2 token trong nội dung file:

```sh
cp -r neon-mdf/tools/templates examples/my_app   # hoặc tools/templates_freertos
cd examples/my_app
# thay __APP_NAME__ -> my_app, __NEON_MDF_RELATIVE_PATH__ -> ../../neon-mdf
# trong CMakeLists.txt, CMakePresets.json, Src/main.c, .vscode/settings.json
```

(Wizard VS Code tự làm bước thay token này — copy tay cần tự thay, dùng
`sed` hoặc tay đều được, chỉ cần đúng 2 token trên.)

## 2. Cấu trúc app vừa tạo

```
my_app/
  CMakeLists.txt      # project(my_app) + neon_mdf_add_executable()/neon_mdf_add_app_executable()
  CMakePresets.json    # include cmake/presets/targets.json dùng chung - tự có preset cho MỌI target
  Src/main.c            # điểm bắt đầu - int main(void) toi giản, KHÔNG GPIO/LED demo
  Kconfig               # placeholder rỗng (comment sẵn cấu trúc mẫu) - cả 2 template đều có, xem guide/MenuConfig.md nếu muốn thêm option riêng
  .vscode/              # settings.json (cmake.environment trỏ NEON_MDF_PATH) + extensions.json
```

`Src/main.c` cung cấp vòng lặp main hoặc task làm điểm bắt đầu để sửa/thêm
logic ứng dụng. `WEACT_F412` hiện không có syscall retarget `printf()` sang
UART, nên các lệnh `printf()` trong template không tạo log UART trên target này.

## 3. Build

```sh
cd examples/my_app
cmake --preset WEACT_F412
cmake --build --preset WEACT_F412
```

Đổi `WEACT_F412` thành `FU2`/`CH32V003F4P6` để build target khác — không
cần sửa gì trong `CMakeLists.txt`/`Src/main.c` (trừ khi code có `#ifdef`
theo tên target, xem `examples/Blinky/Src/main.c` cho ví dụ 1 file chạy
được cả 3 target).

Nếu target có `bootloader:` trong YAML (`WEACT_F412`/`CH32V003F4P6`), build
ra thêm executable `bootloader` — app template mặc định KHÔNG có OTA (build
1 bản duy nhất, `neon_mdf_add_app_executable()`) — muốn thêm OTA thật, xem
`guide/BootloaderOTA.md`.

## 4. Nạp + xem log (board thật, `WEACT_F412`)

Qua VS Code: chọn target `WEACT_F412` ở status bar → **NEON-MDF: Build +
Flash + Log**.

Qua dòng lệnh:
```sh
cmake --build --preset WEACT_F412 --target flash_jtag
cmake --build --preset WEACT_F412 --target serial_monitor   # Ctrl+] de thoat
```

Thấy dòng in ra `"my_app dang chay"` lặp lại — app đã chạy thật trên board.

## 5. Sửa code, build lại

Sửa `Src/main.c` (vd thêm GPIO toggle LED — copy logic từ
`examples/Blinky/Src/main.c` làm mẫu: `neon_gpio_clock_enable()` PHẢI gọi
trước `neon_gpio_init()`, xem `guide/DriverImplement.md` nếu muốn hiểu API
driver). Build lại + flash lại — không cần `cmake --preset` lại trừ khi
sửa `CMakeLists.txt`/`flashmap.csv`/`Kconfig`.

## 6. Bước tiếp theo

- Muốn bật/tắt tính năng qua `menuconfig` (option riêng cho app này):
  `guide/MenuConfig.md`.
- Muốn thêm OTA thật: `guide/BootloaderOTA.md`.
- Muốn debug qua VS Code (breakpoint, step): `guide/Debugging.md`.
- Muốn hiểu vì sao SDK thiết kế thế này: `ARCHITECTURE.md`.
