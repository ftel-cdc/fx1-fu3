# Nạp firmware FU2 qua UART

FU2 Flash Tool **1.0** dùng dịch vụ USART update của ROMCODE để ghi image vào
Internal Flash. Tool chạy trên máy tính; không cần OpenOCD cho đường nạp này.

Quy trình cho đầy đủ các đường boot nằm trong
[Chế độ khởi động và quy trình chạy firmware FU2](FU2BootModes.md).

## Chuẩn bị

**Linux/macOS:** phần nạp UART và monitor FU2 dùng `termios`, `select` và
`os` trong thư viện chuẩn Python, tương tự tool `uart_boot_loader.py` ban đầu.
Không cần cài `pyserial` hoặc `python3-serial` để chạy tool FU2 khi đã chỉ rõ
port. Có thể chuyển thẳng sang bước kết nối board và chạy lệnh nạp.

**Windows:** tool dùng `pyserial` để truy cập cổng COM. Cài dependency từ thư
mục gốc repository:

```sh
python3 -m pip install -r neon-mdf/tools/requirements.txt
```

Cài bằng đúng Python mà CMake dùng để chạy tool; kiểm tra trên Windows bằng
interpreter tương ứng, chẳng hạn:

```sh
python3 -c "import serial; print(serial.__version__)"
```

`pyserial` vẫn được dùng cho các tool UART STM32 và chức năng tự dò cổng của
CLI SDK. Khi dùng FU2 trên Linux/macOS, truyền cổng bằng `--port`, `-p` hoặc
`NEON_MDF_SERIAL_PORT` như các ví dụ bên dưới để không cần thư viện này.

Kết nối UART nạp của board: TX của bộ chuyển đổi nối RX của board, RX nối TX
và nối chung GND. Dùng mức điện áp phù hợp với board. Chọn boot mode **100 —
USART update** theo sơ đồ chân/jumper của board. Đóng chương trình Serial
Monitor khác đang giữ cổng trước khi nạp.

Board cần có NVR policy hợp lệ, tương thích với image Secure/Non-secure.
Tool này không thực hiện provisioning NVR. Chữ ký và chính sách bảo mật được
ROMCODE kiểm tra trên thiết bị; kiểm tra CRC trên PC không thay thế bước đó.

## Nạp bằng CMake

Chạy trong thư mục ứng dụng, ví dụ `examples/Printf_Log/`:

```sh
cmake --preset FU2
```

Truyền port và baud ở lần chạy target nạp:

```sh
cmake -E env \
  NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 \
  NEON_MDF_SERIAL_BAUD=19200 \
  NEON_MDF_UART_BYTE_GAP_MS=2 \
  NEON_MDF_UART_LOG=full \
  cmake --build --preset FU2 --target flash_uart_fu2
```

Với PowerShell hoặc CMD, viết cùng lệnh trên một dòng và thay cổng bằng
`COM3`, chẳng hạn. `cmake -E env` truyền biến môi trường cho tiến trình build
và tool, không lưu port/baud vào CMake cache. Đổi các giá trị này không cần
configure lại. `cmake --build` không nhận trực tiếp tùy chọn `--port`/`--baud`.

Target tự build ứng dụng và lấy đúng `<app>_image.bin`. Không chọn file `.elf`
hoặc `.bin` chỉ chứa payload. Target `flash_uart_fu2` trỏ tới ứng dụng đầu tiên
được đóng gói; nếu project có nhiều image, chọn `<app>_flash_uart_fu2` cụ thể.
Project dùng `neon_mdf_add_executable()` hoặc `neon_mdf_add_fu2_image()` được
đăng ký các target này tự động.

Tool hiển thị banner gồm version, boot mode yêu cầu, image profile, version
ứng dụng, địa chỉ image, port, baud và phương thức reset. Khi xuất hiện:

```text
[WAIT] Select boot mode 100 - USART update, then press RESET on the FU2 board.
[WAIT] UART is ready. Press Ctrl+C to cancel.
```

Nhấn **RESET trên board FU2**. Lúc này cổng UART đã mở và tool sẵn sàng trả lời
ROMCODE. Mặc định tool chờ lần Reset đầu tiên đến khi nhận được yêu cầu hoặc
người dùng nhấn Ctrl+C. Sau khi truyền bắt đầu, mỗi yêu cầu có timeout 300 giây,
bao gồm khoảng ROMCODE tạm ngừng giao tiếp để chuẩn bị/xóa Flash.

