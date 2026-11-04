# FPGA_Terminal

Terminal dòng lệnh chạy trên FU2 FPGA qua UART0. Một firmware cung cấp prompt
`fpga> ` để gọi các testcase. Hiện đã tích hợp UART echo polling từ ví dụ
`UART_ECHO_Polling`, nút bấm/LED từ `GPIO_Input_Output` với output đổi sang PB7,
ngắt `GPIO_Interrupt` trên PB5, LED nháy bằng `TIM_Periodic_Interrupt` trên PB7
và xóa/ghi/đọc Flash bằng `Flash_Read_Write`. UART RX interrupt và PWM
cũng đã có code. `SPI_W5500` thay cho hai khung `SPI_Loopback` và
`SPI_Master_Slave` trong kế hoạch ban đầu: hiện có 20 testcase,
8 bài có code và 12 bài chờ triển khai.

Mặc định: **UART0, 19200 baud, 8N1, FIFO tắt, clock FPGA 10 MHz**. Terminal
nhận lệnh bằng polling; testcase `gpio_irq` bật ngắt GPIO, `tim_irq` bật ngắt
timer. Testcase `uart_rx_irq` tạm bật FIFO và ngắt RX trên UART0,
rồi khôi phục cấu hình console khi thoát. Không dùng DMA hoặc FreeRTOS.

## Cấu trúc

```text
FPGA_Terminal/
├── CMakeLists.txt
├── CMakePresets.json
├── Kconfig
├── sdkconfig
├── README.md
├── Inc/
│   └── main.h                # Include dùng cho main.c
├── Src/
│   └── main.c                # UART, nhận dòng, bảng lệnh và dispatch
└── testcase/
    ├── CMakeLists.txt        # Đăng ký thư mục của từng testcase
    ├── UART_ECHO_Polling/
    ├── GPIO_Input_Output/
    ├── GPIO_Interrupt/
    ├── TIM_Periodic_Interrupt/
    ├── PWM_Output/
    ├── I2C_Register_Read_Write/
    ├── SPI_W5500/
    ├── Flash_Read_Write/
    ├── UART_RX_Interrupt/
    ├── ADC_Sampling/
    ├── DMA_Memory_Copy/
    ├── RTC_Calendar/
    ├── CRC_Known_Vector/
    ├── AES_Known_Vector/
    ├── TRNG_Read/
    ├── QSPI_Flash_Indirect/
    ├── I2C_Slave/
    ├── SPI_DMA/
    ├── I2C_DMA/
    └── FreeRTOS_Peripheral_Queue/
```

`Inc/`, `Src/` và `testcase/` cùng cấp. `main.c` include `Inc/main.h`, nơi
gộp các include của terminal. Khi thêm testcase, mỗi bài có một thư mục
riêng dưới `testcase/`, chứa header và source. Thêm include header của bài
vào `Inc/main.h` và handler vào bảng `commands` trong `main.c`; `help` đọc
cùng bảng để liệt kê lệnh.
Source `.c` được thêm qua `target_sources(fpga_terminal PRIVATE ...)` trong
CMake của testcase, không include trực tiếp `.c` vào `main.c`.

Handler có dạng `void handler(void)`. Terminal lấy từ đầu tiên làm tên lệnh
và bỏ qua phần nhập thêm phía sau. Handler trả về để terminal hiện lại
prompt. Wrapper của lệnh
`uart_echo`, `gpio_io`, `gpio_irq`, `tim_irq` và `spi_w5500` truyền handle UART0 hiện tại
cho testcase.

Lệnh `flash_rw` chạy một lượt rồi trả về, không cần nhận UART.

Mỗi thư mục khung mới có `CMakeLists.txt`, file `.h` khai báo hàm
`Run(void)` và file `.c` chứa stub của hàm đó. Bảng lệnh gọi wrapper private
`static_fpga_terminal_command_*` trong `main.c`, wrapper gọi `Run()` của bài.
Stub in `[TODO] <testcase> is not implemented yet.` rồi trở về prompt. Phần
thiết lập ngoại vi, truyền dữ liệu và kiểm chứng sẽ được điền sau; các
khung DMA/FreeRTOS cũng chưa bật driver hoặc scheduler.

## UART RX Interrupt (`uart_rx_irq`)

Ví dụ tối giản chỉ có hai hàm: `USART0_IRQHandler()` nhận dữ liệu và
`uart_rx_interrupt_run()` thiết lập UART, chạy main rồi trả về terminal.
Luồng dữ liệu: PC → UART RX FIFO → ISR → ring buffer → main → echo về PC.

Nhập `uart_rx_irq`, chờ dòng `[READY]` rồi gửi chuỗi. ISR đọc các byte đang
có trong FIFO vào mảng `g_rx_buffer`; main lấy từng byte ra, in log nhận
`[PC -> FPGA]` rồi echo nguyên byte với nhãn `[FPGA -> PC]`. Ví dụ gửi `A`:

```text
[PC -> FPGA] 'A' (0x41)
[FPGA -> PC]  A  (0x41)
```

