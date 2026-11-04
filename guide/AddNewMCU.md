# Thêm MCU target mới

Hướng dẫn từng bước để thêm 1 target mới vào neon-mdf — dù là chip hoàn toàn
mới (kiến trúc mới, vendor mới) hay 1 biến thể của family đã có sẵn
(`FU2`/`WEACT_F412`/`CH32V003`). Ví dụ cụ thể trong bài dùng lại số liệu thật
của `CH32V003F4P6` (target RISC-V đầu tiên của SDK, thêm sau khi đã có 2
target Cortex-M).

## 0. Trả lời trước 3 câu hỏi

1. **Kiến trúc CPU đã có backend chưa?** Hiện có `cortex-m` và `riscv`
   (`mcu-description/tools/backends/{cortex_m,riscv}/`). Kiến trúc khác (vd
   Xtensa) cần viết backend mới — ngoài phạm vi guide này.
2. **Family mới hay family đã có?** Family mới cần viết driver GPIO +
   `system_<TARGET>.c/.h` từ đầu. Family đã có (thêm 1 device cùng chip,
   biến thể package/flash size khác) — dùng lại toàn bộ driver, chỉ cần file
   `.yaml` + đăng ký mới.
3. **Có cần bootloader/OTA không?** Quyết định này ảnh hưởng có phải viết
   thêm Flash driver + UART driver + arch glue file hay không (mục 7-8).

**Bắt buộc có Reference Manual thật của chip** (không phải product brief cấp
cao) trước khi viết bất kỳ địa chỉ/bit nào. Không có TRM thật → viết code
được nhưng phải đánh dấu rõ ràng PLACEHOLDER trong YAML (xem
`devices/FU2.yaml` làm ví dụ) — không giả vờ số liệu là thật.

Tham chiếu đầy đủ từng field của file YAML (bước 1 dưới đây): `guide/DeviceYAML.md`.

## 1. Viết `mcu-description/devices/<TARGET>.yaml`

Theo `schema/common.schema.yaml` (field chung: `device`/`vendor`/`family`/
`arch`/`cpu`/`memory`/`peripherals`/`interrupts`, tuỳ chọn `bootloader`/
`erase_sectors`) + `schema/arch/<arch>.schema.yaml` (field riêng kiến trúc —
Cortex-M: `nvic_priority_bits`/`num_irq`/`vtor_supported`/`fpu_present`;
RISC-V: `isa`/`mtvec_mode`/`interrupt_controller`/`hart_count`, rơi vào
`cpu.extra` tự động, **không cần sửa `model.py`/`loader.py`**).

Tối thiểu cần:
- `memory:` — ít nhất 1 vùng `kind: flash` + 1 vùng `kind: ram`, địa chỉ/size
  thật.
- `peripherals:` — RCC/clock-enable + GPIO là bắt buộc (driver GPIO cần).
  Peripheral khác (UART/SPI/...) thêm khi thật sự cần — không cần khai hết
  ngay từ đầu, thêm dần khi viết driver mới.
- `interrupts:` — số IRQ thật, tối thiểu đủ để vector table không rỗng.

Ví dụ thật (`CH32V003F4P6.yaml`, rút gọn):

```yaml
device: CH32V003F4P6
vendor: WCH
family: CH32V003
arch: riscv-rv32ec
cpu:
  core: qingke-v2a
  max_freq_hz: 48000000
  isa: rv32ec
  mtvec_mode: vectored-address
  interrupt_controller: pfic
  hart_count: 1
memory:
  - {name: FLASH_MEM, kind: flash, address: 0x00000000, size: 0x4000}
  - {name: RAM,       kind: ram,   address: 0x20000000, size: 0x800}
peripherals:
  - name: RCC
    base: 0x40021000
    registers: [...]
  - name: GPIOA
    base: 0x40010800
    registers: &gpio_regs [...]
  - {name: GPIOC, base: 0x40011000, registers: *gpio_regs}
interrupts:
  - {number: 12, name: SysTick, description: "..."}
  - ...
```

**Bẫy thật đã gặp**: nếu chip có 1 peripheral CŨNG tên `FLASH` (flash
controller, dùng cho bootloader), **đặt tên vùng nhớ flash trong `memory:`
khác đi** (vd `FLASH_MEM`) — trùng tên với `peripherals:[].name: FLASH` sẽ
sinh 2 `#define FLASH_BASE` đè lên nhau trong header, lỗi khó nhận ra ngay.

Nếu có bootloader/OTA (mục 7): thêm `bootloader: {size, image_header_size,
uart: {peripheral, tx_pin, rx_pin, af, baud}}` và `erase_sectors:` (bảng
run-length kích thước sector erase thật — dùng để validate `flashmap.csv`
sau này, xem `WEACT_F412.yaml`/`CH32V003F4P6.yaml` làm ví dụ 2 kiểu khác
nhau: sector không đều/đều nhau).

## 2. Đăng ký trong `cmake/mcu_variants.cmake`

