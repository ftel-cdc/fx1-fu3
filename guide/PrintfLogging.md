# Xuất log bằng printf trên FU2

FU2 có bộ `printf` gọn, xuất qua syscall hook `_write`, không cần heap hoặc
stdio của newlib. Bộ formatter dùng truy cập word 32-bit để phù hợp SRAM FU2.
Với FU2, chỉ dùng cờ `CONFIG_NEON_MDF_DRIVER_UART`: bật UART là có cả
`printf/_write`, không cần cờ printf riêng. STM32F103 không cung cấp `_write`;
WEACT_F412 hiện không có retarget printf sang UART.

## Sử dụng trong app

Build bằng project helpers của SDK. Header `<stdio.h>` cung
cấp khai báo chuẩn; SDK cung cấp implementation của `printf`, `vprintf`,
`puts` và `putchar`. Bật UART trong sdkconfig:

```text
CONFIG_NEON_MDF_DRIVER_UART=y
```

Không cần tự viết `_write()` hoặc bật một cờ printf khác. Khi UART tắt, driver
và console không được biên dịch; app gọi printf sẽ báo lỗi link.
App chỉ cần include `<stdio.h>` rồi gọi `printf`. Lần xuất đầu tiên tự khởi tạo
UART0 ở **19200 baud, 8N1**, dùng `SystemCoreClock` do startup thiết lập.

```c
#include <stdio.h>

int main(void)
{
    if (printf("[INFO] counter=%u status=0x%08lX\n", 1U, 0x1234UL) < 0) {
        /* Log thất bại; có thể kiểm tra bằng debugger. */
    }
    for (;;) { }
}
```

Clock startup phải phù hợp board. Ví dụ `Printf_Log` dùng clock FPGA cố định
10 MHz. Console tự bật/reset UART0 và cấu hình baud ở lần ghi đầu tiên; không
tự thay đổi chính sách clock của toàn hệ thống.

Nếu cần UART/baud khác, có thể include `console/neon_console.h`, khởi tạo một
handle `static` theo cấu hình ứng dụng và gọi `neon_console_attach_uart()` trước
khi in. API này tùy chọn; chỉ chấp nhận handle READY, word length 8-bit và
timeout khác 0. Handle phải tồn tại trong suốt thời gian attach. Gọi
`neon_console_detach_uart()` trước khi deinit handle đó; lần in sau quay về UART0
mặc định. Các giao dịch UART0 khác phải phối hợp với console; lần printf đầu
tiên có thể reset UART0 nếu chưa attach handle ứng dụng.

## Cơ chế hoạt động

1. Khi family là FU2 và `CONFIG_NEON_MDF_DRIVER_UART=y`, CMake đưa
   `components/console/platform/fu2/neon_console.c` và
   `components/driver/uart/platform/fu2/neon_uart.c` vào thư viện
   `neon_mdf_driver`. Không có cờ printf riêng.
2. `<stdio.h>` khai báo `printf`. Khi app gọi hàm, linker lấy implementation
   của SDK từ thư viện driver. FU2 link với `-nostdlib` và dùng formatter
   của SDK, không link formatter stdio của newlib.
3. `printf` thu thập đối số bằng `va_list`, gọi `vprintf`, rồi formatter phân
   tích `%s`, `%d`, `%x`, width/precision… và sinh từng ký tự.
4. `emit` đặt ký tự vào một word 32-bit aligned, rồi gọi `_write(1, data, 1)`.
   `_write` tự khởi tạo UART0 nếu chưa có handle console, đổi LF thành CRLF,
   rồi gọi HAL UART. stdout (`file=1`) và stderr (`file=2`) dùng chung console.
5. Driver UART polling chờ TX sẵn sàng, ghi ký tự vào thanh ghi THR; phần cứng
   UART phát các bit ra chân TX theo baud đã cấu hình. USB-UART và Serial
   Monitor nhận luồng byte rồi hiển thị thành văn bản.

`neon_console_attach_uart` là cách tùy chọn để thay UART mặc định, không phải
bước bắt buộc trước printf. `_write` mới là tầng output chung của console.
Hàm không được app sử dụng có thể được linker loại khỏi firmware bằng
`--gc-sections`; khi bật UART, API printf có sẵn mà không cần cờ bổ sung.

### Syscall hook trên bare-metal

Với newlib, luồng xuất thường là `printf → stdio → _write → UART`. `_write`
là hàm C do firmware cung cấp cho thư viện C; trên bare-metal nó không nhất
thiết gọi vào kernel hoặc phát lệnh SVC. Nó có thể gọi HAL UART hoặc polling
và ghi thẳng thanh ghi TX, tùy backend.

