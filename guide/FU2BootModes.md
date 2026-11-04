# Chế độ khởi động và quy trình chạy firmware FU2

## Nguyên lý khởi động

Sau Reset hoặc cấp nguồn, FU2 chạy ROMCODE trước. ROMCODE đọc
`SYSCSR_STATUS[2:0]` để chọn nguồn firmware, kiểm tra image và chuyển sang
ứng dụng nếu các điều kiện hợp lệ. Người dùng thiết lập chế độ boot bằng
chân/jumper/công tắc theo thiết kế board; tham số phần mềm phía PC không tự
thay đổi các chân này.

Các mã trong tài liệu là **giá trị logic của SoC**, không phải số hiệu jumper
hoặc thứ tự công tắc trên một board cụ thể. Dùng sơ đồ board để xác định cách
đặt các mức logic, giữ cấu hình ổn định trong lúc Reset.

| `STATUS[2:0]` | `MODE[1:0]` | `BOOT` | Chế độ | Nguồn image | Vị trí chạy ứng dụng |
|---|---|---|---|---|---|
| `000` | `00` | `0` | Internal Flash | Image đã nạp trong Internal Flash | Internal Flash |
| `100` | `10` | `0` | USART update | PC gửi image qua UART | Internal Flash sau khi cập nhật |
| `101` | `10` | `1` | QSPI indirect update | Image từ nguồn QSPI ngoài | Internal Flash sau khi sao chép |
| `111` | `11` | `1` | QSPI XIP | Image đã nạp trong Flash QSPI ngoài | Trực tiếp từ Flash QSPI ngoài |

Các mã `001`, `010`, `011`, `110` không chọn được đường khởi động ứng dụng.
ROMCODE quay lại xử lý Reset và đọc lại trạng thái. Để thao tác nhất quán,
đặt một chế độ hợp lệ rồi nhấn Reset.

## Chuẩn bị đúng image

SDK đóng gói image theo cấu trúc:

```text
[header 128 byte][vector table của ứng dụng][code/data của ứng dụng]
```

| Nội dung | Profile `normal` | Profile `xip` |
|---|---|---|
| Dùng cho chế độ | `000`, `100`, `101` | `111` |
| Địa chỉ image nhìn từ CPU | `0x00020000` | `0x30000000` |
| Địa chỉ vector table | `0x00020080` | `0x30000080` |
| Kích thước tối đa, gồm header | `0x80000` byte (512 KB) | `0x40000` byte (256 KB cửa sổ XIP) |
| File dùng để nạp | `<app>_image.bin` | `<app>_image_xip.bin` |

Địa chỉ liên kết của chương trình phải khớp với profile và header. Không đổi
địa chỉ trong header của image `normal` để biến nó thành image XIP.

Build profile mặc định từ thư mục ứng dụng:

```sh
cmake --preset FU2
cmake --build --preset FU2
```

Nếu project trước đó đã chọn XIP, chuyển lại bằng:

```sh
cmake --preset FU2 --fresh -DNEON_MDF_PROFILE=normal
cmake --build --preset FU2
```

Build image XIP:

```sh
cmake --preset FU2 --fresh -DNEON_MDF_PROFILE=xip
cmake --build --preset FU2
```

Profile là tham số ảnh hưởng linker và code nên cần chọn ở bước configure.
Port/baud là thông số giao tiếp khi nạp, chỉ truyền ở bước chạy target nạp.
Các file device/linker sinh tự động hiện được dùng chung: configure và build
từng profile tuần tự, không build đồng thời normal/XIP trong cùng SDK.

Kiểm tra image trước khi nạp, từ thư mục gốc repository:

```sh
python3 neon-mdf/tools/fu2_flash.py \
  --image /path/to/my_app_image.bin --check-only
```

## Chế độ 000 — Chạy từ Internal Flash

