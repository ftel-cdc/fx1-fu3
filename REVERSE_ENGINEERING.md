# Phân tích kiến trúc firmware và tài liệu bàn giao neon-mdf

Tài liệu này được xây dựng từ mã nguồn của repository tại thời điểm
2026-08-11. Các kết luận được phân biệt như sau:

- **Sự thật:** thể hiện trực tiếp trong mã nguồn hoặc cấu hình.
- **Tuyên bố của dự án:** được tài liệu dự án ghi nhận nhưng chưa được kiểm
  chứng lại trong môi trường hiện tại.
- **Suy luận:** kết luận kỹ thuật hợp lý rút ra từ cách triển khai.

## 1. Kết luận tổng quan

`neon-mdf` không phải một firmware sản phẩm duy nhất. Đây là một SDK theo phong
cách ESP-IDF dành cho vi điều khiển nhỏ, bao gồm:

- Bộ mô tả MCU bằng YAML và trình sinh mã.
- Hệ thống build CMake/Kconfig.
- Startup, linker script và lớp trừu tượng ngắt cho Cortex-M và RISC-V.
- Driver dùng API chung nhưng có implementation riêng theo từng họ chip.
- Bootloader UART, đóng gói image và cơ chế OTA/rollback.
- FreeRTOS, các ứng dụng mẫu, CLI và extension VS Code.

Ba target hiện có:

| Target | CPU | Flash/RAM | Mức độ tin cậy hiện tại |
|---|---|---:|---|
| `FU2` | Arm Cortex-M0, tối đa 60 MHz | 512 KiB / 128 KiB | Địa chỉ thanh ghi và UART/I2C/GPIO còn là placeholder; không an toàn để chạy trên silicon |
| `WEACT_F412` | STM32F412RET6, Cortex-M4F | 512 KiB / 256 KiB | Được dự án ghi nhận đã kiểm thử phần cứng; test rig chính |
| `CH32V003F4P6` | WCH QingKe V2A, RV32EC | 16 KiB / 2 KiB | Được ghi nhận đã build/link; chưa kiểm chứng trên board thật |

Repository này không chứa Cortex-A, Kria K26, Zynq MPSoC, GIC, MMU, DDR,
Linux, AXI hay giao tiếp PL. Kiến trúc thực tế là bare-metal Cortex-M/PFIC,
tùy chọn chạy FreeRTOS.

Luồng kiến trúc trung tâm:

```text
mcu-description/devices/<TARGET>.yaml
       |
       +--> device header và register struct
       +--> startup code và vector table
       +--> linker script whole-flash/bootloader/application
       +--> debugger SVD
       +--> metadata CMake
                    |
application --> API driver chung --> implementation theo family --> MMIO
                    |
                    +--> FreeRTOS tùy chọn
                    +--> bootloader + OTA tùy chọn
```

Rủi ro kỹ thuật lớn nhất là phần cứng FU2 còn giả lập, OTA metadata không giao
dịch, flash driver không kiểm tra lỗi/timeout, các trường hợp biên về giới hạn
image trong bootloader, CRC không cung cấp tính xác thực và thiếu kiểm thử tự
động.

## 2. Phạm vi repository

Checkout hiện có 1.105 file. Phần lớn là CMSIS 5 và toàn bộ ma trận port của
FreeRTOS upstream. Nếu loại các thư viện vendor, output sinh tự động, cache và
gói extension, phần mã/cấu hình thuộc dự án có khoảng 181 file, 15.400 dòng.

```text
/
+-- README.md, ARCHITECTURE.md, SETUP.md, VSCODE.md
+-- guide/                       hướng dẫn cho developer
+-- examples/                    các project ứng dụng độc lập
+-- neon-mdf/
|   +-- CMakeLists.txt, Kconfig  composition của SDK
|   +-- arch/                    startup, linker, IRQ abstraction
|   +-- components/              driver, bootloader, OTA
|   +-- mcu-description/         YAML, schema và generator
|   +-- cmake/                   điều phối target/toolchain/build
|   +-- tools/                   CLI, image builder và template
|   +-- rtos/FreeRTOS-Kernel/    FreeRTOS vendor
+-- vscode-extension/            công cụ phát triển TypeScript
```

