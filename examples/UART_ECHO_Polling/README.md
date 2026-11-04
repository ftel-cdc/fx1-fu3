# UART_ECHO_Polling — FU2 UART0 echo có log

Nhận một byte bằng polling, in log dữ liệu người dùng gửi, rồi gửi lại byte
đó và in log phía FPGA. Mặc định **UART0, 19200 baud, 8N1, FIFO bật**, clock
FPGA **10 MHz**. Không dùng interrupt, DMA hoặc FreeRTOS.

Gõ `A` trên terminal sẽ thấy:

```text
[USER -> FPGA] 'A' (0x41)
[FPGA -> USER] A (0x41)
```

Ký tự `A` sau nhãn FPGA được gửi bằng `neon_uart_transmit()`. Các nhãn/mã hex
được in bằng printf. Mỗi byte được xử lý ngay, không cần Enter hoặc buffer dòng.
Với byte không in được, dòng USER hiển thị `.` và mã hex; byte echo vẫn giữ
nguyên nên CR/LF hoặc ký tự điều khiển có thể làm thay đổi vị trí con trỏ.

## Build và cấu hình

```sh
cd /home/quang-hoang/merge_neon_mdf/CDC_NEON_MDF_merged/examples/UART_ECHO_Polling
cmake --preset FU2
cmake --build --preset FU2
```

Mở menuconfig:

```sh
cmake --build --preset FU2 --target menuconfig
```

Giữ `components/driver → Biên dịch UART driver` bật. Menu `UART_ECHO_Polling`
cho chỉnh baud, fixed FPGA clock và RX/TX timeout. Clock phải khớp board;
timeout tính bằng vòng poll, không phải mili-giây. Lưu bằng `S`, thoát bằng `Q`,
rồi chạy lại configure và build như trên.

## Nạp và chạy

Nạp PDI phần cứng phù hợp vào FPGA trước. UART0 TX/PC0 và RX/PC1 theo mapping
FU2 được route tới FTDI theo PDI; dùng đúng serial port đó. Nếu nối USB-UART
ngoài, nối TX ↔ RX và GND chung, xác nhận điện áp theo board map.

File boot là `build/FU2/uart_echo_polling_image.bin` (header ROM v5 128-byte +
payload). File `uart_echo_polling.bin` chỉ là payload raw. Xem [FU2.md](../../guide/FU2.md).

Trên máy nối FPGA, thay đường dẫn/cổng tương ứng rồi nạp:

```sh
python3 /home/quang-hoang/fu2_fpga/uart_boot_loader.py --port /dev/serial/by-id/usb-Xilinx_VEK280_262502175696-if02-port0 --image /home/quang-hoang/merge_neon_mdf/CDC_NEON_MDF_merged/examples/UART_ECHO_Polling/build/FU2/uart_echo_polling_image.bin --baud 9600 --byte-gap-ms 2 --no-monitor
```

Chạy loader trước rồi giữ SW5 khoảng 2 giây theo guideline. Baud BootROM 9600
ở lệnh trên độc lập với baud app 19200; đổi nếu PDI đang dùng có cấu hình khác.

Sau khi loader hoàn tất và đóng cổng:

```sh
python3 -m serial.tools.miniterm /dev/serial/by-id/usb-Xilinx_VEK280_262502175696-if02-port0 19200 --raw
```

Không thêm `--echo` để tắt local echo. Gõ từng ký tự hoặc chuỗi ngắn và quan
sát hai nhãn USER/FPGA. Thoát bằng `Ctrl+]`. Banner có thể đã đi qua trước khi
monitor mở; log hai chiều vẫn xuất hiện khi gõ.

Log làm tăng số byte TX, nên gõ chậm và tránh gửi burst dài gây đầy RX FIFO.
Luồng trả về có cả byte echo và log, không dùng để so sánh raw toàn bộ stream.

## Trạng thái

`g_uart_echo_stats` trong debugger giữ các bộ đếm byte RX/echo TX thành công,
RX/TX timeout, lỗi và mã lỗi gần nhất. Không tính byte log printf vào `tx_bytes`.
TX thành công nghĩa là byte đã được ghi vào THR; UART có thể vẫn đang phát
byte đó. Driver TX chỉ chờ LSR[5] THRF=0, không chờ LSR[6] TEMT.
RX timeout khi chưa có dữ liệu chỉ tăng bộ đếm rồi tiếp tục polling, không in
log liên tục. RX lỗi và TX lỗi được in với nhãn `[ERROR]`.

Chỉ include `<stdio.h>` để dùng printf; không gọi `neon_console_attach_uart()`.
Log khởi động đầu tiên tự khởi tạo console UART0 ở 19200 baud. App đợi
LSR[6] TEMT để banner truyền xong trước khi reset UART; đây là bước chuẩn bị
cho đổi cấu hình, không nằm trong đường truyền TX polling. Sau đó app
cấu hình lại UART0 theo menuconfig và bật FIFO; echo và log tiếp theo dùng
chung cấu hình phần cứng này. Nếu đổi baud app, log khởi động đầu tiên vẫn
ở 19200 baud, còn banner ready và log RX/TX ở baud app đã chọn.
Log là chẩn đoán best-effort; các bộ đếm TX theo dõi byte echo chính, không
theo dõi lỗi in từng phần của log. Không tự gửi lại dữ liệu sau TX timeout:
với một buffer nhiều byte, các byte trước đó có thể đã được ghi vào THR.

Buffer một byte sở hữu trọn word `uint32_t`, phù hợp SRAM word-only của FU2.
Build kiểm tra driver archive và ELF để từ chối truy cập byte/halfword. Chưa
kiểm chứng truyền nhận trên FPGA thật.