1. Chuẩn bị image profile `normal` và nạp vào Internal Flash trước, chẳng hạn
   theo quy trình USART update ở mục tiếp theo.
2. Chọn mã boot `000` trên board.
3. Nhấn Reset hoặc cấp nguồn lại.
4. ROMCODE kiểm tra image hiện có tại `0x00020000`, gồm header, CRC, vector và
   yêu cầu xác thực theo NVR policy. Nếu hợp lệ, ROMCODE chuyển sang ứng dụng.
5. Kiểm tra kết quả của ứng dụng: LED, UART hoặc chức năng ngoại vi đã viết.

Chế độ này không chờ PC gửi firmware. Không dùng tool UART để yêu cầu ghi
image trong lúc board đang ở mode `000`.

## Chế độ 100 — Nạp qua UART và chạy từ Internal Flash

1. Build ứng dụng theo profile `normal`. Board phải có NVR policy hợp lệ,
   tương thích với image Secure/Non-secure được dùng.
2. Nối UART nạp đúng theo board và chọn boot mode `100`. Đóng Serial Monitor
   khác đang giữ cổng.
3. Từ thư mục ứng dụng, chạy:

   ```sh
   cmake -E env \
     NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 \
     NEON_MDF_SERIAL_BAUD=19200 \
     NEON_MDF_UART_BYTE_GAP_MS=2 \
     NEON_MDF_UART_LOG=full \
     cmake --build --preset FU2 --target flash_uart_fu2
   ```

   Thay port và baud theo board thực tế; UART ROMCODE dùng 8N1, không có flow
   control. Không cần thêm port/baud vào lệnh configure.
4. Khi tool hiển thị `[WAIT] UART is ready`, nhấn **RESET trên board FU2**.
5. ROMCODE yêu cầu từng offset image, nhận dữ liệu, chuẩn bị Flash, ghi
   payload, kiểm tra image rồi ghi header sau cùng. Giữ kết nối và nguồn điện
   ổn định trong quá trình nạp.
6. Chờ `[PASS] Image transfer completed`. Tool chỉ báo kết quả này sau khi
   nhận `0xFFFFFFFF` từ ROMCODE và đã truyền đủ image. ROMCODE tiếp tục chuyển
   sang ứng dụng ngay trong phiên này.
7. Chuyển sang mode `000` trước những lần Reset/cấp nguồn tiếp theo để chạy
   image đã cài, tránh quay lại chế độ chờ cập nhật.

Tool hiển thị **boot mode yêu cầu**; nó không đọc hoặc xác nhận trực tiếp
giá trị chân boot trên board. Chi tiết tham số và xử lý lỗi nằm trong
[Nạp firmware FU2 qua UART](FU2Flashing.md).

## Chế độ 101 — Sao chép từ QSPI vào Internal Flash

1. Build image profile **`normal`**, vì ứng dụng cuối cùng sẽ chạy trong
   Internal Flash.
2. Chuẩn bị image ở nguồn QSPI ngoài. Với Flash NOR mà ROMCODE đọc từ đầu
   thiết bị, đặt toàn bộ `<app>_image.bin` tại **offset `0`**. Dùng programmer
   hoặc loader phù hợp với thiết kế board và đọc lại để xác nhận dữ liệu.
3. Kết nối nguồn QSPI với controller/CS theo board, chọn boot mode `101`,
   rồi nhấn Reset.
4. ROMCODE đọc header từ nguồn QSPI, kiểm tra địa chỉ image `0x00020000`, xóa
   vùng Internal Flash cần thiết và sao chép payload vào đó.
5. ROMCODE kiểm tra CRC, yêu cầu xác thực và vector, ghi header sau cùng rồi
   chuyển sang chạy ứng dụng trong Internal Flash.
6. Kiểm tra hoạt động ứng dụng. Chọn mode `000` cho những lần khởi động tiếp
   theo nếu không muốn lặp lại bước cập nhật từ QSPI.

