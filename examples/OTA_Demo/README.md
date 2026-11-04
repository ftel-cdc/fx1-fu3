# OTA_Demo trên STM32F103CB Blue Pill

## Kết nối

- ST-Link: `SWDIO`, `SWCLK`, `GND`, `3V3`.
- USB-UART để xem log: `PA9 (TX) -> RX`, `PA10 (RX) -> TX`, GND chung,
  `115200 8N1`.
- LED onboard PC13 thể hiện firmware đang chạy; log UART cho biết chính xác
  version 1/2/3.

## Build và nạp factory v1

Chạy từ thư mục `examples/OTA_Demo`:

```sh
cmake --preset STM32F103CB
cmake --build --preset STM32F103CB
cmake --build --preset STM32F103CB --target flash_jtag
```

`flash_jtag` ghi bootloader, factory v1 và đặt `otadata` về factory. Sau reset,
bootloader chờ UART 3 giây rồi LED PC13 nháy và USART1 in `OTA_Demo v1`.

## Nạp OTA v2 hợp lệ

```sh
cmake --build --preset STM32F103CB --target flash_ota
```

Target chỉ ghi slot `app_ota` và `otadata`, sau đó reset. V2 gọi
`neon_ota_confirm_valid()` sau 10 vòng lặp; những lần reset sau tiếp tục chạy v2.

## Kiểm tra rollback bằng v3

```sh
cmake --build --preset STM32F103CB --target flash_ota_bad
```

V3 cố ý không confirm. Lần boot ngay sau flash là attempt 1; reset thêm ba lần
để attempt kế tiếp vượt ngưỡng 3. Bootloader sẽ chọn lại factory và log trở về
`OTA_Demo v1`.

Mở log trên Linux, ví dụ:

```sh
python3 -m serial.tools.miniterm /dev/ttyUSB0 115200
```

`flash_ota`/`flash_ota_bad` là cách giả lập OTA qua SWD. Giao thức cập nhật UART
`0x55` dùng hai target sau (cổng mặc định `/dev/ttyACM0`):

```sh
# OTA v2 hop le
cmake --build --preset STM32F103CB --target flash_ota_uart

# OTA v3 khong confirm, dung test rollback
cmake --build --preset STM32F103CB --target flash_ota_uart_bad
```

Sau khi client in `Dang cho bootloader`, reset board để byte SYNC `0x55` rơi
vào cửa sổ 3 giây của bootloader. Phải đóng Serial Monitor trước khi chạy.
Cần bấm nút `RESET/NRST`, không rút nguồn hay rút cáp USB. `/dev/ttyACM0` phải
là USB-UART/ST-Link VCP nối với `PA9/PA10`; cổng USB trực tiếp của Blue Pill
không mang giao thức USART1 này.
Có thể đổi cổng mà không configure lại:

```sh
NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 \
  cmake --build --preset STM32F103CB --target flash_ota_uart
```
