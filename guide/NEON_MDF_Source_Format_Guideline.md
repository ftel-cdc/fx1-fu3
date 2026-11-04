# Neon MDF — Source Format & Convention V1.3

## 1. Mục đích và phạm vi

Neon MDF áp dụng [NEON Coding Convention V1.3](NEON_Coding_Convention_V1.3.md)
cho C/header viết tay trong SDK, template và example. File này mô tả cách áp
convention vào cấu trúc, build và phần cứng đang có của Neon MDF.

Quy ước chính: public HAL API `neon_module_action()`, private helper
`module_action()`, type `_t`, field `snake_case`, indent 4 spaces và ngoặc mở
cùng dòng. Helper private vẫn giữ từ khóa `static`.

Device register type, tên register, startup/linker symbol, `main`, ISR,
`SystemInit`, `SystemCoreClock`, syscall/runtime và hook FreeRTOS giữ hợp đồng
của nền tảng. `SDK_IRQ_*` là interface Port RISC-V, không phải HAL API.
Cortex-M dùng CMSIS trực tiếp. CMSIS/FreeRTOS và output generator không được
đổi chỉ để đồng bộ style.

## 2. Cấu trúc repository

```text
<repository>/
├── .clang-format
├── .clang-format-ignore
├── neon-mdf/
│   ├── components/
│   │   ├── driver/                 # HAL và backend ngoại vi
│   │   ├── console/                # printf/console FU2
│   │   ├── bootloader/
│   │   └── ota/
│   ├── arch/                       # CPU, device, IRQ, runtime
│   ├── mcu-description/            # YAML, schema, generator
│   ├── cmake/                      # Toolchain và tích hợp build
│   ├── tools/                      # CLI, template, audit và test
│   └── rtos/FreeRTOS-Kernel/        # Upstream và patch nền tảng
├── examples/<App>/                 # Project CMake độc lập
└── guide/
```

Cấu trúc SDK/app và chiều phụ thuộc được mô tả thêm trong
[ARCHITECTURE.md](../ARCHITECTURE.md).

## 3. Kiến trúc và dependency

App gọi component/HAL; backend ánh xạ API sang register, clock, reset và
pinmux của family. Driver không include `main.h` hay phụ thuộc source app.
Header dùng `mcu_device.h` hoặc header `neon_<module>_device.h` tương ứng để
lấy type/instance của device được chọn.

CMake chọn backend theo `NEON_MDF_TARGET_<TARGET>_FAMILY`. Cortex-M bật/tắt
IRQ và đặt priority qua CMSIS `NVIC_*`, critical section lưu/khôi phục PRIMASK
bằng CMSIS. RISC-V dùng `SDK_IRQ_*` từ `arch/RISCV/Port/`.
Thao tác Flash/reset cần thiết trên FU2 vẫn chạy từ SRAM.

## 4. Quy ước thư mục và file source

| Thành phần | Đường dẫn/mẫu hiện tại |
|---|---|
| Entry header tùy chọn | `components/driver/neon.h` |
| Type dùng chung | `components/driver/neon_types.h` |
| Module switches | `components/driver/neon_config.h` |
| Header HAL | `components/driver/<module>/neon_<module>.h` |
| Header instance | `<module>/neon_<module>_device.h`, khi API cần alias instance |
| Backend | `<module>/platform/<family>/neon_<module>.c` |
| Header internal theo platform | `syscsr/platform/fu2/neon_syscsr_internal.h` |
| Utility memory nội bộ FU2 | `components/driver/neon_utils_memory.h` |
| Source/header app | `examples/<App>/Src/`, `Inc/` theo app hiện có |
| Testcase terminal | `examples/FPGA_Terminal/testcase/<Testcase>/` |

Ví dụ:

```text
components/driver/uart/
├── CMakeLists.txt
├── neon_uart.h
├── neon_uart_device.h
└── platform/
    ├── fu2/neon_uart.c
    └── stm32f103/neon_uart.c
```

Tên file dùng chữ thường và dấu gạch dưới. Các tên thư mục app/testcase đã có
như `FPGA_Terminal`, `CRC_Known_Vector` vẫn là tên project/testcase. Testcase
được thêm bằng `target_sources()`, không include trực tiếp file `.c`.

## 5. Naming convention

