# Khai báo và implement driver (`components/driver/`)

Hướng dẫn viết driver mới trong neon-mdf: thêm implementation cho 1 family
chưa có (vd GPIO/Flash cho chip mới), hoặc thêm hẳn 1 loại peripheral mới
(vd SPI, chưa từng có trong SDK) — và cách wire vào CMake để app dùng được.

Xem `ARCHITECTURE.md` mục 6 cho tổng quan thiết kế, `guide/ArchitectDetail.md`
cho đồ thị phụ thuộc CMake đầy đủ. File này đi sâu vào **cách viết**, không
lặp lại lý do thiết kế.

## Source format áp dụng cho Neon MDF

Áp dụng [NEON Coding Convention V1.3](NEON_Coding_Convention_V1.3.md) và
[hướng dẫn tích hợp Neon MDF](NEON_MDF_Source_Format_Guideline.md).

- Public HAL API dùng `neon_<module>_<action>()`, helper private dùng
  `static ... <module>_<action>()`, type dùng `_t`, field dùng `snake_case`.
- Dùng 4 spaces, ngoặc mở cùng dòng và ngoặc cho mọi block điều kiện/vòng lặp.
- Header guard dùng `__NEON_<MODULE>_H__`; giữ C++ guard cho hàm external.
- Header chung ở thư mục module, backend ở `platform/<family>/neon_<module>.c`.
- Type dùng chung ở `neon_types.h`; `neon_config.h` cung cấp module switches
  0/1 dựa trên Kconfig/sdkconfig. `neon.h` là entry header tùy chọn.
- UART/I2C tách `init(handle, instance)` và `set_config(handle, config)`.
  Driver sở hữu field handle và giữ bản sao cấu hình trong storage private.
- Giữ register type/device symbol, ISR và startup/runtime hook. Cortex-M dùng
  CMSIS trực tiếp; API `SDK_IRQ_*` thuộc Port RISC-V.
- Sửa header/startup/linker sinh từ YAML qua nguồn mô tả hoặc generator.
- Giữ ABI word-access FU2, compiler policy và `.ramfunc` của Flash/reset.
- CMSIS/FreeRTOS giữ quy ước upstream và patch nền tảng hiện có.

## Mô hình chung: 1 header, N implementation

Mọi peripheral trong `components/driver/` theo đúng 1 khuôn:

```
components/driver/<peripheral>/
  neon_<peripheral>.h          # API CHUNG - app include() đúng file này
  platform/<family>/neon_<peripheral>.c # implementation THẬT, 1 file/family
```

App gọi `neon_gpio_init()`/`neon_flash_erase_region()`... **giống hệt nhau**
trên mọi target — CMake chọn đúng file `.c` theo family lúc build
(`components/driver/CMakeLists.txt`), app không bao giờ `#ifdef` theo
family để gọi driver.

## 1. Viết header API chung (`neon_<peripheral>.h`)

Quy tắc bắt buộc:

- **Dùng tên kiểu THEO TYPE, không theo instance**: `GPIO_TypeDef*`, không
  phải `GPIOA_TypeDef*`/`GPIO0_TypeDef*` — struct này sinh tự động từ
  `mcu-description` (`peripherals:[].type`, xem `guide/DeviceYAML.md`), lấy
  qua `#include "mcu_device.h"` (alias cố định, không hardcode tên device).
  Nhờ vậy 1 file driver dùng được cho MỌI instance cùng layout thanh ghi
  (`GPIOA`/`GPIOC`/`GPIOD` đều truyền vào cùng 1 hàm) và cho device tương
  lai cùng family.
- Backend Cortex-M điều khiển IRQ qua CMSIS `NVIC_*`; critical section lưu
  PRIMASK rồi khôi phục đúng trạng thái trước đó. Backend RISC-V dùng
  `SDK_IRQ_*` từ `arch/RISCV/Port/Inc/sdk_irq.h` để điều khiển PFIC/CSR.
- Trả `neon_status_t` (`neon_types.h`: `NEON_OK`/`NEON_ERROR`/`NEON_BUSY`/
  `NEON_TIMEOUT`) cho hàm có thể thất bại — không bắt buộc cho hàm chắc
  chắn không lỗi (vd `WritePin`/`TogglePin` hiện trả `void`).
