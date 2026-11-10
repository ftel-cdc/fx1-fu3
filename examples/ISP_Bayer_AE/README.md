# FX1 ISP: init bare-metal, FreeRTOS và RAW/AEC

Example dùng preset **`FX1`**, tạo ELF và log trong `build/FX1/`.
Đọc [FX1 Quickstart](../../guide/FX1_FREERTOS.md) để chuẩn bị CMake >= 3.25,
Ninja, Python/PyYAML và cross-compiler. BSP/startup/linker đã ở trong Neon;
configure/build ELF không cần repo `fx1` hoặc binary VP.
Các bài frame/service tự tạo RAW tổng hợp, không cần bộ ảnh RAW sensor.

Lệnh chạy tập trung và cây thư mục có chú thích:
[Quickstart — FreeRTOS](../../guide/FX1_FREERTOS.md)
và [cây thư mục](../../guide/FX1_FREERTOS.md).

## 1. Configure lần đầu

Chuẩn bị `SDK_ROOT`, `FX1_PYTHON`, PATH và `NEON_FX1_VP_EXECUTABLE`
theo Quickstart. Từ gốc SDK, vào example:

```bash
cd examples/ISP_Bayer_AE

# Công cụ/PATH và FX1_PYTHON đã chuẩn bị theo Quickstart.
cmake --preset FX1 -DNEON_MDF_PYTHON3="$FX1_PYTHON" \
  -DNEON_FX1_VP_EXECUTABLE="$NEON_FX1_VP_EXECUTABLE"
```

Configure dùng BSP local và compiler đã nằm trên PATH,
tạo cache và thư mục build; chưa chạy firmware. Không cần khai báo SDK root
hoặc copy source từ gói VP.

**Vừa di chuyển repo hoặc đổi compiler:** thay lệnh configure bằng:

```bash
cmake --fresh --preset FX1 -DNEON_MDF_PYTHON3="$FX1_PYTHON"
```

Lệnh này tạo lại cache trong `build/FX1/`. Sau khi cấu hình,
sửa source chỉ cần build hoặc run lại.

## 2. Build `Src/main.c` thành ELF

Đứng trong `examples/ISP_Bayer_AE`, chạy:

```bash
cmake --build --preset FX1 --target isp_bayer_ae
```

ELF được tạo tại **`build/FX1/isp_bayer_ae.elf`**. Chương trình `Src/main.c`
khởi động FreeRTOS, tạo task rồi gọi init Bayer/AEC và đọc lại thanh ghi.
Build thành công xác nhận biên dịch/liên kết được; chưa xác nhận kiểm tra VP.

## 3. Build và chạy trên VP

Khi cần chạy, chọn riêng **file executable VP** (đổi theo vị trí trên máy):

```bash
cmake --preset FX1 \
  -DNEON_MDF_PYTHON3="$FX1_PYTHON" \
  -DNEON_FX1_VP_EXECUTABLE="$NEON_FX1_VP_EXECUTABLE"
cmake --build --preset FX1 --target run-vp
```

Target này phụ thuộc `isp_bayer_ae`: tự build cập nhật ELF nếu source thay
đổi, rồi dùng [run_vp.cmake](run_vp.cmake) gọi `fx1_soc --fw <ELF>`.
Không cần copy ELF sang repo VP. Có thể đặt binary trên PATH hoặc dùng
môi trường `NEON_FX1_VP_EXECUTABLE` trước lần configure mới. Nếu chưa chọn
VP, build ELF vẫn được; target run sẽ báo rõ thiếu executable.

Xem log của lần chạy:

```bash
cat build/FX1/platform.log
```

Kết quả mong đợi:

```text
[ftel_isp] BAYER_AEC_INIT PASS checks=16
[fx1_soc] RESULT PASS
```

