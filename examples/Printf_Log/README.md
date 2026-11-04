# Printf_Log — log UART trên FU2

Example bare-metal dùng `printf` trực tiếp, qua bộ formatter word-only built-in
của SDK và `_write`; app chỉ include `<stdio.h>`, không cần gọi UART config/Attach.
Chỉ cần `CONFIG_NEON_MDF_DRIVER_UART=y`; không có cờ printf riêng. Lần printf
đầu tiên tự khởi tạo UART0.
UART0 mặc định **19200 baud, 8N1**. Clock FPGA cố định **10 MHz**, cùng chính sách
với Test_VP. `NEON_PRINTF_FIXED_CLOCK_HZ` áp dụng cho cả startup/device và app.
`sdkconfig` đã bật UART. Nếu tắt UART, cả backend UART và console đều không
được biên dịch; app gọi printf sẽ báo lỗi link.

## Build

```sh
cd examples/Printf_Log
cmake --preset FU2
cmake --build --preset FU2
```

Output:

- `build/FU2/printf_log`: ELF cho debugger/emulator.
- `build/FU2/printf_log.bin`: payload raw.
- `build/FU2/printf_log_image.bin`: header ROM v5 128-byte + payload; nạp tại
  `0x00020000` qua công cụ FU2 hiện có.

Không dùng payload raw để boot qua ROM. Hợp đồng nạp và XIP: [FU2.md](../../guide/FU2.md).
Example không bổ sung công cụ flash FU2 hoặc bootloader riêng.

Board có clock khác phải configure lại, ví dụ:

```sh
# Board cung cấp clock cố định 20 MHz:
cmake --preset FU2 -DNEON_PRINTF_FIXED_CLOCK_HZ=20000000
# Hoặc dùng chính sách SystemInit bình thường của SDK:
cmake --preset FU2 -DNEON_PRINTF_FIXED_CLOCK_HZ=0
```

Chọn đúng một cấu hình clock cho board, rồi build lại. Console mặc định 19200
baud; muốn đổi UART/baud có thể dùng API AttachUART tùy chọn trong hướng dẫn.

## Xem log

UART0 TX → USB-UART RX, GND chung; xác nhận route và điện áp theo board map.
Mở Serial Monitor của extension ở 19200 baud hoặc:

```sh
python3 -m serial.tools.miniterm /dev/ttyUSB0 19200
```

Reset board sau khi mở monitor. Output mong đợi với cấu hình mặc định:

```text
[INFO] NEON MDF printf ready
[INFO] printf via _write
[DEBUG] signed=-42 hex=0x1234ABCD text=hello
[INFO] counter=0
[INFO] counter=1
...
```

Counter dùng busy-loop, không phải chu kỳ thời gian chính xác. Ví dụ dừng tại
`halt()` nếu printf thất bại (kể cả UART init tự động lỗi); có thể xem status trong
debugger. Prefix INFO/DEBUG là chuỗi do app viết, chưa có hệ thống lọc log level.

API, format hỗ trợ và cách sử dụng trong app khác:
[PrintfLogging.md](../../guide/PrintfLogging.md).
