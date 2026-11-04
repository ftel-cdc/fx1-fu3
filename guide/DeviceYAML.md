# Khai báo `mcu-description/devices/<TARGET>.yaml`

Tham chiếu đầy đủ từng field khi viết file mô tả MCU — nguồn sự thật duy
nhất mà toàn bộ header/linker script/startup/`.svd` được sinh ra từ đó (xem
`ARCHITECTURE.md` mục 1-2). File này giải thích **field nào, ý nghĩa gì,
giá trị hợp lệ ra sao** — muốn biết quy trình đầy đủ thêm 1 target mới
(đăng ký CMake, driver, bootloader...) xem `guide/AddNewMCU.md`.

Nguồn schema thật (đây là tài liệu **diễn giải** — sai lệch với 2 file dưới
thì 2 file dưới là đúng): `mcu-description/schema/common.schema.yaml` (field
chung mọi kiến trúc) + `mcu-description/schema/arch/<arch>.schema.yaml`
(field riêng theo kiến trúc). `loader.py` chỉ validate field **bắt buộc**
(raise lỗi rõ ràng nếu thiếu) — field tuỳ chọn có giá trị mặc định ghi rõ
trong từng mục dưới.

## Khung file tối thiểu

```yaml
device: MY_CHIP
vendor: MyVendor
family: MY_CHIP
arch: cortex-m4          # hoặc riscv-rv32ec, ...
description: >
  Mo ta ngan chip nay.

cpu:
  core: cortex-m4
  revision: r0p1
  max_freq_hz: 100000000
  # + field rieng arch, xem muc "cpu:" ben duoi

memory:
  - {name: FLASH_MEM, kind: flash, address: 0x08000000, size: 0x80000}
  - {name: RAM,       kind: ram,   address: 0x20000000, size: 0x40000}

peripherals:
  - name: RCC
    base: 0x40023800
    registers: [...]

interrupts:
  - {number: 0, name: WWDG, description: "..."}
```

## Field cấp gốc