Firmware kiểm tra 16 điều kiện init/readback và từ chối cấu hình sai; một
điều kiện sai sẽ gọi `fx1_fail()`. Chỉ khi đi hết các kiểm tra mới gọi
`fx1_pass()`. Runner yêu cầu tiến trình VP trả mã 0, marker của bài và
`RESULT PASS`; không kết luận từ một dòng PASS riêng lẻ. Bài `main.c`
chưa gửi RAW, chưa kiểm chứng pixel hoặc AE tự điều chỉnh exposure/gain.

## 4. Các bài riêng: bare-metal và frame RAW/AEC

```bash
# Src/main_baremetal.c: init không liên kết FreeRTOS.
cmake --build --preset FX1 --target run-init-vp

# Src/frame.c: RAW tổng hợp -> ISP/DMA -> CH0, đọc và kiểm tra AEC.
cmake --build --preset FX1 --target run-frame-vp
```

| Bài | Target chỉ build ELF | ELF trong `build/FX1/` | Log trong `build/FX1/` | Kết quả tham chiếu |
|---|---|---|---|---|
| Bare-metal init | `isp_init_baremetal` | `isp_init_baremetal.elf` | `baremetal_init.log` | 5 PASS |
| FreeRTOS init/readback | `isp_bayer_ae` | `isp_bayer_ae.elf` | `platform.log` | 16 PASS |
| RAW/AEC | `isp_bayer_aec_frame` | `isp_bayer_aec_frame.elf` | `frame_aec.log` | 7 frame, 114 PASS |
| Service FreeRTOS | `isp_freertos_service` | `isp_freertos_service.elf` | `service.log` | 37 PASS |

Bài frame sinh RAW bằng code, so thống kê AEC với công thức độc lập và
kiểm tra DDR. Chưa có golden màu toàn ảnh hoặc vòng feedback AE.

## 5. Service FreeRTOS: owner task, IRQ và recovery

```bash
cmake --build --preset FX1 --target isp_freertos_service
cmake --build --preset FX1 --target run-service-vp
cat build/FX1/service.log
```

Source: [Src/service.c](Src/service.c). Bài này dùng public API
`ftel_isp_freertos.h` từ `rtos/services/ftel_isp/include/`: một worker sở hữu
ISP, queue tối đa 3 ticket, chờ
frame bằng IRQ PLIC source 3 và task notification. Hai client mỗi bên chạy
8 frame, task nền vẫn tiến triển. Kiểm AEC global/histogram, frame identity,
RAW/output padding và guard DDR; chưa so golden pixel toàn ảnh.

Kiểm tra thêm buffer chồng lấn, caller hết hạn chờ nhưng giữ ticket, task
khác không được lấy ticket, generation chống kết quả cũ, cấu hình sai,
frame timeout, auto-recovery và queued job bị hủy khi service faulted.
Trường hợp recovery hết hạn dùng linker `--wrap` chỉ trong ELF test để
trì hoãn xác nhận recovery; buffer chỉ được release sau lần recovery sau
được driver xác nhận. Không thay đổi thư viện production hoặc model VP.

Log mong đợi:

```text
ISP_FREERTOS_SERVICE PASS checks=37
[fx1_soc] RESULT PASS
```

Chi tiết API và ownership: [hướng dẫn FreeRTOS](../../guide/FX1_FREERTOS.md#6-phạm-vi-và-ownership).

## 6. Các bước build

- `cmake --preset FX1`: **configure**, chọn MCU/toolchain/BSP local và tạo build.
- `cmake --build --preset FX1 --target ...`: **build**, hoặc build + chạy
  nếu target là `run-vp`, `run-init-vp`, `run-frame-vp`, `run-service-vp`.

Chi tiết [API, flow và phạm vi kiểm chứng](../../guide/FX1_FREERTOS.md#6-phạm-vi-và-ownership).


SDK xử lý Kconfig cho FX1; Python cần PyYAML và kconfiglib. Bài
`service.c` dùng job Bayer/AEC. Job toàn pipeline với profile được minh hoạ
trong `examples/FX1_ISP_Image_FreeRTOS`; hai loại dùng chung worker/queue.
Xem [phạm vi và ownership](../../guide/FX1_FREERTOS.md#6-phạm-vi-và-ownership).
