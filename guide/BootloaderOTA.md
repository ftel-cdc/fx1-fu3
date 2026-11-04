# Thêm bootloader/OTA vào app

Hướng dẫn thực hành thêm OTA thật vào 1 app — viết `flashmap.csv`, sửa
`CMakeLists.txt`, gọi API trong code, và cách test thật chu trình
factory→OTA→rollback. Xem `ARCHITECTURE.md` mục 7-8 cho lý do thiết kế,
`guide/DeviceYAML.md` mục `bootloader:`/`erase_sectors:` cho field YAML.

**File này là component rủi ro cao nhất trong SDK** (từng brick board thật
2 lần trong lịch sử dự án) — làm đúng thứ tự bước, verify từng bước bằng
board thật trước khi tin dùng, đừng chỉ tin build sạch.

## 0. Khi nào NÊN dùng OTA — kiểm tra ngân sách trước

OTA cần **2 bản ảnh đầy đủ** (factory + OTA) cùng lúc trong flash, cộng
`otadata`. Trước khi viết `flashmap.csv`, ước lượng:

```
bootloader.size + 2 × (image_header_size + kích_thước_app_thật) + otadata_size ≤ flash thật
```

**Bài học thật từ `CH32V003F4P6`**: FreeRTOS-Kernel tối thiểu đã chiếm
~8.5KB flash (đo thật, xem comment CMakeLists.txt) — với chip 16KB thì 2 bản không đủ chỗ cho riêng app
`FreeRTOS_Blinky` (không có `flashmap.CH32V003F4P6.csv`). Đây là quyết định
**theo từng app**, không phải giới hạn của cả target — `examples/Blinky`
(bare-metal, không FreeRTOS) trên **cùng chip** này vẫn có OTA thật (đủ 12KB
sau bootloader). SDK **không** ép dùng flashmap cho mọi app cùng target —
bootloader vẫn thật (verify header/CRC32, nhận firmware qua UART) dù app có
flashmap hay không, chỉ khác 1 slot cố định (không A/B) hay 2 slot. Nếu ngân
sách không đủ, đừng tạo `flashmap.<TARGET>.csv` cho app đó (xem mục 2) —
không phải lỗi, là lựa chọn kiến trúc hợp lệ.

## 1. Điều kiện tiên quyết

Device đang build phải có `bootloader:` trong YAML (`WEACT_F412`/
`STM32F103CB`/`CH32V003F4P6` đã có, `FU2` thì chưa — build sẽ `FATAL_ERROR` rõ ràng nếu
gọi nhầm helper OTA cho target không có bootloader).

## 2. Viết `flashmap.csv`

Đặt cạnh `CMakeLists.txt` của app (hoặc `flashmap.<TARGET>.csv` nếu app
build cho nhiều device khác flash size — `neon-mdf/CMakeLists.txt` tự ưu
tiên file có hậu tố đúng `NEON_MDF_TARGET` trước, fallback về `flashmap.csv`
chung nếu không có):

```
# name,     type, subtype, size
app,        app,  factory, 0x8000     # slot factory - KHONG BAO GIO bi OTA dung
otadata,    data, ota,     0x10000    # trang thai OTA - can nguyen 1 sector
app_ota,    app,  ota,     0x60000    # slot OTA - CHI OTA ghi vao
```

Bắt buộc đúng 1 partition **tên** `app` (`type=app,subtype=factory`) — thứ
tự các dòng không quan trọng (xem `guide/DeviceYAML.md`). Kích thước mỗi
`type=app` partition đã bao gồm cả "khe" `image_header_size` (xem
`bootloader.image_header_size` trong device YAML) — không phải chỉ code.

## 3. Sửa `CMakeLists.txt` — build 2 (hoặc N) bản từ CÙNG 1 `main.c`

Mẫu tối thiểu (2 bản, giống `examples/Blinky/CMakeLists.txt`):

```cmake
neon_mdf_add_bootloader_executable(bootloader)

function(_myapp_add_variant name linker_script version)
  add_executable(${name} Src/main.c)
  target_link_libraries(${name} PRIVATE neon_mdf_driver neon_mdf_ota)
  target_compile_definitions(${name} PRIVATE NEON_OTA_APP_VERSION=${version})
  target_link_options(${name} PRIVATE -T${linker_script} -Wl,-Map=$<TARGET_FILE:${name}>.map)
  set_target_properties(${name} PROPERTIES LINK_DEPENDS ${linker_script})
  add_custom_command(TARGET ${name} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${name}> $<TARGET_FILE:${name}>.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${name}>
  )
endfunction()

set(_myapp_ld_dir ${CMAKE_BINARY_DIR}/flashmap)
_myapp_add_variant(myapp     ${_myapp_ld_dir}/app.ld     1)
_myapp_add_variant(myapp_ota ${_myapp_ld_dir}/app_ota.ld 2)

include(${NEON_MDF_PATH}/cmake/flash_targets.cmake)
neon_mdf_add_flash_targets(bootloader myapp ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
neon_mdf_add_ota_flash_target(myapp_ota ${NEON_MDF_PATH}/cmake/openocd/${NEON_MDF_TARGET}_openocd.cfg)
```

Mẫu trên dùng `add_executable()` trực tiếp để chọn linker script của từng
partition. SDK không tự retarget `printf()` sang UART trên `WEACT_F412`;
log UART qua `printf()` hiện không có trên target này. Xem
`examples/OTA_Demo/CMakeLists.txt` cho cách tạo đầy đủ các biến thể OTA.

