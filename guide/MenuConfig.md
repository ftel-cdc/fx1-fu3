# Khai báo tuỳ chọn `menuconfig`/`sdkconfig`

Hướng dẫn thêm 1 option bật/tắt được qua `menuconfig` — giống hệ thống
`Kconfig`/`sdkconfig` của ESP-IDF (dùng lại `kconfiglib`, không tự viết
parser). Xem `ARCHITECTURE.md` mục 11 cho tổng quan thiết kế; file này đi
sâu vào **cú pháp khai option + cách dùng**.

## Mô hình chung

```
neon-mdf/Kconfig                      # mainmenu SDK - rsource từng component
  └─ components/driver/Kconfig         # option CỦA SDK
  └─ osource "$(NEON_MDF_APP_KCONFIG)" # option CỦA APP (nếu app có Kconfig riêng)

<app>/Kconfig     # TUỲ CHỌN - option riêng app, TỰ ĐỘNG gộp vào CÙNG 1 menu
<app>/sdkconfig   # luu lua chon THAT (text file, version-control theo app)
        │  (cmake/kconfig.cmake -> tools/kconfig/gen_config_header.py, chạy lúc configure)
        ▼
build/<TARGET>/config/sdkconfig.h      # #define CONFIG_X ... - dùng trong code C
build/<TARGET>/config/sdkconfig.cmake  # set(CONFIG_X ...) - dùng trong CMakeLists.txt
```

Option SDK (`components/driver/Kconfig`) và option app (`<app>/Kconfig`,
nếu có) **gộp chung 1 menu, 1 file `sdkconfig`** — app không cần biết gì về
cơ chế gộp, chỉ cần tạo file `Kconfig` cạnh `CMakeLists.txt` của mình (xem
mục 3).

## 1. Cú pháp khai 1 option

Cú pháp Kconfig chuẩn (giống hệt Linux kernel/ESP-IDF/Zephyr — `kconfiglib`
đọc trực tiếp, không có cú pháp riêng của SDK này):

```kconfig
menu "Ten menu hien thi"

config TEN_OPTION
    bool "Mo ta ngan hien trong menuconfig"
    default y
    help
        Mo ta dai, giai thich option nay lam gi, anh huong the nao.

config TEN_OPTION_SO
    int "So vong lap ..."
    default 500000
    help
        ...

endmenu
```

| Type | Cú pháp `default` | Sinh ra trong `sdkconfig.h` | Sinh ra trong `sdkconfig.cmake` |
|---|---|---|---|
| `bool` | `default y` \| `default n` | `#define CONFIG_TEN_OPTION 1` (chỉ khi `y`, KHÔNG có `#define` nào nếu `n` — dùng `#ifdef`, không phải `#if`) | `set(CONFIG_TEN_OPTION ON)` \| `OFF` |
| `int` | `default 500000` | `#define CONFIG_TEN_OPTION 500000` | `set(CONFIG_TEN_OPTION 500000)` |
| `hex` | `default 0x1000` | `#define CONFIG_TEN_OPTION 0x1000` | `set(CONFIG_TEN_OPTION 0x1000)` |
| `string` | `default "abc"` | `#define CONFIG_TEN_OPTION "abc"` | `set(CONFIG_TEN_OPTION "abc")` |

Tên option trong Kconfig (`TEN_OPTION`) **không có tiền tố** — macro/biến
sinh ra tự động thêm `CONFIG_` (quy ước giống hệt ESP-IDF). Đặt tên option
SDK với tiền tố `NEON_MDF_` (vd `NEON_MDF_DRIVER_UART`) để không đụng tên
với option của app; option app không cần tiền tố này (vd `BLINKY_DELAY_LOOPS`).

Ví dụ thật (`components/driver/Kconfig`):

```kconfig
menu "components/driver"

config NEON_MDF_DRIVER_UART
    bool "Bien dich UART driver (uart/neon_uart.c)"
    default y
    help
        Bao gom driver UART trong build. Tat de giam dung luong Flash neu
        app khong dung UART - luc do uart/neon_uart.c KHONG duoc compile.

endmenu
```

## 2. Dùng trong code C

```c
#include "sdkconfig.h"

#if defined(CONFIG_NEON_MDF_DRIVER_UART)
  /* chi bien dich khi option BOOL dang bat */
#endif

#define DELAY_LOOPS CONFIG_BLINKY_DELAY_LOOPS   /* option INT dung truc tiep nhu hang so */
```

`sdkconfig.h` tự nằm trong include path (target `neon_mdf_kconfig`, đã link
sẵn vào `neon_mdf_driver` PUBLIC — mọi app link `neon_mdf_driver` đều
`#include "sdkconfig.h"` được ngay, không cần khai gì thêm trong
`CMakeLists.txt` của app).