| Thành phần | Format/ví dụ |
|---|---|
| Public HAL function | `neon_uart_transmit()`, `neon_flash_erase_sector()` |
| Private function | `static ... uart_wait_tx_ready(...)` |
| Handle | `neon_uart_handle_t` |
| Configuration | `neon_uart_config_t` |
| Struct/enum khác | `neon_uart_state_t`, `neon_spi_fifo_config_t` |
| Type dùng chung | `neon_status_t`, `neon_field8_t`, `neon_field16_t` |
| Named struct tag | `neon_uart_handle_s` nếu cần đặt tag |
| Enum value/public macro | `NEON_UART_STATE_READY`, `NEON_GPIO_PIN_7` |
| Private macro | `UART_CONTEXT_PIN_AUTO`, `FLASH_RAMFUNC` |
| Field/parameter/local | `baud_rate`, `clock_frequency_hz`, `timeout_polls` |
| Header guard | `__NEON_UART_H__` theo lựa chọn của V1.3 |

Private helper không thêm `static_` vào tên. Type của HAL dùng `_t`; type
register như `UART_TypeDef` vẫn do device generator cung cấp. Interface nội
bộ theo platform có thể mang namespace family, ví dụ `fu2_syscsr_*`; ứng dụng
không dùng header internal.

Enum công khai có giá trị tường minh, giữ nguyên encoding hiện có. Dùng macro
unsigned cho bitmask kết hợp được. Giữ literal `U`, `UL`, `ULL` theo độ rộng.
Input pointer không bị sửa dùng `const`. Các operation có thể thất bại thường
trả `neon_status_t`; giữ ý nghĩa của các status hiện hữu, kể cả
`NEON_NOT_SUPPORTED`, `NEON_LOCKED`, `NEON_NOT_MAPPED`.

## 6. Format header

Thứ tự: Doxygen file → include guard → include cần thiết → type/macro → public
prototype → đóng C++ guard và include guard. Dùng section banner như các file
hiện tại; chỉ tạo section có nội dung.

Header tự cung cấp dependency cho khai báo của nó và dùng độc lập được trên
family mà module hỗ trợ. Bọc khai báo external bằng `extern "C"` có điều
kiện. Private biến/helper nằm trong `.c`; header internal ghi `@internal`.

```c
#ifndef __NEON_EXAMPLE_H__
#define __NEON_EXAMPLE_H__

#include "neon_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the example module.
 * @return NEON_OK on success, otherwise a module error status.
 */
neon_status_t neon_example_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __NEON_EXAMPLE_H__ */
```

## 7. Format source

Source include public header tương ứng trước; tiếp theo là dependency
private/platform và standard library. File đã chuyển vào `platform/` include
header bằng đường dẫn module như `"uart/neon_uart.h"`.

Thứ tự section:

1. `INCLUDE FILES`
2. `DEFINES AND MACROS` khi có
3. `STRUCTURES AND OTHER TYPEDEFS` khi có
4. `LOCAL VARIABLES` khi có
5. `PRIVATE FUNCTION PROTOTYPES` khi có
6. `GLOBAL FUNCTIONS`
7. `PRIVATE FUNCTIONS` khi có

Dùng 4 spaces, ngoặc mở cùng dòng, có ngoặc cho mọi `if/else/for/while`.
Không gộp hàm, vòng lặp hoặc nhánh điều kiện thành một dòng. Khai báo helper
trước nơi gọi. File `.c` biên dịch bằng C không cần thêm khối `extern "C"`.

`.clang-format` tại root mã hóa các quy tắc trình bày, gồm `InsertBraces` và
cột khuyến nghị 120. `.clang-format-ignore` loại build, upstream và output
hardware generator khỏi thao tác format hàng loạt.

## 8. Comment và Doxygen

File có `@file`, `@brief`, `@details`. Public API mô tả tại header bằng
`@brief`, `@param[in/out/in,out]`, `@return` khi non-void. Dùng `@details`,
`@note` khi cần giải thích blocking, ISR, thread safety, ownership và side
effect của register. Implementation dùng `@copydoc` để dùng lại hợp đồng.

Không thêm `@param None`. Comment tập trung vào lý do và hợp đồng phần cứng,
đặc biệt timeout, read-to-clear/W1C, buffer, pinmux và thứ tự reset.

## 9. Cấu hình và include

`neon_config.h` cung cấp switch `NEON_<MODULE>_MODULE_ENABLED` dạng **0/1**.
Các switch mặc định phản ánh family và `sdkconfig` của build; có thể override
switch trước khi include header. `neon.h` là entry header tùy chọn để gộp các
module đang bật. Header từng module vẫn include độc lập được.

```c
#include "neon_config.h"

#if NEON_UART_MODULE_ENABLED == 1
#include "uart/neon_uart.h"
#endif
```