- Tên hàm `neon_<peripheral>_<action>` (snake_case), macro
  `NEON_<PERIPHERAL>_*`, type `neon_<peripheral>_<name>_t` — xem `neon_gpio.h`/
  `neon_flash.h` làm mẫu chính xác.

Ví dụ khung tối thiểu (peripheral mới, vd SPI):

```c
#ifndef __NEON_SPI_H__
#define __NEON_SPI_H__

#include "neon_types.h"
#include "mcu_device.h"

typedef struct {
    uint32_t clock_speed_hz;
    /* ... */
} neon_spi_config_t;

neon_status_t neon_spi_init(SPI_TypeDef *instance, const neon_spi_config_t *init);
neon_status_t neon_spi_transmit(SPI_TypeDef *instance, const uint8_t *data, uint32_t size_bytes,
                                uint32_t timeout_polls);

#endif /* __NEON_SPI_H__ */
```

`SPI_TypeDef` chỉ tồn tại nếu device YAML đang build có khai 1 peripheral
`type: SPI` (xem `guide/DeviceYAML.md` mục `peripherals:`) — nếu build cho
family chưa khai SPI trong YAML, code app gọi `neon_spi_*` sẽ lỗi **build**
rõ ràng (thiếu type), không phải lỗi ẩn lúc chạy.

## 2. Viết implementation cho 1 family (`platform/<family>/neon_<peripheral>.c`)

Thao tác thẳng qua struct thanh ghi thật (không bịa offset — đối chiếu
Reference Manual, xem `guide/DeviceYAML.md`). Ví dụ thật, rút gọn từ
`components/driver/gpio/platform/ch32v003/neon_gpio.c`:

```c
#include "gpio/neon_gpio.h"

neon_status_t neon_gpio_clock_enable(GPIO_TypeDef *instance) {
    /* RCC->APB2PCENR bit2=IOPAEN, bit4=IOPCEN, bit5=IOPDEN - THAT, RM 3.4.7. */
    if (instance == GPIOA) {
        RCC->APB2PCENR |= (1U << 2);
    } else if (instance == GPIOC) {
        RCC->APB2PCENR |= (1U << 4);
    } else if (instance == GPIOD) {
        RCC->APB2PCENR |= (1U << 5);
    } else {
        return NEON_INVALID_PARAM;
    }

    return NEON_OK;
}
```

Lưu ý:
- `neon_gpio_clock_enable()` **PHẢI được app gọi trước** `neon_gpio_init()` trên chip
  có clock-gating (đa số chip thật) — bản thân hàm này không tự enable ngầm
  cho hàm khác, kể cả nếu 1 family không cần bật clock (no-op) thì vẫn giữ
  API để app dùng chung 1 flow cho mọi target.
- File `.c` này **không có header riêng** — chỉ implement đúng interface
  đã khai trong `neon_<peripheral>.h`, không thêm hàm public mới ngoài
  interface đó (muốn thêm hàm mới, sửa header chung trước, để MỌI family
  đều biết interface đã đổi, tránh lệch API giữa family).

## 3. Wire vào CMake

`components/driver` tạo một target `neon_mdf_driver`. Mỗi peripheral có
`CMakeLists.txt` riêng và chọn source qua helper theo family:

```cmake
if(CONFIG_NEON_MDF_DRIVER_SPI)
  neon_mdf_driver_select_family_source(
    TARGET neon_mdf_driver
    PERIPHERAL spi
    SOURCES
      FU2 ${CMAKE_CURRENT_LIST_DIR}/platform/fu2/neon_spi.c
      MY_MCU ${CMAKE_CURRENT_LIST_DIR}/neon_spi_my_mcu.c
  )
endif()
```

Thêm folder mới vào vòng `foreach(... add_subdirectory(...))` tại
`components/driver/CMakeLists.txt`, rồi thêm Kconfig nếu driver có thể tắt.
Thêm family cho driver đã có chỉ cần thêm cặp `FAMILY source` trong helper.
GPIO dùng `REQUIRED` để configure báo lỗi khi thiếu backend. FU2 luôn build
CRM/SYSCSR/Flash; Flash của family khác chỉ cần khi bật bootloader.

Target chung đã public include root `components/driver` và link device,
Kconfig. Không cần thêm include path cho từng peripheral. eFuse vẫn có folder
riêng; Kconfig giữ tắt và audit bỏ qua vì chưa hỗ trợ memory map FU2 hiện tại.

