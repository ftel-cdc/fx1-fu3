# GPIO_Input_Output — testcase 2 cho FU2

Đọc nút bấm tại **GPIOB_5 (PB5)** và điều khiển LED tại **GPIOB_1 (PB1)**:
**nhấn thì bật, nhả thì tắt**. Vòng lặp main đọc GPIO trực tiếp.
`printf` báo trạng thái qua UART0, 19200 baud, 8N1.

## Đấu nối và hoạt động

| Tín hiệu | Cấu hình trong main.c |
|---|---|
| PB5 | Input pull-up, nhấn = 0, nhả = 1 |
| PB1 | Output active-high: nhấn = 1, nhả = 0; khởi động ở 0 |
| UART0 | Log `printf`, giữ PC0/PC1 cho TX/RX |

Nút bấm nối PB5 xuống GND. Nếu FPGA/PDI không thực hiện pull-up của PAD,
dùng điện trở kéo lên ngoài theo đúng điện áp bank. PB1 có thể nối LED kèm
điện trở hoặc thiết bị đo; chân header phụ thuộc mapping của PDI đang nạp.
Chân và mức logic được viết trực tiếp trong `Src/main.c` để dễ đọc và sửa.

PB1 khởi động ở mức 0. Khi main đọc PB5 bằng 0, PB1 lên 1 để bật LED;
khi PB5 bằng 1, PB1 về 0 để tắt LED. Log chỉ in khi mức input thay đổi.
Ví dụ không dùng chống dội, nên nút cơ khí có thể tạo nhiều log lúc nhấn/nhả.

## Build và cấu hình

```sh
cd /home/quang-hoang/merge_neon_mdf/CDC_NEON_MDF_merged/examples/GPIO_Input_Output
cmake --preset FU2
cmake --build --preset FU2
```

Đổi clock bằng:

```sh
cmake --build --preset FU2 --target menuconfig
cmake --preset FU2
cmake --build --preset FU2
```

Cấu hình clock nằm trong menu `GPIO_Input_Output`. Clock FPGA mặc định 10 MHz;
cần khớp clock thật để baud UART chính xác. Giá trị clock 0 dùng clock startup
của SDK. Giữ driver UART bật để dùng `printf` với `<stdio.h>`; ứng dụng không
cần gọi `neon_console_attach_uart()` hay tự khởi tạo UART.
Đổi chân, pull hoặc mức active bằng cách sửa trực tiếp `Src/main.c`.

## Nạp và xem log

File nạp là **`build/FU2/gpio_input_output_image.bin`**, có header boot 128 byte.
`gpio_input_output.bin` chỉ là payload raw, không dùng trực tiếp với loader.

Trên máy nối FPGA, thay serial port và đường dẫn theo máy đó:

```sh
python3 /home/quang-hoang/fu2_fpga/uart_boot_loader.py --port /dev/serial/by-id/usb-Xilinx_VEK280_262502175696-if02-port0 --image /home/quang-hoang/merge_neon_mdf/CDC_NEON_MDF_merged/examples/GPIO_Input_Output/build/FU2/gpio_input_output_image.bin --baud 9600 --byte-gap-ms 2 --no-monitor
```

Chạy loader rồi reset theo quy trình board. Khi nạp xong, mở terminal:

```sh
python3 -m serial.tools.miniterm /dev/serial/by-id/usb-Xilinx_VEK280_262502175696-if02-port0 19200 --raw
```

Nhấn rồi nhả nút PB5. PB1 phải lên 1 khi nhấn và về 0
khi nhả, đồng thời xuất log:

```text
[INFO] PB5 button -> PB1 LED
[GPIO] RELEASED -> LED OFF
[GPIO] PRESSED -> LED ON
[GPIO] RELEASED -> LED OFF
```

Banner và trạng thái ban đầu có thể đã đi qua trước khi mở terminal;
nhấn/nhả nút để nhận log mới. Thoát miniterm bằng `Ctrl+]`.

## Ghi chú

Không dùng SysTick, ngắt GPIO, delay, DMA hay FreeRTOS. Main đọc nút và ghi
output bằng `neon_gpio_read_pin()` / `neon_gpio_write_pin()`. Trong lúc UART
in log, main tạm chờ; vòng lặp tiếp tục đọc nút sau khi printf trả về.

Giữ PC0/PC1 cho UART0 nếu sửa chân GPIO. Xác nhận trên FPGA thật bằng log
cùng trạng thái PB1/LED; kết quả build không xác nhận đường dây và pad.
