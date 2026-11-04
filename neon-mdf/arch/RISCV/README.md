# Architecture support for RISC-V

Neon MDF hiện có backend cho `CH32V003F4P6` của WCH, dùng lõi QingKe V2A
(RV32EC). Cấu trúc trong thư mục này:

```text
arch/RISCV/
  Core/                     # CSR helpers, soft-math và libc shim
  Device/<Vendor>/<Family>/ # Device header/startup/linker sinh từ YAML; system code viết tay
  Port/                     # SDK_IRQ_* cho PFIC và critical section qua mstatus
```

`Port/Src/sdk_irq.c` điều khiển PFIC riêng của WCH và bit interrupt-enable
trong CSR `mstatus`. Đây không phải implementation PLIC/CLINT chuẩn.
`examples/FreeRTOS_Blinky` và template FreeRTOS dùng `SDK_IRQ_Enable()` để
bật ngắt tick của CH32V003.

CMake thêm source/include của Port khi chọn target RISC-V. Backend Cortex-M
dùng CMSIS NVIC/PRIMASK trực tiếp. Xem [FreeRTOSPort.md](../../../guide/FreeRTOSPort.md)
để biết cách nối tick và trap handler với FreeRTOS.
