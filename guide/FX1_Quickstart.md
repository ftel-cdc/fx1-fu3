# FX1 — chọn luồng build và chạy

Bắt đầu tại gốc SDK, nơi chứa `examples/`, `neon-mdf/`, `guide/`.
Tên thư mục clone và vị trí trên máy không ảnh hưởng các lệnh.

- [Chuẩn bị công cụ và terminal](FX1_FREERTOS.md#1-chuẩn-bị-terminal):
  Python local, PATH toolchain, `SDK_ROOT`, `VP_ROOT`, `RAW_FILE`.
- [Ảnh Bare-metal](FX1_NEON_ISP_RUN.md): app `FX1_ISP_Image`.
- [Ảnh FreeRTOS](FX1_FREERTOS.md#8-raw-thật-qua-freertos--giữ-runner-elfraw-hiện-có):
  app `FX1_ISP_Image_FreeRTOS`, cùng giao diện runner `run_vp_neon ELF RAW`.
- [Init và frame/AEC](FX1_FREERTOS.md#2-build-elf-và-chạy-init-trong-task),
  [service ISP](FX1_FREERTOS.md#3-chạy-frameaec-và-service-isp),
  [smoke CPU/tick/FPU/PLIC](FX1_FREERTOS.md#4-smoke-freertos-nền-tảng).
- [Vai trò các thư mục](FX1_FREERTOS.md#5-file-chính),
  [API và buffer ownership](FX1_FREERTOS.md#6-phạm-vi-và-ownership),
  [YAML và đổi vị trí SDK](FX1_FREERTOS.md#7-yaml-và-đổi-vị-trí-sdk).

Build ELF dùng startup/linker/BSP/kernel local; VP và RAW chỉ cần lúc run.
Hai example ảnh có profile/generator local riêng. Sau bước chuẩn bị terminal,
chọn một trong hai thư mục và build theo cùng thứ tự:

```bash
cd "$SDK_ROOT/examples/FX1_ISP_Image_FreeRTOS"
python3 tools/generate_profile.py
cmake --preset FX1 -DNEON_MDF_PYTHON3="$FX1_PYTHON"
cmake --build --preset FX1
```

Đổi thư mục thành `examples/FX1_ISP_Image` để build Bare-metal. ELF tương ứng:
`fx1_isp_image_freertos.elf` hoặc `fx1_isp_image.elf` trong `build/FX1/`.
Bước Python sinh header xem trước; CMake tự sinh header dùng khi biên dịch.
Không cần dùng cache, ELF hoặc đường dẫn từ máy tác giả.