```cmake
set(NEON_MDF_TARGET_<TARGET>_ARCH   cortex-m4)      # hoặc riscv-rv32ec, ...
set(NEON_MDF_TARGET_<TARGET>_FAMILY <FAMILY>)
```

RISC-V cần thêm 2 biến (đường dẫn output sâu hơn 1 cấp:
`arch/RISCV/Device/<Vendor>/<Family>/`, Cortex-M chỉ có
`arch/CortexM/CMSIS/Device/<Family>/`):

```cmake
set(NEON_MDF_TARGET_<TARGET>_VENDOR <Vendor>)
set(NEON_MDF_TARGET_<TARGET>_ABI    ilp32e)          # khớp cpu.isa trong YAML
```

Thêm `<TARGET>` vào `NEON_MDF_KNOWN_TARGETS` ở đầu file.

## 3. Viết tay `system_<TARGET>.c/.h`

**Không sinh tự động** — chứa `SystemInit()`/`SystemCoreClockUpdate()` +
biến `SystemCoreClock`, cấu hình clock thật sau reset (hoặc để mặc định HSI
nếu chưa cần tối ưu tần số, miễn `SystemCoreClock` đúng giá trị thật đang
chạy — mọi thứ khác trong SDK, kể cả tick FreeRTOS, tính theo biến này chứ
không hardcode Hz).

Đặt tại `arch/{CortexM/CMSIS,RISCV/Device/<Vendor>}/Device/<Family>/Source/system_<TARGET>.c`
(+ header cùng thư mục `Include/`).

## 4. Verify codegen chạy sạch (chưa cần app)

```sh
cd neon-mdf/mcu-description/tools
python3 generate.py --device <TARGET>
```

Kiểm tra header sinh ra (`arch/*/Device/<Family>/Include/<TARGET>.h`) có
đúng địa chỉ/bit đã viết trong YAML — đọc lại bằng mắt trước khi build app,
lỗi ở bước này (vd offset sai) rẻ hơn nhiều so với phát hiện lúc chạy thật.

## 5. Thêm CMake preset

`cmake/presets/targets.json` — thêm 1 `configurePreset` (copy 1 target có
sẵn cùng kiến trúc, đổi `name`/`NEON_MDF_TARGET`/`CMAKE_TOOLCHAIN_FILE` nếu
khác toolchain). File này `include` chung bởi mọi app — không cần sửa
`CMakePresets.json` riêng của từng ví dụ.

## 6. Family mới: viết driver GPIO

Chi tiết đầy đủ (API contract, cách wire CMake, thêm loại peripheral mới):
`guide/DriverImplement.md`. Tóm tắt — bắt buộc mọi family phải có (kể cả
không dùng bootloader). Tạo
`components/driver/gpio/neon_gpio_<family>.c`, implement đúng API trong
`components/driver/gpio/neon_gpio.h` (`neon_gpio_clock_enable/config/WritePin/ReadPin/TogglePin`) —
dùng struct thanh ghi sinh ra từ bước 1, không tự bịa offset.

Thêm nhánh dispatch trong `components/driver/CMakeLists.txt`:

```cmake
elseif(NEON_MDF_TARGET_${NEON_MDF_TARGET}_FAMILY STREQUAL "<FAMILY>")
  set(_neon_mdf_driver_sources gpio/neon_gpio_<family>.c)
```

## 7. Build thử app tối thiểu

Copy `tools/templates/` (template bare-metal có sẵn) vào 1 thư mục tạm, đổi
target, build:

```sh
cmake --preset <TARGET>
cmake --build --preset <TARGET>
```

Nếu target chưa có `bootloader:` trong YAML, app build thẳng — dừng ở đây là
đủ cho 1 family mới không cần bootloader/OTA. **Family mới BẮT BUỘC cũng
phải thêm vào whitelist FAMILY trong `neon-mdf/CMakeLists.txt`** (dòng
`if(... FAMILY STREQUAL "FU2" OR ... STREQUAL "WEACT_F412" OR ...)`) — thiếu
bước này, `components/driver` không được `add_subdirectory()`, lỗi build sẽ
trông như "thiếu header" chứ không nói rõ nguyên nhân thật.

## 8. (Tuỳ chọn) Bootloader/OTA

Cần thêm 3 file mới cho family mới (nếu family đã có sẵn, bỏ qua mục này):

1. **`components/driver/flash/neon_flash_<family>.c`** — implement
   `neon_flash_erase_region`/`neon_flash_program_word` (API trong
   `neon_flash.h`) theo flash controller thật (đơn vị erase/program mỗi chip
   khác nhau — vd STM32F4 ghi được 32-bit/lần, CH32V003 chỉ ghi được
   16-bit/lần dù API ngoài vẫn nhận 1 word 32-bit). Thêm nhánh dispatch
   trong `components/driver/CMakeLists.txt` (khối `if(NEON_MDF_BOOTLOADER_ENABLED)`).
2. **`components/bootloader/neon_bootloader_uart_<family>.c`** — implement
   `neon_bootloader_uart.h` (`config/SendByte/RecvByte/TryRecvByte`) theo
   UART thật đã khai ở `bootloader.uart` trong YAML.