Khi hoàn tất:

```text
[PASS] Image transfer completed in ... seconds; total runtime ... seconds.
[INFO] ROMCODE verified and committed the image; application handoff follows.
```

Tool chỉ báo thành công khi ROMCODE đã yêu cầu đủ image và trả về
`0xFFFFFFFF`. ROMCODE sau đó chuyển sang ứng dụng; không cần reset lần nữa
để bắt đầu phiên chạy này. Chọn lại boot mode **000 — Internal Flash** trước
những lần reset/cấp nguồn tiếp theo để chạy firmware đã cài.

Baud nạp phải khớp với ROMCODE thực tế trên board. `19200` là ví dụ; tham số
chỉ cấu hình UART phía PC, không thay đổi baud của ROMCODE. Khung truyền là
**8 data bits, no parity, 1 stop bit (8N1)**, không dùng flow control.

## Theo dõi ứng dụng

Baud của ứng dụng độc lập với baud nạp:

```sh
cmake -E env \
  NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 \
  NEON_MDF_MONITOR_BAUD=19200 \
  cmake --build --preset FU2 --target serial_monitor
```

Mặc định monitor FU2 dùng 19200 baud; nhấn Ctrl+C để thoát. Log `[WAIT]`,
`[TX]`, `[PASS]` thuộc tool PC. Nội dung sau khi ứng dụng chạy phụ thuộc vào
chính ứng dụng; không có một dòng log khởi động bắt buộc cho mọi firmware.
Mở monitor sau khi nạp có thể bỏ lỡ log đầu tiên của ứng dụng.

## Sử dụng tool trực tiếp và CLI của SDK

Từ thư mục gốc repository:

```sh
python3 neon-mdf/tools/fu2_flash.py --version
python3 neon-mdf/tools/fu2_flash.py --list-boot-modes
python3 neon-mdf/tools/fu2_flash.py --image /path/to/my_app_image.bin --check-only
python3 neon-mdf/tools/fu2_flash.py \
  --port /dev/ttyUSB0 --baud 19200 \
  --image /path/to/my_app_image.bin \
  --monitor --monitor-baud 19200
```

Tool nhận `.bin` hoặc `.hex`/`.mem` dạng token byte/word; đây không phải parser
Intel HEX. Dùng `--hex-format byte|word` nếu cần chỉ rõ cách diễn giải.
`--byte-gap-ms` mặc định 2 ms để giãn các byte trả lời, giống tool
`uart_boot_loader.py` ban đầu. Log mặc định hiển thị từng yêu cầu và phản hồi:

```text
[RX #1] 00 00 00 00  request offset=0x00000000
[TX] 55 54 50 46  image[0x00000000:0x00000004]  0.0%
[RX #2] 04 00 00 00  request offset=0x00000004
[TX] ...  image[0x00000004:0x00000008]  ...%
```

Tool trả đủ 4 byte theo từng offset, ghi log bằng luồng riêng sau khi phản
hồi để việc hiển thị terminal không chặn vòng truyền. Không kiểm tra/hủy TX
giữa các byte của cùng một word. Nếu ROMCODE yêu cầu lại offset, tool gửi lại
đúng word đó. Có thể chọn `--log-mode progress` hoặc `--quiet` để chỉ xem
phần trăm; `--verbose` tương đương `--log-mode full`.

`--timeout` thay timeout yêu cầu; `--wait-timeout` giới hạn
thời gian chờ Reset đầu tiên. Mặc định nạp xong thì thoát, chỉ mở monitor nếu
có `--monitor`.

### Điều chỉnh tốc độ truyền

Các tham số được đọc lúc chạy, không cần configure lại:

| Biến môi trường | Tùy chọn tool trực tiếp | Giá trị mặc định |
|---|---|---|
| `NEON_MDF_UART_BYTE_GAP_MS` | `--byte-gap-ms` | `2` ms |
| `NEON_MDF_UART_LOG` | `--log-mode` | `full` (từng RX/TX); hoặc `progress` |

Mỗi word 4 byte có 3 khoảng nghỉ. Với image 31.476 byte và gap 2 ms, riêng
thời gian nghỉ trên PC cộng lại đã khoảng 47 giây, chưa tính truyền UART,
USB và thao tác Flash của ROMCODE. Đây cũng là cách giãn byte của tool gốc.

