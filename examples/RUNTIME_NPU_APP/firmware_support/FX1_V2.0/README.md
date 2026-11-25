# FX1 V2.0 firmware support dùng bởi MDF runtime

Thư mục này chứa BSP và FreeRTOS subset dùng để build firmware runtime cho FX1 V2.0 trong `examples/RUNTIME_NPU_APP/Makefile`:

- FreeRTOS kernel/config, startup và linker script.
- UART/PLIC BSP cùng `libc_min.c`.
- Header SoC memory map and IRQ map. NPU V4 registers live with the V2 driver.

Firmware graph runtime được build bằng Makefile của ví dụ, không bằng Makefile firmware vendor. Model `.nsm` được truyền riêng cho VP binary tại `examples/RUNTIME_NPU_APP/platform/virtual_platform/fx1/vp/bin/VP_FX1_V2.0/`. Các demo TFLite Micro và test firmware vendor không nằm trong BSP subset này. Mã NPU driver nằm tại `neon-mdf/components/driver/runtime_npu/platform/FX1_V2.0/`.

Xem [RUNTIME_NPU_APP README](../../../README.md) để biết cấu trúc và lệnh chạy.