Log nằm ở main, sau khi byte đã được lấy khỏi FIFO và ring; không biểu thị
dữ liệu hiện còn trong FIFO. Ký tự không in được hiển thị bằng `.` ở dòng
nhận; giá trị hex cho biết byte thực tế. Dòng gửi vẫn truyền byte echo gốc,
nên CR/LF có thể làm xuống dòng. Log từng byte tăng lượng dữ liệu TX, nên
gửi từng ký tự hoặc burst ngắn để dễ quan sát. Tắt local echo trên PC.

Mảng dùng phần tử `uint32_t` theo yêu cầu bộ nhớ FU2. `g_rx_head` là
vị trí ghi tiếp theo, `g_rx_tail` là vị trí đọc tiếp theo, `g_rx_count` là
số byte đang chờ. Main khóa ngắt trong đoạn ngắn cập nhật ring.

Kích thước ring đặt bằng `CONFIG_UART_RX_INTERRUPT_RING_BUF_SIZE`, mặc
định 128 slot. Ring đầy thì bỏ byte mới và tiếp tục lấy dữ liệu khỏi FIFO.
Không có struct thống kê, hàm phụ, log lỗi hay kiểm tra điều kiện khởi tạo
trong ví dụ. Ví dụ dùng UART0 8-bit đã được terminal cấu hình sẵn; các kiểm
tra bên trong driver UART vẫn do driver thực hiện.

Biến `g_main_work` tăng mỗi lượt main, có thể quan sát trong debugger để
thấy main vẫn chạy khi chưa gửi dữ liệu. Ctrl+C (`0x03`) đặt cờ dừng ngay
trong ISR, kể cả khi ring đầy. Main echo hết các byte đã xếp hàng trước đó,
tắt ngắt RX và khôi phục cấu hình UART để hiện lại `fpga> `. LF dư của lệnh
CRLF được bỏ qua. Dữ liệu dư đã nhận sau Ctrl+C bị bỏ; chờ prompt trước
khi gửi lệnh tiếp theo.

TX dùng polling. `neon_uart_try_receive()` cho phép ISR nhận RX trong lúc
main đang truyền trên cùng handle. Trước khi đổi cấu hình, ví dụ gọi
`neon_uart_wait_tx_complete()` để chờ byte cuối truyền hết trên dây.
Quá trình thiết lập FIFO có reset UART, nên chỉ gửi payload sau `[READY]`.

Kiểm tra: gửi `ABC123` và quan sát echo đúng thứ tự, nhấn Ctrl+C rồi thử
`help`; chạy lại `uart_rx_irq` để kiểm tra khả năng vào/thoát nhiều lần.
Xác nhận hoạt động IRQ/FIFO trên FPGA thực tế.

## SPI W5500 (`spi_w5500`)

FU2 là SPI0 master, W5500 là slave. Testcase kiểm tra giao tiếp SPI bằng
thanh ghi và SRAM của W5500; chưa cấu hình IP, socket hoặc truyền Ethernet.
Không cần cắm cáp LAN để chạy các phép thử này.

Đấu dây theo bảng dưới, nối chung GND, cấp nguồn đúng yêu cầu
module và bảo đảm RSTn đã được kéo lên/mạch reset hoạt động. Tín hiệu
W5500 dùng mức 3,3 V; INTn chưa sử dụng.

| SPI0 | Chân PMOD | Package FU2 | Die pad / nhóm SYSCSR |
|---|---|---|---|
| SCK | IO5 | 60 | PIN_70 / right[21] |
| MOSI | IO4 | 62 | PIN_74 / top[1] |
| MISO | IO1 | 61 | PIN_73 / top[0] |
| SS0 → W5500 SCSn | IO0 | 63 | PIN_75 / top[2] |

Driver SPI tự chọn mux và cấu hình pad khi `pin_config=AUTO` (mặc định).
DeInit khôi phục các bit pad/mux đã lưu; testcase không ghi SYSCSR trực tiếp.
`KEEP` dành cho ứng dụng đã tự cấu hình chân. Số package và số die pad khác
nhau. UART0 vẫn dùng để nhập lệnh và in log.

Nhập `spi_w5500` rồi Enter. Mặc định mode 3, MSB trước, SCK 1 MHz, hardware
SS0; tại clock hệ thống 10 MHz, divider bằng 4. Testcase đợi 560 ms cho
mạch reset module, đọc `VERSIONR` (phải bằng `0x04`), ghi MR=`0x80` để soft
reset rồi chờ MR về `0x00`. Chỉ reset/ghi thử RCR sau khi đọc đúng ID.
Sau `[READY]`, nhấn từng phím, không cần Enter:

