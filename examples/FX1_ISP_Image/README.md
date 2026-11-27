# FX1_ISP_Image — camera session với deferred IRQ

Example này dùng `neon_camera_freertos`, register backend scalar MAS v1.07 và
session core. Mặc định replay tám frame RAW12 BGGR 2688×1520, bốn slot luân phiên,
output CH0 NV12 cùng kích thước. WB/DG unity, CSC BT.601 limited range; table
consumers và 3A/statistics tắt. Các file `profile/*.csrw`, binary 3A và generator
cũ còn lưu nhưng **không được nạp hoặc link vào đường mới**. Hướng dẫn legacy
`guide/FX1_NEON_ISP_RUN.md` và hash ảnh D65 cũ không áp dụng cho example này.

```sh
cmake --preset FX1 -B /tmp/fx1-camera-image
cmake --build /tmp/fx1-camera-image
```

Nếu toolchain chưa có trong PATH, truyền
`-DNEON_FX1_TOOLCHAIN_DIR=/opt/toolchains/riscv-none-elf/bin` hoặc thư mục tương ứng.
ELF: `/tmp/fx1-camera-image/fx1_isp_image.elf`. `sdkconfig` bật CAMERA_RUNTIME và
CAMERA_FX1_BACKEND; FreeRTOS config nằm trong `Inc/` của chính example.

RAW là 16-bit little-endian containers, lower 12 bits, stride `2*WIDTH`. Nạp cùng
geometry vào cả bốn địa chỉ `0x81000000`, `0x81800000`, `0x82000000`, `0x82800000`
trước khi chạy. Output Y ở `0x88000000 + slot*0x00800000`, UV ở Y+`0x00400000`.
Y/UV strides bằng WIDTH. Ví dụ tối thiểu cho VP có capability v1.07 phù hợp:

```sh
fx1_soc --fw /tmp/fx1-camera-image/fx1_isp_image.elf --timeout-ms 6000 \
  --load 0x81000000:raw.bin --load 0x81800000:raw.bin \
  --load 0x82000000:raw.bin --load 0x82800000:raw.bin
```

`-DFX1_IMAGE_FRAME_COUNT=N`, `-DFX1_IMAGE_WIDTH=W`, `-DFX1_IMAGE_HEIGHT=H` cấu hình
replay; kích thước chẵn 32..3840 × 32..2160, stride phải chia hết 16. Slot buffers
vẫn ở vùng DDR nêu trên. Consumer acquire/return qua command queue; STOP dùng
mailbox riêng. Worker giữ claim đến drain, mọi lease return và IRQ detach hoàn tất.

## Evidence ngày 27/11/2026

Cross-build RV32 pass cả default geometry và 64×48. VP local revision
`36ad26a4b200232ff49570c2b31f1120ccec5bd3` có version `0x00010007`, TUNE_ID
`0x54554E45` nhưng caps `0x003FFFFF` thiếu INPUT_PAD. Example log START_RESULT=5
(UNSUPPORTED), dừng bằng mã `0xca01`; chưa xử lý được ảnh trên VP này. Không có
legacy fallback, không suy build/host-test pass thành image end-to-end pass.

Có probe riêng trong `neon-mdf/tests/camera_hal/fx1_irq_port_probe.c`, build bằng:

```sh
cmake --preset FX1 -B /tmp/fx1-camera-probe -DFX1_CAMERA_IRQ_PORT_PROBE=ON
cmake --build /tmp/fx1-camera-probe --target fx1_camera_irq_port_probe
fx1_soc --fw /tmp/fx1-camera-probe/fx1_camera_irq_port_probe.elf --timeout-ms 500
```

Probe chỉ chạy trên ISP stopped/exclusive, chủ động tạo lỗi alignment DMA,
không đọc/ghi ảnh hợp lệ. VP PASS deferred IP mask giữ sticky status, PLIC complete,
đăng ký trùng bị chặn, unregister đúng context và re-register với stale pending.
Đây là kiểm chứng port IRQ độc lập, không vượt kiểm tra capability của session.

Xem [runtime README](../../neon-mdf/components/neon_camera_runtime/README.md) và
[MAS mapping](../../neon-mdf/components/neon_camera_runtime/MAS_MAPPING.md) cho
phạm vi scalar, kiểm thử, cache hooks và các phần chưa nghiệm thu trên hardware.