## 3. Dùng trong `CMakeLists.txt` (bật/tắt file nguồn)

```cmake
if(CONFIG_NEON_MDF_DRIVER_UART)
  list(APPEND _neon_mdf_driver_sources uart/neon_uart.c)
endif()
```

Biến `CONFIG_*` chỉ có giá trị **SAU KHI** `neon_mdf_process_kconfig()`
(`cmake/kconfig.cmake`) đã chạy — trong `components/driver/CMakeLists.txt`
luôn dùng được vì `neon-mdf/CMakeLists.txt` gọi hàm này TRƯỚC
`add_subdirectory(components/driver)` (xem `guide/ArchitectDetail.md` mục
4 cho thứ tự đầy đủ). Đây chính là cơ chế `components/driver` dùng để
KHÔNG compile 1 file `.c` khi option tắt (xem `guide/DriverImplement.md`
mục 3, kiểu dispatch (c)) — khác "ẩn tính năng lúc chạy", app gọi hàm bị
tắt sẽ lỗi **link** rõ ràng (`undefined reference`), không phải lỗi ẩn.

## 4. Thêm option mới cho SDK (component có sẵn)

Sửa thẳng `components/driver/Kconfig` (thêm 1 khối `config ...` mới trong
`menu` đã có). Không cần sửa `neon-mdf/Kconfig` (đã `rsource` sẵn file
này).

## 5. Thêm Kconfig cho app (option riêng của 1 ví dụ)

Tạo `<app>/Kconfig` cạnh `<app>/CMakeLists.txt` — **không cần sửa
`CMakeLists.txt` của app** để "bật" tính năng này (giống `osource` = kiểu
`-include` của Make, tự động gộp nếu file tồn tại, im lặng bỏ qua nếu
không — hành vi cũ của app không có file này giữ nguyên 100%):

```kconfig
menu "Ten App (app nay)"

config MY_APP_SOME_OPTION
    int "Mo ta option"
    default 1000
    help
        ...

endmenu
```

Đọc trong `Src/main.c` qua `#include "sdkconfig.h"` +
`CONFIG_MY_APP_SOME_OPTION` giống hệt option của SDK — không phân biệt
được (và không cần phân biệt) option nào tới từ SDK hay từ app trong code
dùng nó.

## 6. Mở menuconfig — sửa giá trị

```sh
cd examples/Blinky
cmake --build --preset WEACT_F412 --target menuconfig   # mo UI chon (giong idf.py menuconfig)
cmake --preset WEACT_F412                                 # BAT BUOC re-configure sau khi luu de ap dung
```

Hoặc qua VS Code: lệnh **NEON-MDF: menuconfig**. Hoặc CLI:
`neon_mdf.py menuconfig`. Cả 3 đường đều gọi cùng 1 target CMake `menuconfig`
(`cmake/kconfig.cmake`) — không có logic riêng nào khác ở tầng UI.

Cũng có thể sửa tay `<app>/sdkconfig` (text file, giống ESP-IDF) rồi
`cmake --preset <TARGET>` lại — ví dụ tắt UART:

```
# CONFIG_NEON_MDF_DRIVER_UART is not set
```

`sdkconfig` **tự ghi lại mỗi lần configure** (giống ESP-IDF): option mới
thêm vào Kconfig tự điền `default` vào file, option không còn tồn tại
trong Kconfig (đã xoá khỏi file `.Kconfig`) tự bị dọn khỏi `sdkconfig` —
không cần tự tay dọn file này khi đổi cấu trúc option.

## Checklist thêm 1 option mới

1. Xác định thuộc SDK (sửa `components/driver/Kconfig`, hoặc file
   `Kconfig` mới cho component khác + thêm `rsource` trong `neon-mdf/Kconfig`)
   hay thuộc riêng 1 app (`<app>/Kconfig`, tự động gộp, không sửa gì thêm).
2. Chọn đúng `type` (`bool`/`int`/`hex`/`string`) — quyết định cách dùng
   trong C (`#ifdef` cho `bool`, giá trị trực tiếp cho `int`/`hex`/`string`).
3. Đặt `default` hợp lý — app build lần đầu (chưa có `sdkconfig`) dùng
   ĐÚNG giá trị này, không phải giá trị rỗng/0.
4. Nếu dùng để bật/tắt compile 1 file `.c` — thêm `if(CONFIG_...)` đúng chỗ
   trong `CMakeLists.txt` liên quan (xem mục 3).
5. `cmake --preset <TARGET>` lại (configure) để `sdkconfig.h`/`.cmake` sinh
   ra khớp Kconfig mới — build cũ (`build/`) không tự cập nhật.