`${_myapp_ld_dir}/app.ld`/`app_ota.ld` sinh tự động từ `flashmap.csv` lúc
configure — không viết tay.

## 4. Gọi API trong `Src/main.c`

```c
#include "neon_ota.h"

int main(void)
{
  /* ... khoi tao GPIO/peripheral ... */

  uint32_t loop_count = 0;
  for (;;) {
    /* ... logic chinh ... */
    loop_count++;
    if (loop_count == 10u) {   /* sau khi chay on dinh 1 luc */
      neon_ota_confirm_valid();
    }
  }
}
```

`neon_ota_confirm_valid()` (`components/ota/neon_ota.h`) **PHẢI được app tự
gọi** sau khi đã chạy ổn định (không phải bootloader tự làm) — bootloader
không có cách nào biết app "chạy tốt" ngoài việc chờ chính app xác nhận.
Gọi quá sớm (chưa kịp phát hiện lỗi thật) làm mất tác dụng an toàn của cơ
chế rollback; gọi quá muộn/không gọi → tự động rollback về factory sau
`NEON_OTA_MAX_BOOT_ATTEMPTS` (mặc định 3) lần reset chưa xác nhận.

`NEON_OTA_APP_VERSION` (macro `target_compile_definitions` ở mục 3) chỉ để
code tự phân biệt bản nào đang chạy (vd đổi chu kỳ LED/in log) — không ảnh
hưởng logic OTA của bootloader.

## 5. Flash lần đầu — nạp factory

```sh
cmake --preset WEACT_F412
cmake --build --preset WEACT_F412
cmake --build --preset WEACT_F412 --target flash_jtag
```

`flash_jtag` ghi bootloader + slot `app` (factory) + reset `otadata` về
`active_slot=0,state=VALID` trong CÙNG 1 phiên SWD — sau lệnh này board
LUÔN chạy đúng bản factory vừa nạp, bất kể trạng thái OTA cũ trước đó (xem
`ARCHITECTURE.md` mục 7, chính sách này từng là nguồn bug thật trước khi
được thêm).

## 6. Test OTA thật (giả lập qua SWD — chưa có đường mạng thật)

```sh
cmake --build --preset WEACT_F412 --target myapp_ota   # build bản OTA (version 2)
cmake --build --preset WEACT_F412 --target flash_ota
```

`flash_ota` ghi `[header+myapp_ota]` vào slot `app_ota` + otadata
`active_slot=1,state=PENDING_VERIFY` — bootloader lần boot tới sẽ nhảy vào
slot OTA. Verify:

```sh
# UART (neu app co printf/log)
python3 -m serial.tools.miniterm /dev/cu.usbmodemXXXX 115200

# Hoac doc otadata truc tiep qua SWD (chinh xac hon, khong phu thuoc timing UART)
openocd -f cmake/openocd/WEACT_F412_openocd.cfg -c init -c "reset halt" \
  -c "mdw <NEON_MDF_FLASHMAP_OTADATA_BASE thuc>" -c shutdown
```

`otadata` là 5 word 32-bit liên tiếp: `magic`(=`0x4154414F`)/`active_slot`/
`state`(0=VALID/1=PENDING_VERIFY/2=INVALID)/`boot_attempts`/`crc32`. Sau khi
app tự `ConfirmValid()`, `state` phải chuyển về `0` (VALID) — đây là bằng
chứng chắc chắn nhất, không phụ thuộc đọc đúng lúc UART in ra.

## 7. Test auto-rollback thật

Build 1 bản CỐ Ý không bao giờ confirm (xem `examples/OTA_Demo`'s
`NEON_OTA_SKIP_CONFIRM` làm mẫu — 1 `target_compile_definitions` bổ sung,
`#if !defined(NEON_OTA_SKIP_CONFIRM))` bọc quanh lời gọi `ConfirmValid()`).
Ví dụ có sẵn target `flash_ota_bad` để ghi bản đó. Sau khi nạp, tự reset board
đến lần boot chưa xác nhận thứ `NEON_OTA_MAX_BOOT_ATTEMPTS + 1` (mặc định:
lần thứ 4) — lần cuối phải thấy board quay lại chạy bản **factory**, và đọc
`otadata` qua SWD thấy `active_slot=0`.

## 8. Lỗi thường gặp

| Triệu chứng | Nguyên nhân |
|---|---|
| Configure lỗi `FATAL_ERROR ... chua khai bootloader:` | Target đang build không có `bootloader:` trong YAML (vd `FU2`) — đổi target. |
| `flash_ota` báo thiếu `NEON_MDF_FLASHMAP_APP_OTA_BASE`/`OTADATA_BASE` | App chưa có `flashmap.csv` hợp lệ, hoặc gọi `neon_mdf_add_ota_flash_target()` trước khi `flashmap.csv` được xử lý. |
| Linker lỗi region FLASH overflow ở bản OTA | Ngân sách flash không đủ cho 2 bản (xem mục 0) — giảm code, hoặc bỏ OTA cho device đó (mục 0). |
| Board không rollback dù đã reset nhiều lần | Kiểm tra lại thật đã build bản `NEON_OTA_SKIP_CONFIRM` (không phải bản thường) trước khi `flash_ota`. |
| `flash_jtag` xong mà board vẫn chạy bản OTA cũ | Đang dùng `flash_jtag` cũ chưa có bước reset `otadata` — cập nhật SDK lên bản có chính sách ở mục 5 (`ARCHITECTURE.md` mục 7). |