Các file startup/header/linker được sinh tự động không được sửa trực tiếp.
Nguồn sự thật là YAML, generator Python và file `system_<TARGET>.c` viết tay.

## 3. Kiến trúc build

Mỗi application là một project CMake độc lập. `CMakeLists.txt` của application
include `neon-mdf/tools/cmake/project.cmake`, sau đó SDK được thêm bằng
`add_subdirectory()`.

Thứ tự cấu hình quan trọng:

```text
mcu_variants.cmake       sinh/chọn device target và cờ kiến trúc
flashmap.cmake           sinh partition, bật OTA nếu có flashmap
kconfig.cmake            sinh sdkconfig.h và target menuconfig
components/driver        chọn implementation theo family
components/bootloader    cung cấp CRC, UART và source bootloader
application CMake        tạo một hoặc nhiều ELF
flash_targets.cmake      đóng gói và tạo target nạp firmware
```

Các CMake target chính:

| Target | Vai trò |
|---|---|
| `neon_mdf_device_<TARGET>` | Startup, system, IRQ port, header và CPU flags |
| `neon_mdf_kconfig` | Export include path của `sdkconfig.h` |
| `neon_mdf_driver` | GPIO và driver được cấu hình cho family |
| `neon_mdf_crc32` | CRC-32 phản chiếu, polynomial `0xEDB88320` |
| `neon_mdf_bootloader_uart` | UART polling dùng cho recovery |
| `neon_mdf_flashmap` | Export `flashmap.h` được sinh tự động |
| `neon_mdf_ota` | Đọc/ghi/xác nhận OTA metadata |
| `freertos_kernel[_port]` | Kernel và port FreeRTOS được chọn |

Kconfig là cơ chế composition lúc biên dịch, không phải registry lúc chạy.
`sdkconfig` được chuyển thành `sdkconfig.h`; CMake dùng các symbol để quyết định
source driver nào được compile.

### Trạng thái kiểm tra build

Đã thử `cmake --preset WEACT_F412` trong `examples/Blinky`. CMake nhận diện được
Arm GCC 10.2.1 nhưng dừng ở `mcu_variants.cmake` vì bắt buộc tìm executable tên
`python3`, trong khi `python3` chưa có trên `PATH`. Toolchain RISC-V cũng chưa có
trên `PATH`. Vì vậy tài liệu này không tuyên bố đã clean-build thành công trong
môi trường hiện tại.

## 4. Luồng reset và startup

### 4.1 Cortex-M

```text
hardware reset
  -> vector[0] nạp MSP = _estack
  -> vector[1] gọi Reset_Handler                  startup_<target>.s
  -> đặt SP
  -> copy .data từ flash sang RAM
  -> zero .bss
  -> SystemInit()                                 system_<target>.c
       -> M4F: mở quyền CP10/CP11 cho FPU
       -> SCB->VTOR = g_pfnVectors
       -> cập nhật SystemCoreClock
  -> main()                                       bootloader/application
```

Startup hiện không duyệt `.preinit_array`/`.init_array` và không gọi
`__libc_init_array()`. Điều này phù hợp với ứng dụng C hiện tại nhưng là ràng
buộc ẩn nếu thêm C++ hoặc thư viện dùng constructor. Nếu `main()` return, hành
vi không có contract hợp lệ; các `main()` hiện tại đều chạy vô hạn.

Handler yếu trỏ tới `Default_Handler`, là vòng lặp vô hạn. Trên Cortex-M4F,
FreeRTOS sử dụng tên handler direct-routing nên example phải nối rõ ràng
`SVC_Handler`, `PendSV_Handler` và `SysTick_Handler` sang port FreeRTOS.

### 4.2 CH32V003

```text
reset tại _start
  -> jump handle_reset; các word sau là PFIC address-mode table
  -> khởi tạo gp và sp
  -> copy .data, zero .bss
  -> đặt mstatus.MPIE
  -> ghi CSR WCH INTSYSCR 0x804 = 2
  -> mtvec = _start | 3, WCH Address mode
  -> SystemInit()
  -> mepc = main; mret
```