| Field | Bắt buộc | Ý nghĩa |
|---|---|---|
| `device` | Có | Tên chip cụ thể, vd `WEACT_F412`, `CH32V003F4P6` — dùng làm tên file header/linker/startup sinh ra (`<device>.h`, `startup_<device>.s`...) và giá trị `NEON_MDF_TARGET`. |
| `vendor` | Có | Tên nhà sản xuất/team, vd `STMicroelectronics`, `WCH`. Chỉ mô tả — RISC-V dùng thêm để tính đường dẫn `arch/RISCV/Device/<vendor>/<family>/` (xem `cmake/mcu_variants.cmake`'s `NEON_MDF_TARGET_<TARGET>_VENDOR`), Cortex-M không dùng field này để đặt tên thư mục. |
| `family` | Có | Tên họ chip, vd `WEACT_F412`, `CH32V003`. Nhiều `device` cùng `family` dùng chung 1 thư mục `arch/*/Device/<family>/` và dùng lại nguyên `components/driver` (dispatch theo family, xem `guide/AddNewMCU.md`). Thường trùng `device` nếu chip chưa có biến thể nào khác. |
| `arch` | Có | 1 trong: `cortex-m0`, `cortex-m0plus`, `cortex-m3`, `cortex-m4`, `riscv-rv32ec` (đã dùng thật, WCH QingKe V2A) — giá trị bắt đầu bằng `cortex-m`/`riscv` quyết định backend codegen nào chạy (`generate_cortex_m()`/`generate_riscv()` trong `generate.py`) và schema `arch/*.schema.yaml` nào áp dụng cho `cpu:` bên dưới. |
| `description` | Không | Mô tả ngắn, chỉ để đọc, không ảnh hưởng codegen. |

## `cpu:`

Field chung (mọi kiến trúc):

| Field | Bắt buộc | Ý nghĩa |
|---|---|---|
| `core` | Có | Tên lõi CPU, vd `cortex-m4`, `qingke-v2a`. |
| `revision` | Không (default rỗng) | Vd `r0p1` — ghi `"unknown"` nếu chưa xác nhận được, đừng bỏ trống gây hiểu nhầm là đã tra cứu. |
| `max_freq_hz` | Có | Tần số tối đa (Hz) — chỉ mô tả, không tự áp vào `SystemInit()` (clock thật cấu hình tay trong `system_<TARGET>.c`, xem `guide/AddNewMCU.md` bước 3). |

Field riêng theo `arch` (đọc vào `cpu.extra` dict tự động — **không cần sửa
`model.py`/`loader.py`** khi thêm field arch-specific mới, xem comment đầu
`riscv.schema.yaml`):

**Cortex-M** (`schema/arch/cortex-m.schema.yaml`):

| Field | Bắt buộc | Ý nghĩa |
|---|---|---|
| `nvic_priority_bits` | Có (nếu dùng FreeRTOS port CM4F) | Số bit priority NVIC thật (thường 4 trên STM32). Sai giá trị này làm sai toàn bộ priority scheme của FreeRTOS (`configPRIO_BITS` phải khớp) — xác nhận qua datasheet thật, không đoán. |
| `num_irq` | Có | Số IRQ ngoại vi (NVIC hỗ trợ tối đa 32 trên Cortex-M0/M0+, IRQ0..IRQ31) — quyết định độ dài phần "IRQ0.." của vector table sinh ra. **Field NÀY cũng bắt buộc trên RISC-V nếu device có khai `bootloader:`** (xem cảnh báo trong mục `bootloader:` bên dưới) dù không nằm trong danh sách field chính thức của `riscv.schema.yaml` — `loader.py`'s `_check_bootloader_vtor_alignment()` đọc `cpu.num_irq` KHÔNG ĐIỀU KIỆN theo `arch`, raise lỗi rõ ràng nếu thiếu. Trên RISC-V, hiểu là "tổng số slot vector table thật" (vd CH32V003: entry cao nhất PFIC dùng là 38 → `num_irq: 39`), không phải khái niệm NVIC IRQ0.. của Cortex-M. |
| `vtor_supported` | Có | Cortex-M0 thuần (ARMv6-M) không có VTOR chuẩn — nếu `false`, vector table BẮT BUỘC ở địa chỉ `0x00000000` (không dùng được layout bootloader+app, vì app không thể đặt vector table ở địa chỉ khác 0). |
| `mpu_present` | Không (default `false`) | Có MPU phần cứng không — xác nhận qua macro `__MPU_PRESENT` trong CMSIS header gốc của vendor, không suy từ tên core. |
| `fpu_present` | Không (default `false`) | Chỉ có ý nghĩa với `cortex-m4` — `true` sinh macro `__FPU_PRESENT` **và** cần tự thêm `-mfloat-abi=hard -mfpu=fpv4-sp-d16` trong `cmake/arch/cortex-m.cmake` để thật sự dùng phần cứng (khai `true` không tự động bật cờ compile). |

Vector table Cortex-M **không cần khai trong YAML** — cố định theo kiến
trúc ARMv6-M/ARMv7-M (SP/Reset/NMI/HardFault/.../PendSV/SysTick rồi tới
IRQ0..), `gen_startup_vector.py` tự sinh đúng layout, chỉ `interrupts:`
(mục dưới) điền tên cho phần IRQ.

**RISC-V** (`schema/arch/riscv.schema.yaml`, đã triển khai thật cho WCH
QingKe V2A — core khác có thể cần giá trị `mtvec_mode`/`interrupt_controller`
khác):

| Field | Bắt buộc | Ý nghĩa |
|---|---|---|
| `isa` | Có | Vd `rv32ec` (Embedded+Compressed, KHÔNG M/A/F trên CH32V003 — dùng đúng ISA THẬT của silicon, không phải ISA "muốn dùng"; dùng `rv32imac` cho chip không có nhân/chia phần cứng sẽ khiến compiler phát lệnh `mul`/`div` không tồn tại, lỗi chỉ lộ ra khi chạy thật). |
| `mtvec_mode` | Có | Cách CPU dùng thanh ghi `mtvec` để định vị vector table — WCH QingKe dùng `vectored-address` (giá trị vendor-specific, `mtvec[1:0]=3`: mỗi entry là 1 con trỏ hàm, entry 0 là LỆNH `j` chứ không phải địa chỉ, khác `"direct"`/`"vectored"` chuẩn RISC-V cơ bản `mtvec[1:0]=0/1`). Core RISC-V khác cần đọc kỹ tài liệu core đó trước khi điền. |
| `interrupt_controller` | Có | Tên bộ điều khiển ngắt thật, vd `pfic` (WCH, vendor-specific — KHÔNG PHẢI CLINT/PLIC chuẩn RISC-V). Core-private, KHÔNG khai trong `peripherals:` (giống NVIC của Cortex-M) — địa chỉ hardcode trong backend codegen (`gen_startup.py`) + `arch/RISCV/Port/`. |
| `hart_count` | Có | Số hart (lõi RISC-V logic) — `1` cho mọi chip đơn nhân hiện có trong SDK. |
| `num_irq` | Có, NẾU có `bootloader:` | KHÔNG nằm trong field "chính thức" của RISC-V nhưng `loader.py` vẫn đòi hỏi khi device khai `bootloader:` (xem bảng field chung `cpu:` ở trên + mục `bootloader:` bên dưới) — thiếu sẽ raise lỗi ngay lúc `generate.py` chạy, không phải lỗi âm thầm. |

## `memory:`

Danh sách vùng nhớ — dùng để sinh `MEMORY {}` của linker script.

```yaml
memory:
  - name: FLASH_MEM
    kind: flash
    address: 0x08000000
    size: 0x80000
  - name: RAM
    kind: ram
    address: 0x20000000
    size: 0x40000
```

| Field | Bắt buộc | Ý nghĩa |
|---|---|---|
| `name` | Có | Tên vùng nhớ — dùng làm tên `#define <name>_BASE` trong header sinh ra. |
| `kind` | Có | `flash` \| `rom` \| `ram`. Generator chọn vùng `flash`/`ram` **địa chỉ thấp nhất** làm mặc định cho code/data (`_pick_flash()`/`_pick_ram()` trong backend, xem `ARCHITECTURE.md` mục 2) nếu có nhiều vùng cùng loại. |
| `address` | Có | Địa chỉ bắt đầu thật. |
| `size` | Có | Kích thước (byte) thật. |

**Bẫy thật đã gặp**: nếu chip có 1 peripheral (mục dưới) CŨNG tên `FLASH`
(flash controller, cần cho bootloader), **đặt tên vùng nhớ flash khác đi**
(vd `FLASH_MEM`, đã dùng ở cả `WEACT_F412.yaml` lẫn `CH32V003F4P6.yaml`) —
trùng tên với `peripherals:[].name: FLASH` sinh 2 `#define FLASH_BASE` đè
lên nhau, lỗi khó nhận ra ngay lúc build.

## `peripherals:`

Danh sách peripheral — dùng để sinh struct C kiểu CMSIS + `.svd`.

```yaml
peripherals:
  - name: RCC
    base: 0x40021000
    description: "Reset and Clock control - RM chuong 3"
    registers:
      - {name: APB2PCENR, offset: 0x18, size: 32, access: rw, reset: 0x00000000, description: "..."}
```

| Field | Bắt buộc | Ý nghĩa |
|---|---|---|
| `name` | Có | Tên peripheral, vd `USART1`. Trở thành `#define <name> ((<type>_TypeDef*)<name>_BASE)` trong header — đây là tên app code thật sự dùng (`USART1->DATAR = ...`). |
| `base` | Có | Địa chỉ base thật. |
| `type` | Không (default = `name`) | **Nhóm dedup struct C** — nhiều peripheral cùng `type` sinh CHUNG 1 struct (`<type>_TypeDef`), chỉ khác `#define <name>_BASE`. Dùng khi nhiều instance giống hệt layout thanh ghi (vd `GPIOA`/`GPIOC`/`GPIOD` cùng `type: GPIO`) — tránh generator in trùng struct giống hệt nhau N lần. |
| `description` | Không | Mô tả, nên trích chương/mục Reference Manual thật. |
| `registers` | Có (list, có thể rỗng) | Xem mục dưới. |

**Mẫu dùng chung layout qua YAML anchor** (khi nhiều peripheral cùng
`type`, không lặp lại danh sách `registers` — xem `CH32V003F4P6.yaml`
thật):

```yaml
peripherals:
  - name: GPIOA
    base: 0x40010800
    type: GPIO
    registers: &gpio_regs
      - {name: CFGLR, offset: 0x00, size: 32, access: rw, reset: 0x44444444, description: "..."}
      - {name: INDR,  offset: 0x08, size: 32, access: ro, reset: 0x00000000, description: "..."}
  - {name: GPIOC, base: 0x40011000, type: GPIO, description: "...", registers: *gpio_regs}
  - {name: GPIOD, base: 0x40011400, type: GPIO, description: "...", registers: *gpio_regs}
```

### `registers:`

| Field | Bắt buộc | Ý nghĩa |
|---|---|---|
| `name` | Có | Tên thanh ghi, vd `CTLR1`. Trở thành field struct C (`USART1->CTLR1`). |
| `offset` | Có | Offset (byte) tính từ `base` của peripheral. |
| `size` | Không (default `32`) | `8` \| `16` \| `32` — độ rộng thanh ghi (bit), quyết định kiểu C (`uint8_t`/`uint16_t`/`uint32_t`) trong struct sinh ra. |
| `access` | Không (default `rw`) | `ro` \| `wo` \| `rw` — chỉ dùng cho `.svd`/tài liệu, generator KHÔNG chặn ghi vào thanh ghi `ro` ở mức C (không có `const` tự động). |
| `reset` | Không (default `0x0`) | Giá trị reset thật — chỉ dùng cho `.svd` (debugger hiện giá trị mặc định), không ảnh hưởng runtime. |
| `description` | Không | Nên trích chương/mục Reference Manual thật. |
| `fields` | Không (default rỗng) | Danh sách bitfield chi tiết (`name`/`bit_offset`/`bit_width`/`description`) — chỉ cần khi có tài liệu bitfield chi tiết VÀ thật sự cần (đa số driver trong SDK này tự định nghĩa `#define <REG>_<BIT> (1u << N)` ngay trong file `.c` thay vì dựa vào `fields:`, xem `platform/ch32v003/neon_gpio.c`/`platform/ch32v003/neon_flash.c` làm ví dụ — để trống là bình thường, không phải thiếu sót). |

## `interrupts:`

```yaml
interrupts:
  - {number: 12, name: SysTick, description: "System timer interrupt - THAT, RM 6.3"}
```

| Field | Bắt buộc | Ý nghĩa |
|---|---|---|
| `number` | Có | Số IRQ thật. Cortex-M: `0..(cpu.num_irq-1)`, tương ứng `IRQ0..IRQ(num_irq-1)` trong vector table chuẩn ARMv6-M/v7-M (xem bảng cố định ở mục `cpu:` Cortex-M trên). RISC-V/PFIC: số slot thật trong vector table vendor (vd CH32V003: 2-3=NMI/HardFault cố định theo kiến trúc không khai ở đây, 4 trở lên là số thật của từng IRQ, xem `gen_startup.py`). |
| `name` | Có | Sinh ra `<name>_IRQn` (enum `IRQn_Type`) + `<name>_IRQHandler` (weak handler mặc định trong startup, override bằng cách định nghĩa hàm cùng tên). |
| `description` | Không | Nên trích chương/mục Reference Manual thật. |

Không cần khai **mọi** IRQ chip có — chỉ khai IRQ nào SDK/app thật sự dùng
tới (STK/SysTick, USART cho bootloader...); khe còn lại generator tự điền
`Reserved`/`0` trong vector table, không lỗi.

## `bootloader:` (tuỳ chọn)

Chỉ khai khi device build theo mô hình bootloader+app (xem `ARCHITECTURE.md`
mục 7). Có field này → generator sinh thêm `<DEVICE>_BOOTLOADER.ld` +
`<DEVICE>_APP.ld` + macro `NEON_MDF_APP_BASE`/`APP_VECTOR_BASE`/`APP_SIZE`,
và `NEON_MDF_BOOTLOADER_ENABLED=ON` trong CMake.

```yaml
bootloader:
  size: 0x8000
  image_header_size: 0x200
  uart:
    peripheral: USART1
    tx_pin: PA9
    rx_pin: PA10
    af: 7
    baud: 115200
```

| Field | Bắt buộc | Ý nghĩa |
|---|---|---|
| `size` | Có | Kích thước partition bootloader (byte), tính từ đầu vùng flash thấp nhất. |
| `image_header_size` | Có | Kích thước "khe" header trước vector table của app (byte) — **PHẢI đủ lớn** để thoả yêu cầu align: bội số của 2^n ≥ `4*(16+cpu.num_irq)`, tối thiểu `0x80` — `loader.py` tự tính và raise lỗi rõ ràng nếu không thoả (`_check_bootloader_vtor_alignment()`), không cần tự tính tay. Check nay áp dụng **KHÔNG ĐIỀU KIỆN theo `arch`** (chạy cho mọi device có `bootloader:`, kể cả RISC-V không có VTOR thật — công thức mượn nguyên từ ARMv7-M nhưng vẫn dùng để đảm bảo `image_header_size` đủ lớn nói chung) → **`cpu.num_irq` PHẢI khai dù đang viết YAML cho RISC-V** (xem mục `cpu:` trên). KHÔNG đặt `image_header_size` đúng bằng `sizeof(image_header_t)` thật (16 byte) — luôn lớn hơn nhiều vì lý do align. |
| `uart.peripheral` | Có | Tên trong `peripherals:[].name` dùng làm UART nhận firmware. |
| `uart.tx_pin`/`rx_pin` | Có | Vd `PA9`/`PA10` — chỉ mô tả, driver UART thật (`neon_bootloader_uart_<family>.c`) tự cấu hình GPIO bằng tay, không đọc field này để sinh code. |
| `uart.af` | Có | Alternate function number (Cortex-M) — `0` nếu chip không cần AFIO remap (vd CH32V003 dùng mapping mặc định). |
| `uart.baud` | Có | Vd `115200`. |

## `erase_sectors:` (tuỳ chọn)

Chỉ cần nếu app của device này dùng `flashmap.csv` (OTA, xem
`ARCHITECTURE.md` mục 8) — dùng để validate mỗi partition rơi đúng ranh
giới sector erase thật. Không khai = generator bỏ qua validate này (an
toàn nếu app không dùng flashmap.csv).

```yaml
erase_sectors:
  - {size: 0x4000, count: 4}    # 4 sector 16KB
  - {size: 0x10000, count: 1}   # 1 sector 64KB
  - {size: 0x20000, count: 3}   # 3 sector 128KB
```

Dạng run-length (nhóm sector CÙNG kích thước liên tiếp), liệt kê ĐÚNG thứ
tự địa chỉ tăng dần từ đầu vùng flash. **Tổng `size*count` của mọi nhóm
PHẢI khớp CHÍNH XÁC kích thước vùng flash thấp nhất trong `memory:`** —
`loader.py` raise lỗi rõ ràng nếu lệch (bắt lỗi gõ nhầm/thiếu sector sớm,
trước khi ảnh hưởng tới flash thật).

Chip có sector đều nhau (vd CH32V003: 16 sector 1KB) chỉ cần 1 dòng
`{size: 0x400, count: 16}`.

## Quy tắc PLACEHOLDER vs số liệu thật

Nếu chỉ có product brief (không có Technical Reference Manual mức thanh
ghi) như `FU2.yaml` hiện tại — vẫn viết được YAML để pipeline chạy hết
(build ra `.elf` thật), nhưng **PHẢI đánh dấu rõ trong comment** đâu là số
liệu đoán/tạm (xem `FU2.yaml` làm mẫu) — không được để lẫn lộn với số liệu
đã xác nhận thật khiến người đọc sau tưởng nhầm là đã tra cứu. Không nạp
firmware build từ YAML còn placeholder lên silicon thật.

Mọi field có ghi "THAT"/"THẬT" trong comment của các file `.yaml` hiện có
(`WEACT_F412.yaml`, `CH32V003F4P6.yaml`) nghĩa là đã đối chiếu trực tiếp với
Reference Manual/datasheet chính thức của vendor — giữ đúng quy ước comment
này khi thêm field mới, để phân biệt rõ với phần còn đoán.
