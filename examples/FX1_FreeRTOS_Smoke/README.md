# FX1 FreeRTOS smoke — CPU/tick/FPU/PLIC

Preset `FX1_VP` dùng cùng family/BSP local với target `FX1`. App không chứa
driver ISP, không cần RAW; kiểm startup, tick, context FPU, PLIC và ISR/task.
Chuẩn bị công cụ và biến môi trường theo
[FX1 Quickstart](../../guide/FX1_FREERTOS.md).

## Build và chạy

```bash
cd "$SDK_ROOT/examples/FX1_FreeRTOS_Smoke"
cmake --preset FX1_VP \
  -DNEON_MDF_PYTHON3="$FX1_PYTHON" \
  -DNEON_FX1_VP_EXECUTABLE="$NEON_FX1_VP_EXECUTABLE"
cmake --build --preset FX1_VP --target run-vp
```

Chỉ build ELF: `cmake --build --preset FX1_VP --target fx1_freertos_smoke`.
Build ELF không yêu cầu VP; run target mới kiểm executable và chạy mô phỏng.

| File trong `build/FX1_VP/` | Nội dung |
|---|---|
| `fx1_freertos_smoke.elf` | Firmware |
| `vp/platform.log` | 25 check firmware và verdict VP |
| `vp/result.json` | Verdict, số check và hash ELF/VP |
| `vp/elf_inspect.txt` | Kiểm ELF/ISA/ABI |

Mong đợi `[FX1_VP] SUMMARY checks=25 failed=0`, `[fx1_soc] RESULT PASS`,
exit 0. Runner kiểm ELF32/RISC-V/little-endian/single-float ABI, ISA object,
đủ các check và summary. Banner hoặc build thành công chưa là runtime PASS.

Sau chuyển máy/thư mục: `cmake --fresh --preset FX1_VP` rồi truyền lại
Python và đường dẫn VP như lệnh configure trên.

## Phạm vi nền tảng

CLINT mtime 20 MHz, tick 1 kHz, PLIC và FreeRTOS chạy trên hart 0;
hart 1 chờ. Giữ `NEON_FX1_SAVE_FPU_CONTEXT=ON`, `configENABLE_FPU=0` theo
extension local đã kiểm. Startup/linker ở `Device/FTEL/FX1/Source/gcc/`,
FPU glue ở `rtos/port/fx1/`. Chi tiết và bài negative control:
[FX1 VP Bring-up](../../guide/FX1_FREERTOS.md).

Kết quả smoke không thay kiểm driver/pixel ISP hay xác nhận silicon.