| Phím | Thao tác |
|---|---|
| `i` | Chạy lại chờ khởi động và soft reset W5500 |
| `t` | Đọc VERSIONR, MR, PHYCFGR, RTR, RCR và trạng thái LINK |
| `p` | Lưu RCR; ghi/đọc `0x55`, `0xAA`; ghi lại và kiểm tra giá trị ban đầu |
| `r` | Đọc liên tiếp 32 byte socket 0 TX SRAM tại offset `0x0000` |
| `b` | Tắt auto, quét B2B từ 10 xuống 0 chu kỳ SCK; đọc lặp và thống kê lỗi CRC32/ID/SPI |
| `d` | Chỉ đọc VERSIONR/RCR/PHYCFGR ở mode 0/3, 1 MHz/100 kHz, rồi trả lại mode/tốc độ đã chọn |
| `0`–`4` | Mode 0: 100 kHz, 500 kHz, 1 MHz, 2,5 MHz, 5 MHz |
| `5`–`9` | Mode 3: cùng năm mức tốc độ trên |
| `a` | Bật/tắt ping/pong tự động mỗi 3 giây |
| `h` / `?` | In hướng dẫn |
| Ctrl+C / `q` | Tắt SPI0, khôi phục pad và SysTick, trở về `fpga> ` |

Auto mặc định OFF. Chọn một preset `0`–`9` sẽ chạy ping/pong ngay và bật
auto. `CONFIG_SPI_W5500_SCK_HZ` đặt tốc độ ban đầu; divider làm tròn lên
để SCK thực không vượt giá trị yêu cầu. `CONFIG_SPI_W5500_TIMEOUT_POLLS`
là số vòng chờ mỗi giao dịch, không phải mili giây.

Driver FU2 mặc định nghỉ 10 chu kỳ SCK giữa các byte, kể cả các byte trong
word 32 bit. Chỉnh `NEON_SPI_FU2_B2B_SCK_CYCLES` trong `spi/neon_spi.h`
rồi build lại để đổi mặc định. Driver tự tính
`T_B2B = between_frames * 2 * (clock_divider + 1)` khi đơn vị là
`NEON_SPI_B2B_SCK_CYCLES`, kể cả khi đổi divider lúc chạy.
Tại core 10 MHz/SCK 1 MHz, giá trị mặc định của `T_B2B` là 100, tương ứng
10 µs. Giá trị thanh ghi phải vừa 16 bit; cấu hình vượt giới hạn bị từ chối.

Log `[FU2 -> W5500]` hiển thị lệnh/TX; `[W5500 -> FU2]` hiển thị RX thô
và byte dữ liệu thấp. Ví dụ đọc VERSIONR gửi `0x00390000`: hai byte địa chỉ,
một byte control và một byte dummy. Ba byte RX của phần header bị bỏ qua.
RX khi ghi không phải ACK; kết quả ghi được xác nhận bằng lần đọc sau.
Ping/pong chỉ `[PASS]` nếu cả hai mẫu đọc đúng và RCR đã khôi phục đúng.

Burst gửi header `00 00 10` (BSB=2, read, VDM), sau đó 32 byte dummy. Giữ
SS0 LOW bằng manual hold: tám word 32-bit chứa header và 29 byte dữ liệu,
rồi một word 24-bit nhận ba byte cuối và release CS. Phần cứng tạo B2B
giữa các byte trong word. Sau đó trở về word 32-bit và SS bình
thường. Nội dung SRAM sau reset không cố định nên dump này không dùng
để kết luận PASS/FAIL hoặc làm mẫu kiểm tra bộ nhớ.

Phím `b` chỉ đọc VERSIONR và cùng 32 byte TX SRAM, không ghi W5500.
Socket không chạy nên vùng TX SRAM giữ nguyên trong suốt bài đo. Ở B2B=10,
test lấy CRC32 lần đọc đầu làm mẫu và kiểm chứng bằng toàn bộ lượt đọc
của mức này. Nếu VERSIONR khác `04`, CRC32 thay đổi hoặc giao dịch lỗi thì
dừng vì mẫu chuẩn chưa ổn định. Các mức 9..0 so với cùng CRC32 đó, không
cập nhật mẫu theo lần đọc mới. Mỗi lượt chỉ tính fail một lần; các cột
CRC/ID/IO cho biết nguyên nhân và có thể trùng nhau.

`CONFIG_SPI_W5500_B2B_TRIES` đặt số lượt đọc mỗi mức (mặc định 1.000).
Log báo số lượt fail, tỷ lệ phần trăm và mức nghỉ nhỏ nhất không có lỗi
trong lần chạy. Không in từng giao dịch khi quét. Nhấn `q`/Ctrl+C để hủy
và về menu; test luôn thử khôi phục cấu hình ban đầu, auto giữ OFF.
Chạy lại ở các mode/tốc độ cần dùng trước khi chọn mặc định mới. CRC32
kiểm tra tính lặp lại so với mẫu, chưa chứng minh nội dung SRAM là đúng;
VERSIONR là kiểm tra bổ sung với giá trị đã biết.

SPI dùng polling, không dùng DMA/IRQ. SysTick chạy đếm chu kỳ, không bật
ngắt; nếu SysTick đang bật thì testcase báo bận và trả về. Timeout SPI sẽ
reset controller trước giao dịch tiếp theo; burst lỗi không in dump dở.

Kiểm tra trên board: đọc ID `04`, chạy `p` thấy PASS và RCR giữ nguyên,
thử các preset, `r`, `d`, bật/tắt auto rồi Ctrl+C và chạy lại `help`.
Dùng FPGA/logic analyzer để xác nhận pin, timing, CS và đường MISO/MOSI thực tế.