## Quy tắc riêng của FU2

- Mọi truy cập dữ liệu SRAM/MMIO phải là word 32-bit căn hàng. Field logic
  8/16-bit trong struct dùng `neon_field8_t`/`neon_field16_t`; buffer byte sở hữu
  trọn các aligned word và được truy cập qua `<string.h>` của runtime FU2.
- DMA cũng phải giữ địa chỉ, số byte và burst chia hết cho bốn. Disassembly
  chỉ kiểm tra lệnh CPU, không chứng minh cấu hình DMA đúng.
- Helper private dùng `<module>_<action>` và linkage `static`, ví dụ
  `uart_wait_tx_ready()`. Interface nội bộ giữa các source có namespace
  family như `fu2_syscsr_*`.
- Critical section của backend FU2 dùng helper static lưu PRIMASK, gọi
  `__disable_irq()` và khôi phục bằng `__set_PRIMASK()`. Hàm Flash/reset phải
  giữ toàn bộ call chain đang chạy lúc Flash unavailable trong SRAM.
- API polling nhận timeout theo số vòng chờ, ghi rõ hành vi zero trong header.
  ADC/CRC zero chỉ kiểm tra readiness; DMA zero là chờ vô hạn tường minh.
- UART/I2C có tùy chọn route AUTO/KEEP; board cấu hình pinmux cho SPI/PWM/QSPI.

## 4. Link vào app — không cần làm gì thêm

App gọi `neon_mdf_add_executable()`/`neon_mdf_add_app_executable()`
(`tools/cmake/project.cmake`) đã tự `target_link_libraries(... PRIVATE
neon_mdf_driver)` — driver mới thêm vào `components/driver/CMakeLists.txt`
tự động có sẵn cho MỌI app, không cần sửa `CMakeLists.txt` của từng ví dụ.
`#include "gpio/neon_gpio.h"` (hay `"<peripheral>/neon_<peripheral>.h"`)
hoạt động ngay vì include root PUBLIC là `components/driver/` (bước cuối ở
mục 3).

## 5. Checklist thêm 1 loại peripheral HOÀN TOÀN MỚI (vd SPI, chưa từng có)

1. Peripheral đó đã khai trong `mcu-description/devices/<TARGET>.yaml`
   chưa (`peripherals:[].type: SPI`, xem `guide/DeviceYAML.md`) — chưa có
   thì thêm trước, địa chỉ/thanh ghi phải THẬT.
2. Tạo `components/driver/spi/neon_spi.h` (API chung, theo quy tắc mục 1).
3. Tạo `components/driver/spi/neon_spi_<family>.c` cho MỖI family cần dùng
   SPI (không bắt buộc mọi family phải có ngay — chỉ family thật sự cần).
4. Thêm folder vào `components/driver/CMakeLists.txt` và tạo CMake riêng
   dùng helper ở mục 3; thêm Kconfig nếu peripheral là tùy chọn.
5. Build thử 1 app gọi `neon_spi_*` cho family vừa viết — verify link sạch.
6. Regression-check family KHÁC vẫn build sạch (nhánh dispatch mới không
   được vô tình chặn/đổi behavior family cũ — xem lỗi thật đã gặp:
   `list(APPEND)` thiếu làm mất source đã set trước đó).

## 6. Checklist thêm family mới cho peripheral ĐÃ CÓ (vd thêm GPIO cho chip thứ 4)

1. Đối chiếu Reference Manual thật của chip mới — offset/bit CFGLR (hay
   tương đương) có thể khác hẳn family cũ dù cùng tên hàm API.
2. Viết `platform/<family_moi>/neon_<peripheral>.c` implement ĐÚNG interface hiện
   có trong `neon_<peripheral>.h` — không thêm/bớt hàm public (muốn đổi
   interface, sửa header chung trước, ảnh hưởng MỌI family, xem mục 2).
3. Thêm cặp `FAMILY source` vào helper trong
   `components/driver/<peripheral>/CMakeLists.txt` (mục 3).
4. Build thử app cho family mới — xem `guide/AddNewMCU.md` cho toàn bộ quy
   trình thêm target (device YAML, đăng ký CMake...) nếu đây cũng là target
   hoàn toàn mới, không chỉ family mới cho peripheral.