Việc dùng `mret` thay vì `jal main` thiết lập trạng thái ngắt machine-mode có
chủ đích. Khi chạy FreeRTOS, `vPortSetupTimerInterrupt()` đổi `mtvec` sang trap
handler Direct mode của FreeRTOS vì port generic cần tự giải mã `mcause`.

### 4.3 Linker và memory layout

```text
FLASH: vector/.init -> .text -> .rodata -> unwind -> load image của .data
RAM:   .data -> .bss -> vùng dự phòng heap/stack
đỉnh RAM: _estack
```

Linker script WEACT whole-flash dành 0x400 byte cho heap và 0x400 byte cho
stack. Đây chỉ là reservation ở linker, không phải implementation allocator.
App và bootloader có FLASH origin/length khác nhau nhưng dùng cùng RAM vật lý vì
không chạy đồng thời.

Không có MPU/MMU, cache maintenance, DMA buffer đặc biệt, memory barrier policy
hoặc dynamic allocation trong driver/bootloader của SDK.

## 5. Mô hình thực thi application

### Bare-metal Blinky

`main()` bật clock GPIO, cấu hình một pin output rồi lặp: toggle, busy delay,
in log tùy target và xác nhận OTA sau mười vòng. Delay được tính bằng số vòng
lặp, không phải đơn vị thời gian; nó phụ thuộc compiler và clock.

### FreeRTOS Blinky

`main()` cấu hình hai pin, tạo hai task cùng priority rồi gọi scheduler. Task 0
toggle mỗi 200 ms; trên WEACT OTA nó xác nhận firmware sau khoảng 4 giây. Task 1
toggle mỗi 500 hoặc 250 ms tùy version.

Hai task dùng pin khác nhau, nhưng `neon_gpio_toggle_pin()` vẫn là chuỗi đọc
ODR/OUTDR rồi quyết định set/reset. API này không atomic nếu task/ISR cùng thay
đổi mask chồng lấn.

Không có lệnh `xTaskCreate()` nào kiểm tra return value. Khi thiếu heap, một
task có thể không được tạo mà ứng dụng vẫn tiếp tục. Stack overflow hook chỉ
tắt ngắt và treo, không log hoặc reset.

## 6. Bootloader và OTA

### 6.1 Định dạng image

```text
partition base
+0x00  image_header_t: magic, version, size, crc32 — 16 byte
+0x10  padding chưa dùng
+image_header_size  vector/startup và payload application
```

Khoảng header dự phòng lớn hơn struct 16 byte để thỏa alignment của vector/VTOR.
CRC được tính trên đúng `size` byte payload. `version` được lưu nhưng bootloader
không kiểm tra. CRC chỉ phát hiện lỗi ngẫu nhiên; không chống giả mạo, replay
hoặc downgrade.

### 6.2 Timeline boot

```text
Reset_Handler -> SystemInit -> bootloader main
  -> khởi tạo polling UART
  -> bật counter phần cứng 1 ms
  -> chờ byte đầu tối đa 3000 ms
       0x55: vào update mode
       byte khác/timeout: chọn image bình thường
  -> không flashmap: kiểm tra header và jump app cố định
  -> có flashmap: đọc otadata, CRC image, tăng attempt/rollback, jump slot
```

Protocol update:

```text
SYNC 0x55 -> READY 'R' -> header 16 byte -> ACK/NAK
-> các chunk 256 byte, ACK từng chunk
-> CRC toàn image -> ACK/NAK cuối
```

ACK của header chỉ được gửi sau erase và program header. Đây là thiết kế đúng
để host không gửi payload trong lúc CPU đang block vì erase flash.

Sau khi đã vào update mode, mọi UART receive đều có thể block vô hạn. Không có
timeout giao dịch, sequence number, CRC từng chunk, retry, abort hoặc watchdog.

### 6.3 Chuyển quyền sang application

WEACT đọc MSP/reset PC từ vector application, ghi VTOR, nạp MSP rồi gọi reset
address. Code hiện không kiểm tra rõ MSP có nằm trong RAM, reset PC có nằm trong
partition, alignment và Thumb state.