FU2 dùng formatter riêng vì SRAM chỉ hỗ trợ truy cập word aligned. Chỉ thay
`_write` không sửa được truy cập byte/halfword trong formatter, buffer và các
hàm phụ trợ của newlib. Implementation hiện tại là
`printf → vprintf → formatter word-only → _write → HAL UART → TX`.
App không cần gọi driver; bên trong SDK, `_write` tái sử dụng HAL để quản lý
clock/reset/pinmux/baud và timeout, không lặp lại register logic.

Gọi trực tiếp `_write` trả số byte input đã tiêu thụ, có thể trả count một phần
khi UART lỗi sau khi gửi được dữ liệu, hoặc `-1` khi chưa gửi được byte nào.
`neon_console_get_status()` giải thích lỗi; backend bare-metal này không thiết
lập errno của newlib. Buffer truyền vào `_write` phải sở hữu trọn các word
aligned chứa nó, giống các buffer FU2 khác.

## Định dạng và hành vi

| Hỗ trợ | Ví dụ |
| --- | --- |
| Chuỗi, ký tự, dấu `%` | `%s`, `%c`, `%%` |
| Số nguyên signed/unsigned | `%d`, `%i`, `%u` |
| Hex và octal | `%x`, `%X`, `%o`, `%08lX`, `%#x` |
| Con trỏ | `%p` — luôn có tiền tố `0x`, kể cả con trỏ null |
| Integer length | `hh`, `h`, `l`, `ll`, `z` |
| Căn lề, dấu, padding | `-`, `+`, space, `#`, `0` |
| Width/precision | `%8.3s`, `%*.*d`; mỗi field tối đa 1024 |

Không hỗ trợ float (`%f/%e/%g/...`), `%n`, wide characters, positional arguments
hoặc toàn bộ thư viện stdio (`fprintf`, `snprintf`, `scanf`, `fflush`, ...).
Không cần `setvbuf()` hay `fflush()`: mỗi ký tự được gửi ngay bằng UART polling.
Không link thêm newlib stdio để mở rộng các hàm trên trên FU2, vì các đường đó
chưa bảo đảm truy cập SRAM bằng word.

LF (`\n`) tự đổi thành CRLF trên UART; chuỗi đã có `\r\n` không bị thêm CR
lần nữa. `printf/vprintf` trả số ký tự logic trước chuyển đổi newline. Khi gặp
định dạng không hỗ trợ hoặc lỗi UART, trả `-1`; phần đầu log có thể đã được gửi.
`neon_console_get_status()` trả `NEON_NOT_SUPPORTED`, `NEON_INVALID_PARAM`,
`NEON_BUSY`, `NEON_TIMEOUT` hoặc mã lỗi driver tương ứng. Một lần gọi thành công
tiếp theo cập nhật trạng thái; status không phải lỗi được lưu vĩnh viễn.

timeout là budget poll cho **mỗi byte UART**, không phải mili-giây hoặc deadline
toàn bộ dòng log. Log dài có thể chiếm main lâu hơn. Console từ chối lời gọi
lồng nhau/concurrent với BUSY; không chờ khóa. Trong FreeRTOS, dùng mutex hoặc
một task log để tuần tự hóa. Trong ISR, đưa dữ liệu/sự kiện cho main/task xử lý.
Nếu code khác cũng dùng UART đó hoặc gọi trực tiếp `_write`, ứng dụng phải bảo
vệ toàn bộ giao dịch để tránh xen kẽ dữ liệu giữa các ký tự log.

Format string và `%s` trong SRAM phải sở hữu trọn các word aligned chứa dữ
liệu, theo `neon_utils_memory.h`. Các buffer do DMA đang cập nhật cần bàn giao quyền
sở hữu trước khi in. Các string literal trong Flash được đọc bằng word aligned.
Gọi printf từ main/task sau startup/SystemInit. Không dùng trong reset/ISR
trước khi platform sẵn sàng.

## Build và xem log

Xem project hoàn chỉnh tại [examples/Printf_Log](../examples/Printf_Log/README.md).
SDK có Serial Monitor trong VS Code; chọn đúng port, baud 19200 và 8N1 cho
example này. Có thể mở trực tiếp bằng pyserial:

```sh
python3 -m serial.tools.miniterm /dev/ttyUSB0 19200
```

Nối UART0 TX của board tới RX của USB-UART, GND chung; xác nhận chân và mức
điện áp trong board map. Không suy ra khả năng chịu 3.3 V/5 V từ tên UART.
Reset sau khi mở monitor để thấy log khởi động; thoát bằng `Ctrl+]`.

## Trạng thái xác nhận

Đã kiểm tra build khi UART bật/tắt, format số nguyên 64-bit, đường `_write`,
khởi tạo UART tự động, xuống dòng CRLF và phục hồi sau timeout. Runtime FU2
dùng các hàm truy cập bộ nhớ theo word 32-bit.
Chưa xác nhận baud, chân UART hoặc log trên FPGA/silicon thật.