## Lệnh hiện có

| Lệnh | Testcase/chức năng | Ưu tiên | Trạng thái |
|---|---|---|---|
| `help` | Liệt kê lệnh | — | Sẵn có |
| `info` | Thông tin terminal | — | Sẵn có |
| `clear` | Xóa màn hình | — | Sẵn có |
| `uart_echo` | UART_ECHO_Polling | P1 | Có code |
| `gpio_io` | GPIO_Input_Output | P1 | Có code |
| `gpio_irq` | GPIO_Interrupt | P1 | Có code |
| `tim_irq` | TIM_Periodic_Interrupt | P1 | Có code |
| `pwm_output` | PWM_Output | P1 | Có code |
| `i2c_rw` | I2C_Register_Read_Write | P1 | Khung `[TODO]` |
| `spi_w5500` | SPI_W5500 | P1 | Có code |
| `flash_rw` | Flash_Read_Write | P1 | Có code |
| `uart_rx_irq` | UART_RX_Interrupt | P2 | Có code |
| `adc_sampling` | ADC_Sampling | P2 | Khung `[TODO]` |
| `dma_memcpy` | DMA_Memory_Copy | P2 | Khung `[TODO]` |
| `rtc_calendar` | RTC_Calendar | P2 | Khung `[TODO]` |
| `crc_vector` | CRC_Known_Vector | P2 | Khung `[TODO]` |
| `aes_vector` | AES_Known_Vector | P2 | Khung `[TODO]` |
| `trng_read` | TRNG_Read | P2 | Khung `[TODO]` |
| `qspi_flash` | QSPI_Flash_Indirect | P2 | Khung `[TODO]` |
| `i2c_slave` | I2C_Slave | P3 | Khung `[TODO]` |
| `spi_dma` | SPI_DMA | P3 | Khung `[TODO]` |
| `i2c_dma` | I2C_DMA | P3 | Khung `[TODO]` |
| `freertos_queue` | FreeRTOS_Peripheral_Queue | P3 | Khung `[TODO]` |

“Có code” chỉ mô tả trạng thái triển khai, không xác nhận bài đã đạt kiểm tra
phần cứng. Các mục `[TODO]` chưa thực hiện thao tác ngoại vi.

Các lệnh trên phân biệt chữ hoa/thường và bỏ qua phần nhập thêm sau tên lệnh.
Nhập lệnh rồi nhấn Enter; hỗ trợ CR, LF và CRLF. Ví dụ:

```text
FU2 FPGA terminal
Type 'help' to list commands.
fpga> help
Available commands:
  help               Show available commands
  info               Show terminal configuration
  clear              Clear the terminal screen (ANSI)
  uart_echo          UART0 polling echo; Ctrl+C returns to terminal
  gpio_io            PB5 button -> PB7 output; Ctrl+C returns to terminal
  gpio_irq           PB5 rising-edge IRQ counter; 'i' injects; Ctrl+C returns
  tim_irq            TIM1 500 ms IRQ -> PB7 LED blink 1 Hz; Ctrl+C returns
  flash_rw           Erase/program/verify reserved Flash sector at 0x0009FE00
  uart_rx_irq        UART0 rx interrupt; Ctrl+C returns to terminal
  pwm_output         TIM0_CH0 PWM output; Ctrl+C returns to terminal
  i2c_rw             [TODO] I2C register read/write
  spi_w5500          SPI0 W5500 register read/write + burst; Ctrl+C returns
  adc_sampling       [TODO] ADC sampling
  dma_memcpy         [TODO] DMA memory copy
  rtc_calendar       [TODO] RTC calendar
  crc_vector         [TODO] CRC known vector
  aes_vector         [TODO] AES known vector
  trng_read          [TODO] TRNG random data
  qspi_flash         [TODO] QSPI Flash indirect access
  i2c_slave          [TODO] I2C slave
  spi_dma            [TODO] SPI DMA
  i2c_dma            [TODO] I2C DMA
  freertos_queue     [TODO] FreeRTOS peripheral queue
fpga> info
Target: FU2 FPGA
Core clock: 10000000 Hz
Terminal: UART0, 19200 baud, 8N1, polling
Command limit: 127 characters
fpga>
```

## Testcase UART_ECHO_Polling

Nhập `uart_echo` rồi Enter. Testcase dùng chung handle UART0, baud, clock và
FIFO mặc định của console; không reset hoặc khởi tạo lại UART khi vào/ra.
Mỗi byte nhận được sẽ in thông tin `[USER -> FPGA]`, gửi lại chính byte đó
bằng `neon_uart_transmit`, rồi in kết quả `[FPGA -> USER]`, như ví dụ gốc:

```text
fpga> uart_echo
[INFO] UART echo ready. Type a character; Ctrl+C returns to terminal.
[USER -> FPGA] 'A' (0x41)
[FPGA -> USER] A (0x41)
```