Handoff production-grade nên tắt SysTick/ngắt, clear NVIC enable/pending, khôi
phục CONTROL/BASEPRI/PRIMASK, kiểm tra MSP/PC, dùng DSB/ISB quanh VTOR và branch
không tạo C call frame.

### 6.4 State machine OTA

```text
otadata không hợp lệ ----------------------> boot factory

cài OTA thành công
  -> ghi {slot=OTA, PENDING, attempts=0}
  -> reset

PENDING boot
  -> image hỏng -------------> ghi factory VALID -> boot factory
  -> attempts++
       attempts <= 3 --------> boot OTA
           app confirm ------> ghi OTA VALID
           crash/reset ------> lần PENDING tiếp theo
       attempts > 3 ---------> ghi factory VALID -> boot factory

OTA VALID + image hợp lệ -----------------> luôn boot OTA
```

Mô hình bất đối xứng: OTA chỉ ghi slot OTA, không đụng factory. Ưu điểm là luôn
có recovery image; nhược điểm là không thể cập nhật factory từ xa.

Mỗi thay đổi metadata erase cả sector và ghi một record 20 byte. Mất nguồn
trong lúc ghi làm CRC metadata sai; bootloader fallback về factory. Cách này
fail-safe cho khả năng boot nhưng không bảo toàn trạng thái theo giao dịch,
không có record dự phòng, sequence, commit marker hoặc wear-leveling.

`neon_ota_confirm_valid()` không xác minh `active_slot == OTA`, state thực sự là
PENDING, hoặc PC hiện chạy trong slot được chọn. `neon_ota_read_data()` và
`WriteData()` cũng không kiểm tra con trỏ null.

## 7. Phân tích module

### GPIO

API gồm clock enable, init input/output, read, write và toggle. Driver không lưu
state; persistent state nằm trong thanh ghi peripheral.

- WEACT: RCC AHB1, MODER/IDR/ODR/BSRR.
- CH32: APB2, CFGLR/INDR/OUTDR/BSHR/BCR.
- FU2: DIR/DATA/SET/CLR placeholder; clock enable là no-op.

Set/reset bằng BSRR/BSHR/BCR là atomic ở peripheral. Toggle không atomic do cần
đọc trạng thái trước. Mode không hợp lệ bị xử lý như input; bit pin ngoài phạm
vi bị bỏ qua im lặng.

### Flash

WEACT dùng bảng tám sector STM32F4 không đều và program word 32 bit ở giả định
VDD 3,3 V. CH32 dùng sector 1 KiB và tách một word thành hai lần program
halfword.

Cả hai implementation đều:

- Poll BUSY vô hạn, không timeout.
- Không kiểm tra lỗi program/erase/protection/alignment.
- Không read-back dữ liệu.
- Để flash ở trạng thái unlock.
- Không kiểm tra address/range và overflow `address + size`.
- Trả về `void`, nên caller không thể phục hồi lỗi.
- Không reentrant và không an toàn trong ISR.

### UART và I2C công khai

Đây là driver polling placeholder dành cho FU2. Register map, divider và status
bit chưa dựa trên TRM thật. `Timeout` là số vòng poll cho từng byte/giai đoạn,
không phải millisecond hoặc deadline toàn transaction. Chưa có pinmux, clock,
reset, error flag, bus recovery, interrupt hoặc DMA.

Bootloader UART là implementation riêng theo family để phục vụ recovery.
WEACT_F412 hiện không có syscall retarget `printf()` sang UART bootloader.

### IRQ abstraction

Cortex-M dùng NVIC/PRIMASK. RISC-V dùng PFIC và CSR WCH. Đây chỉ là API điều
khiển enable/disable/priority/critical section, không phải trình dispatcher;
tên handler vẫn đến từ vector table hoặc port FreeRTOS.

### Generator mô tả MCU

`model.py` định nghĩa object device/peripheral/register/interrupt. `loader.py`
parse YAML và kiểm tra schema, uniqueness, memory, bootloader sizing, erase
geometry và vector alignment. `generate.py` dispatch sang backend Cortex-M hoặc
RISC-V để sinh header, linker, startup, SVD và CMake fragment.