3. **`components/bootloader/neon_bootloader_arch_<family>.c`** — implement
   `neon_bootloader_arch.h` (`SystickStart1ms/Systick1msElapsed/JumpToApp/SystemReset`)
   — đây là phần "nhảy từ bootloader sang app" thật, sai ở đây có thể khiến
   board không boot được app (lỗi nghiêm trọng nhất trong SDK). Với kiến
   trúc mới (không phải Cortex-M/RISC-V-PFIC đã có), đọc kỹ cách CPU đó xử
   lý vector table/reset trước khi viết `JumpToApp`.

Cả 3 file dispatch theo FAMILY trong `components/bootloader/CMakeLists.txt`.

Verify theo đúng thứ tự (đừng nhảy cóc):
1. `cmake --build` ra cả `bootloader` + app — link sạch.
2. Nạp `bootloader` qua debug probe (JTAG/SWD tuỳ kiến trúc), xác nhận chạy
   tới điểm chờ SYNC (đo bằng breakpoint hoặc LED/UART log tạm).
3. Nạp app qua giao thức UART thật (không phải qua debug probe) — xác nhận
   `neon_bootloader_arch_jump_to_app()` nhảy đúng, app chạy được.
4. Nếu muốn OTA thật: thêm `flashmap.csv`/`flashmap.<TARGET>.csv` cho app,
   verify factory boot → OTA switch → auto-rollback (ngắt nguồn giữa chừng,
   xác nhận không rơi vào trạng thái kẹt).

**Kiểm tra ngân sách RAM/flash trước khi cam kết OTA** — nếu chip nhỏ (vài
chục KB flash trở xuống), 2 bản ảnh (factory+OTA) + bootloader có thể không
đủ chỗ, đặc biệt nếu app dùng FreeRTOS (kernel tối thiểu đã tốn vài KB). Nếu
không đủ: **không tạo `flashmap.<TARGET>.csv`** cho app đó — bootloader tự
rơi về mô hình 1-slot-cố-định (không OTA, vẫn verify header/CRC32 thật), không
cần sửa code C nào (xem `ARCHITECTURE.md` mục 7-8).

## 9. (Tuỳ chọn) FreeRTOS

Nếu kiến trúc CPU đã có port FreeRTOS tương ứng nhưng chưa wire vào
`cmake/freertos.cmake`, thêm 1 nhánh chọn `FREERTOS_PORT` theo `arch` (xem
`ARCHITECTURE.md` mục 9 cho bảng port hiện có). Nếu core không có timer
tương thích CLINT (giống trường hợp CH32V003/PFIC), app cần tự cấu hình tick
qua `vPortSetupTimerInterrupt()`/hàm tương đương (weak trong port, app
override) — xem `tools/templates_freertos/Src/main.c` làm ví dụ đầy đủ.

## 10. Checklist trước khi coi target "xong"

- [ ] `generate.py --device <TARGET>` chạy sạch, header/linker/startup đúng
      địa chỉ đã tra trong Reference Manual.
- [ ] Build sạch cho ít nhất 1 app tối thiểu (`tools/templates/`).
- [ ] Regression-check các target CŨ vẫn build sạch, size không đổi bất
      ngờ (mọi thay đổi ở bước 2/6/7 đều là file **dùng chung**, dễ vô tình
      phá target khác nếu whitelist/dispatch thiếu `else()`/`elif()` đúng
      family).
- [ ] Nếu có bootloader: đã nạp + chạy thật trên board (không chỉ build
      sạch) — đây là component rủi ro cao nhất, đừng coi "link được" là
      "đúng".
- [ ] Cập nhật `README.md` (bảng target) — SDK này giữ docs mô tả đúng hiện
      trạng, không phải nhật ký thay đổi (xem `ARCHITECTURE.md` đầu file).

## Tham khảo nhanh — toàn bộ file có thể cần sửa/tạo

| File | Bắt buộc? | Ghi chú |
|---|---|---|
| `mcu-description/devices/<TARGET>.yaml` | Luôn | Nguồn sự thật |
| `cmake/mcu_variants.cmake` | Luôn | Đăng ký ARCH/FAMILY(/VENDOR/ABI) |
| `arch/*/Device/<Family>/Source/system_<TARGET>.c/.h` | Luôn | Viết tay |
| `cmake/presets/targets.json` | Luôn | 1 preset mới |
| `neon-mdf/CMakeLists.txt` (whitelist FAMILY) | Family mới | Dễ quên nhất |
| `components/driver/gpio/neon_gpio_<family>.c` + dispatch | Family mới | |
| `components/driver/flash/neon_flash_<family>.c` + dispatch | Bootloader | |
| `components/bootloader/neon_bootloader_uart_<family>.c` + dispatch | Bootloader | |
| `components/bootloader/neon_bootloader_arch_<family>.c` + dispatch | Bootloader | Rủi ro cao nhất |
| `cmake/freertos.cmake` (nhánh port mới) | FreeRTOS trên arch mới | |
| `<app>/flashmap.<TARGET>.csv` | OTA | Per-app, không phải per-SDK |