Nhấn Ctrl+C để dừng testcase và quay lại `fpga> `. Dòng `[STATS]` in số byte
RX/TX, số RX/TX timeout, số RX/TX lỗi, trạng thái và mã lỗi gần nhất. Các số
đếm không gồm dữ liệu chẩn đoán, Ctrl+C và LF thuộc Enter CRLF khi gọi lệnh.
Biến `g_uart_echo_stats` có thể xem trong debugger; thống kê được xóa khi chạy
testcase lần tiếp theo.

Khi testcase đang chạy, các ký tự như `help` là dữ liệu echo. Thoát bằng Ctrl+C
trước khi nhập lệnh terminal khác. RX timeout khi chờ gõ là bình thường và
được đếm; lỗi RX/TX được ghi nhận rồi tiếp tục nhận, không tự gửi lại byte lỗi.
Chỉ việc echo thành công chưa tự xác nhận testcase đạt; cần đối chiếu byte
nhận/gửi và các bộ đếm lỗi trên phần cứng.

Tắt local echo trên terminal máy tính vì FPGA đã echo ký tự. Ở prompt
`fpga> `, Backspace và DEL xóa ký tự cuối; Tab thành một khoảng trắng;
Ctrl+C hủy dòng đang nhập.
Phím mũi tên/function key được bỏ qua, chưa có history hoặc autocomplete.
Không cần nhập lại firmware để chạy một lệnh terminal khác.

Buffer mặc định 128 byte chứa tối đa 127 ký tự và ký tự kết thúc. Dòng quá
dài hoặc có lỗi RX sẽ bị bỏ toàn bộ đến Enter; Ctrl+C cũng cho phép nhập lại.
RX timeout khi chưa có dữ liệu là trạng thái idle bình thường. Đây là terminal
tương tác: gõ lệnh hoặc gửi từng dòng, chờ prompt rồi gửi tiếp; chưa có flow
control để bảo đảm nhận các burst/paste dài liên tục.

## Testcase GPIO_Input_Output

Nhập `gpio_io` rồi Enter. Testcase giữ input **GPIOB5 (PB5)** của ví dụ gốc
và dùng **GPIOB7 (PB7)** làm output active-high:

| Chân | Cấu hình và hoạt động |
|---|---|
| PB5 | Input pull-up, đồng bộ input; nút nối xuống GND, nhấn = 0, nhả = 1 |
| PB7 | Output khởi động thấp; nhấn nút = 1, nhả nút = 0 |

Nối PB7 tới LED qua điện trở hoặc thiết bị đo theo pin map của PDI. Nếu PDI
không hỗ trợ pull-up PAD, dùng điện trở kéo lên ngoài cho PB5. GPIO UART0
vẫn được giữ cho terminal.

```text
fpga> gpio_io
[INFO] PB5 button -> PB7 LED. Ctrl+C returns to terminal.
[GPIO] PB5=1 RELEASED -> PB7=0 (LED OFF)
[GPIO] PB5=0 PRESSED -> PB7=1 (LED ON)
[GPIO] PB5=1 RELEASED -> PB7=0 (LED OFF)
```

Log xuất khi vào testcase và khi mức PB5 thay đổi. Vòng lặp đọc GPIO trực
tiếp và kiểm tra UART không chờ để nhận Ctrl+C; các ký tự khác được bỏ qua.
Khi thoát, PB7 được ghi thấp rồi PB5/PB7 được DeInit, tắt output trước khi
trở lại prompt. Có thể chạy lại `gpio_io` hoặc `uart_echo` ngay sau đó.
Testcase không chống dội nút, nên một lần nhấn/nhả có thể tạo nhiều log.

## Testcase GPIO_Interrupt

Nhập `gpio_irq` rồi Enter. Testcase cấu hình **PB5 input pull-up**, bật đồng
bộ input và ngắt cạnh lên. Với nút PB5 nối xuống GND, nhấn tạo cạnh xuống;
**nhả nút tạo cạnh lên và tăng bộ đếm**. Có thể dùng nguồn tín hiệu ngoài
với mức điện áp phù hợp bank thay cho nút.

GPIOB nối tới **IRQ20** theo FU2 Common Specification v8 và wiring
`gpioB.intr => armcortexM0.intisr[20]` trong `FVP_FU2.lisa` của nguồn FU2.
Header/startup hiện tại của SDK đặt tên đường này là `EXTI1_IRQn` và
`EXTI1_IRQHandler`. Testcase dùng CMSIS `NVIC_EnableIRQ`/`NVIC_DisableIRQ`
trực tiếp. BootROM v5 chuyển tiếp ngắt tới vector ứng dụng như mô tả trong
[FU2 build/boot contract](../../guide/FU2.md).

ISR đọc `neon_gpio_get_interrupt_status()`, xử lý riêng PB5, xóa raw pending
và software injection qua `neon_gpio_clear_interrupt()`, rồi tăng biến
`g_gpio_interrupt_count`. Vòng lặp testcase đọc bộ đếm và in log; ISR không
gọi `printf`. Bộ đếm là word 32-bit, được xóa mỗi lần chạy testcase.