Kconfig/menuconfig vẫn là nguồn lựa chọn build của SDK. CMake đọc
`CONFIG_NEON_MDF_DRIVER_<MODULE>` để thêm source; `neon_config.h` chuyển lựa
chọn đó sang switch C 0/1. Override switch trong source không tự thêm driver
bị tắt vào thư viện: khi đổi tính năng phải cập nhật Kconfig/build tương ứng.
Header HAL không yêu cầu cấu hình riêng của app chỉ để parse khai báo.

Chi tiết: [MenuConfig.md](MenuConfig.md).

## 10. Handle, cấu hình và nền tảng

UART/I2C có bước khởi tạo runtime handle và áp cấu hình riêng:

```c
neon_uart_handle_t uart;
neon_uart_config_t config;
neon_uart_get_default_config(&config);
config.baud_rate = 19200U;

neon_status_t status = neon_uart_init(&uart, NEON_UART0);
if (status == NEON_OK) {
    status = neon_uart_set_config(&uart, &config);
}
```

Handle do caller cấp chỗ nhớ, field do driver quản lý. `init()` bind instance
và tạo trạng thái `INITIALIZED`; `set_config()` sao chép cấu hình vào storage
private của backend và thiết lập phần cứng. Một instance có một owner sau
khi cấu hình thành công, đến khi `deinit()`.

Đọc thông tin bằng `get_config()`, `get_instance()`, `get_state()`,
`get_error()`. Caller serialize thao tác cấu hình/teardown với các task/ISR
khác dùng cùng peripheral. Các API polling/configuration có thể blocking
thuộc task context; không gọi blocking wait trong ISR.

Các ràng buộc FU2 tiếp tục áp dụng:

- SRAM/MMIO dùng aligned word 32-bit. `neon_field8_t` và `neon_field16_t`
  vẫn là `uint32_t` trên FU2; backend khác giữ độ rộng 8/16-bit.
- Buffer byte/string sở hữu trọn word chứa dữ liệu và terminator. Dùng
  `<string.h>` cho RAM bình thường; runtime liên kết implementation word-only.
- `neon_utils_memory.h` chỉ là backend nội bộ của `fu2_runtime.c`. MMIO dùng
  register word `volatile`, không đưa MMIO vào hàm memory/string.
- Giữ yêu cầu alignment/count/burst của DMA và `.ramfunc` của Flash/reset.
- Giữ compiler policy FU2 và kiểm tra word-access trên firmware/archive.
- `timeout_polls` là số vòng poll. Giá trị 0 có ý nghĩa theo từng API; không
  tự chuyển timeout thành milliseconds chỉ vì ví dụ trong convention dùng ms.

Build lại toàn bộ object sau khi đổi API/type/handle. Xem [FU2.md](FU2.md).

## 11. Checklist và kiểm tra

- Chọn đúng component/module và backend family.
- Dùng tên file/API/type/field và guard theo V1.3.
- Kiểm tra header độc lập, C++ guard và Doxygen của API.
- Cập nhật mọi chỗ gọi, CMake, template và test khi đổi interface/đường dẫn.
- Kiểm tra owner, input `const`, đơn vị, bounds, timeout, ISR và side effect.
- Format source viết tay; giữ nguyên register/runtime/upstream contract.
- Build các target liên quan, chạy regression có ý nghĩa cho thay đổi.
- Với FU2 kiểm tra word-access, SRAM placement và emulation; emulation không
  xác nhận timing FPGA hay phần cứng thực tế.

```sh
. neon-mdf/export.sh
cd examples/FPGA_Terminal
cmake --preset FU2
cmake --build --preset FU2
```

Cách thêm driver: [DriverImplement.md](DriverImplement.md).

## 12. Mẫu module

Đây là mẫu bố trí tối thiểu; `EXAMPLE` không phải driver có sẵn trong SDK.
Thêm file header theo mục 6 và mô tả đầy đủ các public prototype.

```c
/**
 * @file neon_example.c
 * @brief Example source layout.
 * @details Demonstrates V1.3 private naming and function organization.
 */

/*==================================================================================================
*                                          INCLUDE FILES
==================================================================================================*/
#include "neon_example.h"

/*==================================================================================================
*                                   PRIVATE FUNCTION PROTOTYPES
==================================================================================================*/
static neon_status_t example_reset(void);

/*==================================================================================================
*                                         GLOBAL FUNCTIONS
==================================================================================================*/
/** @copydoc neon_example_init */
neon_status_t neon_example_init(void) {
    return example_reset();
}

/*==================================================================================================
*                                        PRIVATE FUNCTIONS
==================================================================================================*/
static neon_status_t example_reset(void) {
    return NEON_OK;
}
```

Khi thêm backend thật, đặt source trong `platform/<family>/`, khai báo qua
`neon_mdf_driver_select_family_source()` và thêm Kconfig khi module tùy chọn.