Đây là điểm có blast radius lớn nhất: một lỗi generator có thể ảnh hưởng mọi
target. Cần golden-output test và unit test cho validator.

### Công cụ host

- `gen_flashmap.py`: parse CSV, align partition, kiểm tra fit, sinh header và
  linker script.
- `build_ota_image.py`: ghép header và CRC với payload.
- `build_otadata_image.py`: tạo metadata ban đầu/chuyển slot.
- `build_full_image.py`: ghép bootloader, factory và metadata.
- `stm32_uart_flash_firmware.py`: giao tiếp STM32 ROM bootloader.
- `neon_mdf.py`: CLI điều phối build/flash/monitor/menuconfig.

### Extension VS Code

Extension chỉ chạy ở máy phát triển, không link vào firmware. Các module quản
lý target/transport/port, chạy CMake, đọc CMake File API, flash, debug OpenOCD,
menuconfig, serial monitor và tạo project. Rủi ro chính là quoting đường dẫn,
process lifecycle, stale build state và WCH flash/debug chưa được xác minh.

## 8. Luồng dữ liệu và quyền sở hữu

### Update firmware qua UART

```text
host .bin
 -> host tạo header/CRC
 -> serial
 -> UART DATA register
 -> buffer stack 16 byte / 256 byte
 -> flash controller ghi slot
 -> CPU đọc lại flash để tính CRC
 -> ghi OTA metadata
 -> reset -> chọn slot -> application
```

Không dùng heap. Buffer thuộc stack của `run_update_mode*()` và được tái sử dụng
từng vòng. Payload được ghi ngay; mất kết nối để lại slot OTA dở dang nhưng
factory không bị thay đổi.

### Xác nhận OTA

```text
health policy của app
 -> neon_ota_confirm_valid
 -> copy metadata flash vào stack
 -> kiểm tra magic/CRC
 -> đổi state/attempt
 -> erase metadata sector
 -> program năm word
 -> bootloader đọc ở lần reset sau
```

Application phải tự serialize mọi truy cập flash. SDK không có mutex hay cơ chế
bảo vệ nếu task/ISR khác cùng dùng flash.

## 9. Hệ thống ngắt

Bare-metal example không bật ngắt. Bootloader chỉ poll cờ counter, không bật
SysTick interrupt. Default handler treo vô hạn.

| Target | Tick | Context switch | Yield/service |
|---|---|---|---|
| FU2 M0 | `SysTick_Handler` của CM0 port | `PendSV_Handler` | `SVC_Handler` |
| WEACT M4F | wrapper gọi `xPortSysTickHandler` | tail-branch sang `xPortPendSVHandler` | tail-branch sang `vPortSVCHandler` |
| CH32 RV32EC | STK/PFIC IRQ 12 | trap gọi scheduler switch | machine `ecall` |

Không có ISR cho UART, I2C, GPIO, flash, DMA, watchdog hoặc timer ứng dụng. Trên
CH32, handler bỏ qua IRQ lạ mà không clear nguồn; một level interrupt bật nhầm
có thể gây interrupt storm.

## 10. Phát hiện kỹ thuật quan trọng

### Mức nghiêm trọng/cao

1. **FU2 chưa thể triển khai:** địa chỉ và hành vi peripheral còn placeholder.
2. **Không có secure boot:** bất kỳ image nào có CRC đúng đều được chấp nhận.
3. **Final chunk có thể vượt partition:** bootloader kiểm tra
   `size <= code_size` nhưng luôn program chunk cuối được làm tròn lên 256 byte.
   Nếu image gần đầy partition và không chia hết cho 256, phần padding có thể
   vượt giới hạn trong khi erase chỉ dùng kích thước chính xác. Phải kiểm tra
   `align_up(size, 256) <= code_size` hoặc chỉ ghi đến word cuối cần thiết.
4. **Lỗi flash bị che giấu:** mọi API trả `void` và có thể poll BUSY vĩnh viễn.
5. **Handoff application chưa đủ chặt:** thiếu kiểm tra SP/PC và dọn trạng thái
   timer/ngắt của core.