```text
fpga> gpio_irq
[INFO] PB5 rising-edge interrupt on IRQ20 (EXTI1).
[INFO] Release the button to count; 'i' injects an IRQ; Ctrl+C returns.
[GPIO IRQ] PB5 events=1 (+1)
[GPIO IRQ] PB5 events=2 (+1)
[INFO] GPIO interrupt stopped. Total=2
fpga>
```

Nhấn **`i`** để đặt software injection của PB5; ISR phải chạy một lần rồi
xóa nguồn ngắt. Đây là kiểm tra đường GPIO interrupt/NVIC/ISR, không xác
nhận chân PAD ngoài. Các ký tự khác được bỏ qua, Ctrl+C dừng testcase.
Khi thoát, code tắt NVIC và mask PB5, xóa cờ GPIO/NVIC rồi DeInit PB5.
PRIMASK được khôi phục về giá trị trước khi vào testcase.
PB7 không được sử dụng trong testcase này. Có thể chạy lại `gpio_irq` hoặc
chuyển sang `gpio_io`/`uart_echo` mà không nạp lại firmware.

Với nguồn cạnh sạch, mỗi lần ISR phục vụ PB5 tăng bộ đếm một đơn vị. Nếu có
nhiều IRQ trong lúc in, dòng log báo tổng mới và số tăng thêm `(+N)`.
Testcase chưa chống dội phần mềm; nút cơ có thể tạo nhiều cạnh lên. Các cạnh
đến quá nhanh khi cờ đang pending có thể gộp lại, nên đây không phải bộ đếm
xung tốc độ cao. Đối chiếu số cạnh và bộ đếm trên FPGA để xác nhận phần cứng.

## Testcase TIM_Periodic_Interrupt

Nhập `tim_irq` rồi Enter. Testcase dùng **TIM1 CH0, IRQ13
(`TIM1_IRQHandler`)** và **GPIOB7 (PB7)** làm output. Nối PB7 tới LED qua
điện trở theo pin map của PDI. PB7 bắt đầu mức thấp, sau đó đảo mức mỗi
**500 ms** ngay trong ISR: sáng 0,5 giây, tắt 0,5 giây, nháy **1 Hz** để
quan sát bằng mắt. Nếu LED trên board là active-low thì mức sáng/tắt đảo lại.

Timer đếm xuống, bật auto-reload và ngắt update. Clock TIM1 được lấy bằng
`SystemCoreClock`, phải khớp clock FPGA thực tế. Cấu hình mặc định 10 MHz:

```text
TIM_PERIODIC_TICK_HZ = 2
prescaler = SystemCoreClock / 1000 - 1 = 9999
auto_reload (CHL) = 1000 / TIM_PERIODIC_TICK_HZ - 1 = 499
T_update = (prescaler + 1) * (CHL + 1) / timer_clock = 0,5 s
```

Counter dùng tick 1 ms, cùng cấu hình prescaler với phần chạy ngắt định kỳ
của `timer_irq_wdt`. Trên FPGA đã thử, STT chỉ đọc. Theo bên IP, đọc STT
không xóa cờ hoặc clear IRQ và cờ UPDATE không tự hạ ở tick kế tiếp.
Cơ chế acknowledge trên phiên bản FPGA đang thử chưa được xác nhận.

Testcase chỉ bật ngắt UPDATE của CH0. ISR đọc STT_CH0; khi có cờ UPDATE
mới đảo PB7, tăng `g_tim_periodic_ticks`, rồi dùng vòng `while` đọc STT
đợi UPDATE về 0 trước khi thoát để tránh đếm lặp. Vòng chờ chỉ đọc STT,
không thực hiện acknowledge và không có timeout. Nếu cờ giữ mức 1,
vòng chờ sẽ không kết thúc; cần bên IP xác nhận thao tác acknowledge.
Log được in ở vòng lặp chính; ISR không gọi `printf`.
Vòng lặp chính kiểm tra UART để nhận Ctrl+C và in tổng số tick mỗi hai tick
(khoảng một giây). Bộ đếm là số ngắt đã xử lý, không tự đo độ chính xác thời gian.

```text
fpga> tim_irq
[INFO] TIM1 CH0 update interrupt on IRQ13, clock=10000000 Hz.
[INFO] PSC=9999, CHL=499; PB7 toggles every 500 ms (LED 1 Hz).
[INFO] Ctrl+C stops the timer and returns to terminal.
[TIM IRQ] ticks=2 (+2)
[TIM IRQ] ticks=4 (+2)
```

Nhấn Ctrl+C để dừng counter, tắt ngắt timer/NVIC và xóa pending. Testcase
ghi PB7 thấp rồi DeInit chân trước khi trả về prompt; số tick được xóa khi
chạy lại. PRIMASK được lưu/khôi phục giống `gpio_irq`. Bài này dành riêng
**toàn bộ TIM1** vì lúc vào có pulse reset để xóa trạng thái cả CH0/CH1.
CH1 được DeInit để chỉ CH0 có thể phát ngắt. CHL được ghi lại sau Start
vì bản FPGA tham khảo chỉ nạp CNT từ CHL khi kênh đang chạy.