Mode `101` không phải dịch vụ ghi vào Flash QSPI và không yêu cầu một image
XIP. Tool UART 1.0 chưa có backend chuẩn bị nguồn QSPI này. ROMCODE không
phát thông báo hoàn tất UART `0xFFFFFFFF` cho đường QSPI copy.

## Chế độ 111 — Chạy trực tiếp từ QSPI Flash

1. Build image profile **`xip`**, dùng file `<app>_image_xip.bin` và kiểm tra
   header base `0x30000000`. File `<app>_xip.bin` chỉ chứa payload raw.
2. Dùng programmer hoặc firmware loader UART–QSPI để nạp toàn bộ image vào
   Flash ngoài, tại **offset `0`** với bố trí image ở đầu thiết bị. Đọc lại
   để xác nhận nội dung. `0x30000000` là địa chỉ CPU ánh xạ tới Flash ngoài,
   không phải offset phải truyền cho programmer của chip Flash.
3. Kết nối Flash ngoài đúng theo board, chọn boot mode `111`, rồi nhấn Reset.
4. ROMCODE khởi tạo QSPI, bật cửa sổ memory-mapped và kiểm tra header, CRC
   theo cấu hình image, cùng vector ứng dụng tại vùng XIP.
5. Nếu hợp lệ, ROMCODE chuyển sang ứng dụng tại QSPI Flash và giữ cấu hình
   XIP hoạt động. Kiểm tra LED/UART hoặc chức năng của ứng dụng.
6. Giữ mode `111` cho các lần Reset/cấp nguồn tiếp theo nếu tiếp tục chạy XIP.

Trong ROMCODE hiện tại, đường XIP không thực hiện kiểm tra NVR policy và chữ
ký như các đường Internal Flash/update. Không suy ra rằng image đã được xác
thực chữ ký chỉ vì boot XIP thành công.

Tool UART 1.0 chưa cung cấp firmware loader để ghi QSPI Flash. Dịch vụ
USART update của ROMCODE luôn ghi Internal Flash và từ chối image XIP.
Muốn nạp Flash ngoài qua UART cần cả loader phía FU2 và phần giao tiếp PC;
thêm tùy chọn `--boot-mode 111` không tự tạo ra khả năng này.

## Kiểm tra firmware đã chạy

Với ứng dụng có UART log, mở monitor theo baud mà chính ứng dụng cấu hình:

```sh
cmake -E env \
  NEON_MDF_SERIAL_PORT=/dev/ttyUSB0 \
  NEON_MDF_MONITOR_BAUD=19200 \
  cmake --build --preset FU2 --target serial_monitor
```

Baud ứng dụng có thể khác baud nạp. Log đầu tiên có thể xuất hiện trước khi
monitor mở; với chế độ boot bình thường, có thể mở monitor trước rồi Reset
để quan sát lại. Không mở monitor đồng thời với tool đang nạp trên cùng cổng.

| Hiện tượng | Hướng kiểm tra |
|---|---|
| Mode `100` không có yêu cầu UART | Mã boot, thao tác Reset, UART nạp, baud, dây TX/RX/GND |
| Mode `000` không chạy | Image đã có trong Internal Flash chưa; header, CRC, vector, NVR policy |
| Mode `101` không chạy | Nguồn QSPI có image `normal` ở đúng offset; kết nối, định dạng image và NVR policy |
| Mode `111` không chạy | Image `xip`, offset Flash ngoài, loại Flash, cấu hình đọc QSPI và vector |
| Chọn mã không hợp lệ | Chọn lại một trong `000`, `100`, `101`, `111` rồi Reset |

ROMCODE không tự chuyển sang nguồn boot khác khi image được chọn không hợp
lệ và không gửi mã lỗi/NACK chi tiết. Khi khởi động lỗi, kiểm tra đúng đường
boot đang chọn thay vì chờ một cơ chế fallback tự động.