### Mức trung bình

6. OTA metadata chỉ có một record erase-sensitive; nên dùng hai record/page,
   sequence tăng dần và commit marker.
7. API OTA không kiểm tra null và không validate miền giá trị slot/state.
8. Wire format dùng trực tiếp C struct/little-endian; cần static assert kích
   thước/offset và serialize LE rõ ràng.
9. Update mode không có timeout sau SYNC hoặc cơ chế retry/abort.
10. GPIO toggle không thread-safe/ISR-safe.
11. Return value của `xTaskCreate()` bị bỏ qua.
12. Startup không chạy constructor C/C++.
13. `SystemCoreClockUpdate()` còn giả định hằng số; thay PLL có thể làm sai baud,
    tick và delay mà không báo lỗi.

### Điểm mạnh

- Phân lớp rõ giữa mô tả phần cứng, generated artifact, driver và application.
- Application dùng API chung; implementation được chọn lúc build.
- Bootloader architecture glue tách Cortex-M và WCH khỏi logic OTA chung.
- OTA bất đối xứng giữ factory làm recovery image ổn định.
- Comment lưu lại nhiều bài học phần cứng thực tế như thứ tự erase/ACK và handler
  routing của FreeRTOS M4F.

### Nợ kỹ thuật

- Comment quá dài, trộn lịch sử với contract hiện tại.
- Một số comment trỏ đến plan/memory nội bộ không có trong repository.
- Hằng số STK/PFIC CH32 bị lặp giữa bootloader và example FreeRTOS.
- Không thấy test suite, CI, simulator harness hay static analysis cho mã dự án.
- Generated file và gói VSIX/dist làm tăng nhiễu khi review nếu không có kiểm
  tra reproducibility.

## 11. Bản đồ file

| Nhóm file | Trách nhiệm |
|---|---|
| `README.md`, `ARCHITECTURE.md` | Tổng quan và lý do thiết kế |
| `SETUP.md`, `VSCODE.md`, `guide/*.md` | Môi trường, onboarding, YAML, driver, Kconfig, RTOS, OTA, debug |
| `neon-mdf/CMakeLists.txt`, `Kconfig` | Composition gốc của SDK |
| `cmake/toolchain/*.cmake` | Tìm compiler, objcopy, size và bare-metal specs |
| `cmake/arch/*.cmake` | Cờ CPU/ISA/FPU |
| `cmake/mcu_variants.cmake` | Registry target và gọi generator |
| `cmake/kconfig.cmake` | Sinh `sdkconfig.h`, target menuconfig |
| `cmake/flashmap.cmake` | Chọn CSV theo target và bật OTA |
| `cmake/flash_targets.cmake` | Đóng gói và flash JTAG/UART/OTA |
| `cmake/freertos.cmake` | Chọn port FreeRTOS theo kiến trúc |
| `tools/cmake/project.cmake` | Entry point cho project application |
| `mcu-description/schema/**` | Contract YAML |
| `mcu-description/devices/*.yaml` | Memory, peripheral, IRQ và boot policy |
| `mcu-description/tools/mcu_desc/*.py` | Object model, parser và validator |
| `mcu-description/tools/backends/**` | Sinh header/startup/linker/SVD |
| `mcu-description/tools/generate.py` | CLI và backend dispatcher |
| `arch/CortexM/**/startup_*.s` | Reset, data/BSS, vector và weak handler |
| `arch/CortexM/**/linker/*.ld` | Layout whole-flash/boot/app |
| `arch/CortexM/**/system_*.c` | FPU, VTOR và clock setup viết tay |
| `arch/CortexM/CMSIS/` | CMSIS-Core cung cấp NVIC và các intrinsic PRIMASK dùng trực tiếp |
| `arch/RISCV/Core/**` | CSR, soft-math và libc shim tối thiểu |
| `arch/RISCV/**/startup_*.s` | gp/sp, C runtime, CSR, PFIC và `mret` |
| `arch/RISCV/Port/sdk_irq.*` | PFIC và critical section |
| `components/driver/neon_types.h` | Status code dùng chung |
| `components/driver/gpio/*` | API GPIO và ba implementation |
| `components/driver/flash/*` | API flash và implementation WEACT/CH32 |
| `components/driver/uart/*`, `i2c/*` | Driver polling placeholder FU2 |
| `components/bootloader/image_header.h` | Header image 16 byte |
| `neon_bootloader_crc32.*` | CRC-32 không có global state |
| `neon_bootloader_uart.*` | UART recovery theo family |
| `neon_bootloader_arch.*` | Tick/reset/jump abstraction |
| `neon_bootloader_main.c` | Protocol, program image, chọn slot, rollback |
| `components/ota/neon_ota.*` | Metadata và xác nhận OTA |
| `tools/gen_flashmap.py` | Sinh partition/header/linker từ CSV |
| `tools/build_*image.py` | Đóng gói image và metadata |
| `tools/neon_mdf.py` | CLI build/flash/monitor |
| `tools/templates*/**` | Scaffold project bare-metal/FreeRTOS |
| `examples/Blinky/**` | GPIO/Kconfig/OTA portable demo |
| `examples/FreeRTOS_Blinky/**` | Scheduler và tích hợp port đặc thù |
| `examples/OTA_Demo/**` | Factory, OTA xác nhận và rollback cố ý |
| `vscode-extension/src/**` | UI, CMake, flash, debug, monitor và project wizard |