Khi đo bằng scope/logic analyzer, bỏ qua cạnh đầu/cuối lúc start/stop. Khoảng
cách hai cạnh liên tiếp dự kiến 500 ms; một chu kỳ đầy đủ dự kiến 1 s. Mục tiêu
kiểm tra chu kỳ là ±1% so với giá trị đặt khi clock đã đúng; jitter cần ghi
nhận từ phép đo thực tế. Kiểm tra sau Ctrl+C không còn đảo mức và có thể
chạy lại `tim_irq`, `gpio_io`, `gpio_irq`. LED nháy và log tick chỉ xác nhận
luồng ngắt hoạt động; cần thiết bị đo để đánh giá chu kỳ/jitter.

Giữ `CONFIG_NEON_MDF_DRIVER_TIM=y` trong `sdkconfig`. Muốn đổi tốc độ nháy,
sửa `TIM_PERIODIC_TICK_HZ` trong source; mỗi ngắt đảo LED một lần nên tần số
nháy bằng một nửa tần số ngắt.

Spec rev1.0 ghi STT là RW nhưng không mô tả cách xóa; FPGA đã thử báo
lỗi bus khi ghi STT. Backend hiện chỉ đọc STT qua `GetActiveFlags()` và
`neon_tim_clear_flag()` trả `NEON_NOT_SUPPORTED` vì chưa có cách clear
được xác nhận cho phiên bản FPGA này. Cần bên IP cung cấp địa chỉ và thao
tác acknowledge chính xác, gồm cả side effect của lần đọc STT.

## Testcase Flash_Read_Write

Nhập `flash_rw` rồi Enter để chạy một lượt và tự trở về prompt. Lệnh xóa
**sector 512 byte tại 0x0009FE00–0x0009FFFF**, sector cuối của main Flash
512 KB theo map FU2 hiện tại. Mọi dữ liệu cũ trong sector này sẽ bị xóa.
Testcase không gọi chip erase và không truy cập NVR.

`flash_read_write.ld` dành riêng vùng này bằng section `NOLOAD`, cung cấp
địa chỉ cho code qua `__flash_rw_start__` và chặn link nếu image chạm tới
sector test. Vùng scratch không chứa dữ liệu khởi tạo trong image. Script
được ghép với linker script của SDK và yêu cầu profile FU2 normal chạy từ
internal Flash; không sửa linker script được sinh tự động của SDK.

Luồng kiểm tra dùng các API public của driver Flash:

1. GetDefaultConfig và config driver.
2. EraseSector và ReadWord kiểm tra đủ 128 word đã về `0xFFFFFFFF`.
3. ProgramWord đủ 128 word bằng pattern 0/1, xen kẽ bit và dữ liệu có
   thay đổi theo nhóm địa chỉ để phát hiện ghi nhầm địa chỉ.
4. ReadWord đối chiếu từng word, báo PASS hoặc địa chỉ/dữ liệu lỗi đầu tiên.
5. DeInit driver trước khi trả về terminal; pattern được giữ trong sector.

Ở cả profile `normal` và `xip`, `flash_rw` thử trên sector Internal Flash
`0x0009FE00`, không ghi vào QSPI Flash chứa image XIP. Linker kiểm tra vùng
image normal không chồng lên sector này.

```text
fpga> flash_rw
[INFO] Flash scratch: 0x0009FE00, 512 bytes; contents will be erased.
[INFO] Erasing scratch sector.
[PASS] Erase read-back: 128 words are 0xFFFFFFFF.
[INFO] Programming 128 pattern words.
[PASS] Flash_Read_Write: 128/128 words match.
fpga>
```

Các địa chỉ đọc/ghi cách nhau 4 byte và dữ liệu là `uint32_t`. Driver đặt hàm
erase/program và vòng chờ command trong `.ramfunc`, được startup copy
sang RAM qua `.data`; lưu/khôi phục PRIMASK khi thao tác Flash. timeout
command tính theo số vòng poll, không phải mili-giây. Testcase tuân theo
map và command engine FU2 v8 mà driver đang sử dụng; cần chạy trên FPGA để
xác nhận hành vi phần cứng thực tế.

Theo Flash Controller Specification v1.4 mục 9.1, config chờ BUSY và START
cùng về 0 trước khi dùng lại INIT_DONE của BootROM hoặc phát INIT. Khi phát
command, driver ghi opcode với START=0 rồi mới đặt START=1. Mỗi vòng chờ
có giới hạn operation_timeout_polls; hết hạn trả NEON_TIMEOUT thay vì chờ vô hạn.
neon_flash_init trả neon_status_t và không đặt trong `.ramfunc`;
các helper phát lệnh/chờ hoàn tất vẫn chạy từ SRAM và giữ ngắt bị khóa
trong thời gian command hoạt động.

## Lưu ý khi dùng ngắt trên FU2 FPGA

