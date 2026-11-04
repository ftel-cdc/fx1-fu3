# rtos

- `FreeRTOS-Kernel/` — vendor từ repo chính thức
  (`https://github.com/FreeRTOS/FreeRTOS-Kernel.git`, chỉ kernel, không kèm
  demo project). 3 port đã wire (chọn tự động theo `arch`, xem
  `cmake/freertos.cmake`): `portable/GCC/ARM_CM0` (FU2, Cortex-M0 — generic,
  không cần chỉnh gì riêng), `portable/GCC/ARM_CM4F` (WEACT_F412, Cortex-M4F —
  cần alias tường minh SVC/PendSV/SysTick sang tên hàm của port, xem
  `guide/FreeRTOSPort.md`), `portable/GCC/RISC-V` + chip extension
  `RISCV_no_extensions` (CH32V003F4P6, không có CLINT — app tự cấu hình tick
  timer/`mtvec`, xem cùng guide).

  **Trạng thái hiện tại:** thư mục này đang là **vendor thuần** (file đã copy
  sẵn, không phải git submodule sống — repo gốc chưa `git init`). Nếu muốn
  tái lập lại thành git submodule sau khi `git init` repo này:
  ```sh
  rm -rf rtos/FreeRTOS-Kernel   # xoá bản vendor thuần hiện tại
  git submodule add --depth 1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git rtos/FreeRTOS-Kernel
  ```

  Đã wire vào CMake (`cmake/freertos.cmake`, gọi `add_subdirectory()` vào
  kernel dùng hệ thống CMake chính thức của họ, không tự liệt kê source) —
  xem `examples/FreeRTOS_Blinky/`. Mỗi app tự có `FreeRTOSConfig.h`
  riêng (đặt trong `Inc/` của app, không đặt trong SDK) vì stack/heap/priority
  là lựa chọn của app. Xem `ARCHITECTURE.md` mục 9 để biết chi tiết + lưu ý
  đã verify về port `GCC_ARM_CM0` (không cần `configPRIO_BITS`).