CMSIS và FreeRTOS-Kernel được xem là thư viện upstream. Các port không được chọn
không tham gia executable; phân tích từng file port của mọi CPU sẽ là review
FreeRTOS upstream, không phải reverse engineering firmware thực thi của dự án.

## 12. Call graph quan trọng

```text
Reset_Handler
+-- SystemInit
`-- main
    +-- bare-metal
    |   +-- neon_gpio_clock_enable
    |   +-- neon_gpio_init
    |   `-- loop -> TogglePin -> [tùy chọn] OTA_ConfirmValid
    +-- FreeRTOS
    |   +-- GPIO init
    |   +-- xTaskCreate x2
    |   `-- vTaskStartScheduler
    |       +-- SysTick/STK -> xTaskIncrementTick
    |       +-- PendSV/trap -> vTaskSwitchContext
    |       `-- task -> GPIO/delay/OTA confirm
    `-- bootloader
        +-- UART_Init
        +-- SystickStart1ms / TryRecvByte
        +-- update -> Recv -> Flash erase/program -> CRC
        |              `-- metadata -> reset hoặc JumpToApp
        `-- normal boot -> metadata -> CRC -> attempt/rollback -> JumpToApp
```

Flash, UART polling, CRC toàn image và busy delay đều blocking. API driver/boot
không được thiết kế để gọi từ ISR. CRC không dùng static/global state nên có thể
reentrant nếu vùng input không bị thay đổi đồng thời.

## 13. Thứ tự học dành cho maintainer mới

1. Đọc YAML của target, generated header, startup và linker script song song.
2. Nắm `project.cmake`, `mcu_variants.cmake` và `CMakeLists.txt` của application
   để biết source nào thực sự đi vào ELF.
3. Trace reset từ startup qua `SystemInit()` đến đúng `main()`.
4. Với application thường, học GPIO/IRQ implementation và FreeRTOS routing.
5. Trước khi sửa update, phải hiểu đồng thời `image_header.h`,
   `neon_bootloader_main.c`, flash driver, `neon_ota.c`, `gen_flashmap.py` và host
   image builder. Đây là một protocol thống nhất, không phải các file độc lập.

Thay đổi an toàn nhất là tài liệu, host tooling có test hoặc driver mới nằm sau
API cũ. Phải cực kỳ thận trọng với generator, startup, linker, erase geometry,
bootloader handoff, metadata layout và wire format: lỗi ở đây có thể build thành
công nhưng erase sai sector hoặc làm mất đường recovery.

Trước khi dùng production cần có golden test cho generator, test serialize
image/metadata, test state machine OTA trên host, giới hạn kích thước từ map
file, warnings-as-errors, static analysis, fault injection mất nguồn tại từng
điểm erase/program, fuzz UART malformed packet và board-in-loop test cho boot,
update, rollback trên từng target.