**BootROM v5 bàn giao ứng dụng với `PRIMASK=1`, tức ngắt toàn cục đang bị
khóa.** `NVIC_EnableIRQ()` chỉ mở một đường IRQ trong NVIC, không xóa
PRIMASK. Vì vậy, ngoại vi có thể đã phát yêu cầu và NVIC đã pending nhưng
CPU vẫn chưa vào ISR.

Sau khi cấu hình ngoại vi, chuẩn bị dữ liệu dùng chung với ISR và xóa cờ
pending cũ, cần mở cả NVIC lẫn ngắt toàn cục. Ví dụ trong `gpio_irq`:

```c
uint32_t saved_primask = __get_PRIMASK();
/* Cấu hình GPIO, chuẩn bị bộ đếm và xóa pending GPIO/NVIC. */
NVIC_EnableIRQ(EXTI1_IRQn);
__enable_irq();
```

Khi thoát testcase, tắt NVIC và nguồn ngắt ngoại vi, xóa pending rồi gọi
`__set_PRIMASK(saved_primask)` để khôi phục trạng thái ban đầu. Các helper
critical section trong driver lưu/khôi phục PRIMASK; chúng không tự mở khóa
ngắt toàn cục cho ứng dụng.

Nếu nhấn `i` chỉ thấy `[INFO] Injecting a GPIO interrupt event.` mà bộ đếm
không tăng, kiểm tra `__get_PRIMASK()`: giá trị `1` nghĩa là CPU vẫn khóa
ngắt. Giữ ISR ngắn: đọc/xóa cờ và tăng bộ đếm; in `printf` ở vòng lặp chính
vì console có thể đang bận khi ngắt xảy ra.

## Build và cấu hình

```sh
cd /home/quang-hoang/github_neon_mdf/CDC_NEON_MDF/examples/FPGA_Terminal
cmake --preset FU2
cmake --build --preset FU2
```

Đổi clock, kích thước dòng và RX/TX timeout nhận lệnh/testcase:

```sh
cmake --build --preset FU2 --target menuconfig
cmake --preset FU2
cmake --build --preset FU2
```

Giữ driver UART bật. Clock phải khớp PDI đang dùng; timeout tính theo số vòng
poll, không phải mili-giây. Lệnh `printf` đầu tiên tự khởi tạo UART0 theo mặc
định SDK, giống ví dụ `Printf_Log`. Terminal lấy handle qua
`neon_console_get_uart()` rồi dùng trực tiếp cho nhận lệnh và testcase.
`main()` không cấu hình/config UART riêng và không cần gọi `neon_console_attach_uart`.
Console dùng baud 19200 và timeout mặc định cho từng byte `printf`.
RX timeout áp dụng cho nhận ký tự ở prompt. Testcase GPIO kiểm tra RX không
chờ để tiếp tục đọc nút.
Mục `UART_ECHO_Polling testcase` trong menuconfig có RX/TX timeout riêng cho
testcase (`CONFIG_UART_ECHO_RX_TIMEOUT_POLLS`, `CONFIG_UART_ECHO_TX_TIMEOUT_POLLS`).

## Nạp và mở terminal

Nạp PDI phù hợp trước. UART0 phải được wrapper nối tới FTDI hoặc USB-UART
ngoài theo pin map của PDI; firmware này không thay đổi chân header FPGA.

Với profile `normal`, file để nạp BootROM v5 là
**`build/FU2/fpga_terminal_image.bin`**, gồm header 128 byte và payload.
Profile `xip` tạo **`fpga_terminal_image_xip.bin`** cho boot từ QSPI Flash.
Hai file `fpga_terminal.bin` và `fpga_terminal_xip.bin` chỉ chứa payload raw.
Chọn XIP bằng `cmake --preset FU2 --fresh -DNEON_MDF_PROFILE=xip` rồi build.
Xem [FU2 build/boot contract](../../guide/FU2.md).

Trên máy đang nối FPGA, thay serial port và đường dẫn image theo máy đó:

```sh
python3 uart_boot_loader.py \
  --port /dev/ttyUSB2 \
  --image /home/quang-hoang/github_neon_mdf/CDC_NEON_MDF/examples/FPGA_Terminal/build/FU2/fpga_terminal_image.bin \
  --baud 9600 --byte-gap-ms 2 --no-monitor
```

Reset theo quy trình board để loader truyền image. Baud BootROM của lệnh
trên là 9600, độc lập với baud ứng dụng; dùng giá trị khớp PDI thực tế.
Sau khi loader đóng cổng, mở serial terminal ở 19200 baud, 8N1, không local
echo và không hardware flow control. Có thể dùng:

```sh
python3 -m serial.tools.miniterm /dev/ttyUSB2 19200 --raw
```

Thoát miniterm bằng `Ctrl+]`. Banner có thể đã truyền trước khi mở monitor;
nhấn Enter để nhận prompt, sau đó nhập `help`.

Buffer RX và buffer dòng sở hữu đầy đủ các word căn chỉnh; thao tác ký tự
dùng `memcpy`, `memset` và các hàm chuỗi chuẩn do runtime FU2 cung cấp.
Build tự kiểm tra byte/halfword access
trong driver và firmware. Build thành công chưa xác nhận UART chạy trên FPGA.