Để thử giảm thời gian nghỉ, có thể truyền `NEON_MDF_UART_BYTE_GAP_MS=0` trong
lệnh CMake, hoặc `--byte-gap-ms 0` khi gọi tool trực tiếp. Khi gap bằng 0,
tool gửi cả word bằng một lần ghi. Giá trị phù hợp phụ thuộc FIFO/clock và
board thực tế; nếu xuất hiện retry hoặc CRC không khớp, quay lại gap 2 ms
đang dùng ổn định. Tốc độ nạp còn phụ thuộc thời gian ghi Flash và ROMCODE
yêu cầu từng word, nên cần đo thời gian hoàn tất trên board.

Từ thư mục ứng dụng, sau khi đã đặt `NEON_MDF_PATH`:

```sh
python3 "$NEON_MDF_PATH/tools/neon_mdf.py" \
  -t FU2 -p /dev/ttyUSB0 -b 19200 flash-uart-fu2
```

CLI gọi cùng target CMake; có thể nối thêm action `monitor` và
`--monitor-baud 19200` trước danh sách action.

## Các boot mode và image XIP

| Boot mode | Hoạt động của ROMCODE | Vai trò của tool UART 1.0 |
|---|---|---|
| `000` | Kiểm tra và chạy image đã có trong Internal Flash | Không có dịch vụ nhận image UART |
| `100` | Nhận image qua UART và ghi Internal Flash | Nạp image profile `normal` |
| `101` | Đọc image từ nguồn QSPI ngoài và cập nhật Internal Flash | Cần chuẩn bị nguồn QSPI bằng công cụ tương ứng |
| `111` | Kiểm tra và chạy image tại vùng QSPI XIP | Cần nạp Flash ngoài bằng programmer hoặc loader UART–QSPI |

Image XIP có base `0x30000000`, khác image Internal Flash có base
`0x00020000`. Tool kiểm tra được cả hai profile bằng `--check-only`, nhưng
sẽ từ chối truyền image XIP bằng dịch vụ USART update trước khi mở cổng UART.
Thay `--boot-mode` chỉ mô tả chế độ yêu cầu, không điều khiển chân boot và
không bổ sung dịch vụ ghi QSPI cho ROMCODE.

## Sử dụng extension VS Code

1. Chọn target **FU2**, transport **UART** và cổng serial của board.
2. Đặt `neonMdf.fu2FlashBaudRate` khớp baud ROMCODE; mặc định của setting là
   19200. `neonMdf.baudRate` là baud ứng dụng, độc lập với baud nạp; khi chưa
   đặt, monitor FU2 dùng 19200.
3. Chọn **NEON-MDF: Flash** hoặc **Build + Flash + Log**. Extension đóng
   monitor đang mở, truyền port/baud lúc chạy CMake và hiển thị tool trong terminal.
4. Chọn boot mode 100, chờ thông báo UART sẵn sàng rồi nhấn RESET.
5. Với **Build + Flash + Log**, monitor ứng dụng được mở sau khi nạp thành công.

Luồng FU2 không sử dụng các thao tác B0/PB2 hoặc reset qua SWD của STM32.

## Xử lý lỗi

| Hiện tượng | Kiểm tra |
|---|---|
| Thiếu port/baud | Truyền `NEON_MDF_SERIAL_PORT` và `NEON_MDF_SERIAL_BAUD` trong lệnh nạp |
| Không mở được cổng | Tên cổng, quyền truy cập và chương trình Serial Monitor khác |
| Chờ Reset nhưng chưa có dữ liệu | Boot mode 100, kết nối TX/RX/GND, baud và thao tác Reset |
| `[RETRY]` | ROMCODE yêu cầu gửi lại; nếu kéo dài, kiểm tra kết nối, baud và khoảng cách giữa byte |
| Timeout sau header | ROMCODE có thể đang chuẩn bị Flash hoặc đã từ chối image; kiểm tra timeout và cấu hình Flash |
| Truyền đủ byte nhưng không có `[PASS]` | ROMCODE chưa xác nhận hoàn tất; kiểm tra CRC, chữ ký, NVR policy và kết nối |
| Image XIP bị từ chối | Dùng đường nạp Flash QSPI ngoài; USART update chỉ ghi Internal Flash |

ROMCODE không gửi NACK/mã lỗi chi tiết. Tool sẽ báo timeout, không suy diễn
nguyên nhân bảo mật cụ thể hoặc báo thành công chỉ dựa trên phần trăm truyền.
