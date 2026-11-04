# Thêm device và driver

`components/driver` tạo duy nhất target CMake `neon_mdf_driver`. Mỗi
peripheral có thư mục riêng và tự thêm source vào target đó qua
`target_sources()`. Việc chọn backend dựa trên **family**, không dựa trực tiếp
trên tên device:

```text
NEON_MDF_TARGET = STM32F103CB
             └─ NEON_MDF_TARGET_STM32F103CB_FAMILY = STM32F103
                                                        └─ chọn platform/stm32f103/neon_gpio.c
```

Do đó cần phân biệt rõ hai trường hợp dưới đây.

## Thêm device cùng family đã có

Ví dụ: thêm một biến thể STM32F103 khác, vẫn dùng cùng register layout và
backend `STM32F103`.

1. Tạo `mcu-description/devices/<DEVICE>.yaml`.
2. Đăng ký `<DEVICE>` trong `cmake/mcu_variants.cmake`:

   ```cmake
   list(APPEND NEON_MDF_KNOWN_TARGETS <DEVICE>)
   set(NEON_MDF_TARGET_<DEVICE>_ARCH cortex-m3)
   set(NEON_MDF_TARGET_<DEVICE>_FAMILY STM32F103)
   ```

3. Khai báo các thuộc tính riêng của device nếu cần, ví dụ FPU, vendor, ABI
   hoặc `bootloader:` trong YAML.
4. Configure và build app với `-DNEON_MDF_TARGET=<DEVICE>` hoặc preset tương
   ứng.

Không sửa CMake trong `components/driver` nếu device mới thực sự dùng cùng
backend với family cũ. Nếu register layout khác đáng kể, tạo family mới thay
vì cố dùng lại family cũ.

## Thêm family mới

Ví dụ family `MY_MCU` có GPIO và UART riêng.

1. Đăng ký device với `NEON_MDF_TARGET_<DEVICE>_FAMILY MY_MCU` trong
   `cmake/mcu_variants.cmake`.
2. Thêm `MY_MCU` vào guard cho phép dùng driver trong `neon-mdf/CMakeLists.txt`.
3. Tạo source backend trong peripheral phù hợp, ví dụ:

   ```text
   gpio/platform/my_mcu/neon_gpio.c
   uart/platform/my_mcu/neon_uart.c
   ```

4. Thêm mapping vào `CMakeLists.txt` của **chỉ các peripheral mà family hỗ
   trợ**. Ví dụ trong `gpio/CMakeLists.txt`:

   ```cmake
   neon_mdf_driver_select_family_source(
     TARGET neon_mdf_driver
     PERIPHERAL gpio
     REQUIRED
     SOURCES
       FU2     ${CMAKE_CURRENT_LIST_DIR}/platform/fu2/neon_gpio.c
       MY_MCU  ${CMAKE_CURRENT_LIST_DIR}/platform/my_mcu/neon_gpio.c
   )
   ```

`REQUIRED` dùng cho GPIO: thiếu backend thì cấu hình phải lỗi. Với driver tùy
chọn như ADC, SPI, I2C, không dùng `REQUIRED`; family chưa hỗ trợ sẽ không có
source nào được thêm vào.

5. Nếu device có bootloader, thêm mapping Flash trong `flash/CMakeLists.txt`.
   File này chỉ yêu cầu backend khi `NEON_MDF_BOOTLOADER_ENABLED` là `ON`.
6. Build ít nhất một app cho family mới. Kiểm tra log build có đúng các file
   `platform/my_mcu/neon_<peripheral>.c` được compile.

## Thêm driver/peripheral mới

Ví dụ thêm CAN driver.

1. Tạo thư mục `components/driver/can/`, đặt header và source tại đây:

   ```text
   can/neon_can.h
   can/platform/<family>/neon_can.c
   can/CMakeLists.txt
   ```

2. Thêm `can` vào danh sách `foreach(... add_subdirectory(...))` trong
   `components/driver/CMakeLists.txt`. Đây là một thay đổi bắt buộc để CMake
   biết đọc CMake của folder mới.
3. Nếu có thể tắt driver để giảm Flash, thêm symbol Kconfig vào
   `components/driver/Kconfig`, ví dụ `NEON_MDF_DRIVER_CAN`.
4. Viết `can/CMakeLists.txt` và gọi helper. Ví dụ CAN chỉ có trên STM32F103
   và WEACT_F412:

   ```cmake
   if(CONFIG_NEON_MDF_DRIVER_CAN)
     neon_mdf_driver_select_family_source(
       TARGET neon_mdf_driver
       PERIPHERAL can
       SOURCES
         STM32F103  ${CMAKE_CURRENT_LIST_DIR}/platform/stm32f103/neon_can.c
         WEACT_F412 ${CMAKE_CURRENT_LIST_DIR}/platform/weact_f412/neon_can.c
     )
   endif()
   ```

5. Include header theo đường dẫn qualified từ app/source khác:

   ```c
   #include "can/neon_can.h"
   ```

Không thêm `target_include_directories()` cho từng peripheral: include path
public đã là root `components/driver`, nên cách include trên hoạt động cho mọi
driver.

Trong driver, public API dùng `neon_<module>_<action>()`, private helper
`static ... <module>_<action>()`, type `_t`, field `snake_case`. Header guard
và cách trình bày theo [convention V1.3](../../../guide/NEON_Coding_Convention_V1.3.md).
Các backend nằm trong `platform/<family>/` với tên `neon_<module>.c`.
UART/I2C tách khởi tạo handle và cấu hình phần cứng. Interface pinmux nội bộ
FU2 nằm ở `syscsr/platform/fu2/neon_syscsr_internal.h`.

## `neon_mdf_driver_select_family_source()`

Helper trong `components/cmake/driver_helpers.cmake` có dạng:

```cmake
neon_mdf_driver_select_family_source(
  TARGET <target>
  PERIPHERAL <tên-peripheral>
  [REQUIRED]
  SOURCES
    <FAMILY_1> <source_1>
    <FAMILY_2> <source_2>
)
```

Nó lấy family của target đang build, tìm cặp `FAMILY source` tương ứng và gọi
`target_sources(<target> PRIVATE <source>)`. Nên dùng
`${CMAKE_CURRENT_LIST_DIR}` cho source để đường dẫn luôn là thư mục chứa
`CMakeLists.txt` hiện tại.

## Kiểm tra trước khi gửi thay đổi

```sh
cmake --preset <DEVICE>
cmake --build --preset <DEVICE>
```

Nên kiểm tra cả một device cùng family cũ (để bảo đảm không làm hỏng backend
đang có) và device/family vừa thêm. Khi tắt Kconfig của driver tùy chọn, xác
nhận source của nó không xuất hiện trong log build.
