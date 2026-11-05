# FX1_ISP_Image — Neon ISP với profile ứng dụng

App dùng `neon_mdf_ftel_isp`, mặc định nạp full tuning D65, bật các khối xử lý
và chạy 2 frame. RAW đầu vào 2688×1520 BGGR RAW12; output NV12 1920×1080,
stride 2688. Không cần các cờ FULL_TUNING, ALL_BLOCKS hoặc D65_PROFILE.

[Hướng dẫn đầy đủ: build, chuyển RAW, chạy VP và xuất PNG](../../guide/FX1_NEON_ISP_RUN.md).

Từ thư mục example:

```sh
python3 tools/generate_profile.py
cmake --preset FX1
cmake --build --preset FX1
```

ELF nằm ở `build/FX1/fx1_isp_image.elf`. Tuning nằm tại
`profile/profile.csrw`; generator tạo `profile/profile.h` để điền
`isp_config_t`. Không biên dịch Reference driver, không dùng SEAL/APPLY.

Chạy một frame, giữ cùng tuning:

```sh
cmake --preset FX1 -B build/FX1-1frame -DFX1_IMAGE_FRAME_COUNT=1
cmake --build build/FX1-1frame
```

Bật/tắt BLC bằng `BLC_CTRL 0x1` / `BLC_CTRL 0x0` trong
`profile/profile.csrw`, rồi sinh lại header và build. Không dùng cờ CMake. Số frame mặc định 2, có thể chọn 1..4; phải nạp RAW cho từng
slot được chạy. Thư viện quản lý các slot không chồng bộ nhớ.

Full tuning hai frame đã kiểm chứng NV12 khớp từng byte với ProfileReplay và
PNG khớp toàn bộ pixel với `image/elf_all_on_output.png`. Hash NV12:
`4822cf3199fef425980e47a2c357e97745517f3f39ef50066c248224ba912627`.
Một frame có hash khác do trạng thái GTM auto ban đầu. Xem hướng dẫn để chọn
đúng địa chỉ load/dump theo số frame.

## Dọn build

Từ thư mục `examples/FX1_ISP_Image`:

```sh
make clean
```

Xóa `build/FX1/`, gồm ELF/MAP/BIN và CMake cache. Giữ nguyên
source, tuning, header đã sinh và ảnh/log trong `artifacts/`. Các build folder
thử nghiệm khác không bị xóa. Sau đó chạy lại ba lệnh build ở trên;
`make build` cũng thực hiện cùng ba bước sinh header, configure và build.

## Profile độc lập của ứng dụng

- `profile/profile.csrw`: sửa tuning tại đây.
- `profile/profile.h`: được sinh bởi `tools/generate_profile.py`.

App không đọc CSR profile của VP hoặc ProfileReplay. Profile mặc định giữ
bộ hệ số đã kiểm chứng trên D65. Thay các tham số được API Neon hỗ trợ,
sinh lại header rồi build. Tên profile chung không tự tạo calibration cho
mọi nguồn sáng; hãy chọn tuning phù hợp với ảnh.

Cùng ELF chạy được các RAW khác nhau cùng định dạng BGGR RAW10 căn trái,
2688×1520 bằng `run_vp_neon ELF RAW`. Đổi tuning phải build lại ELF.
Kích thước/Bayer/depth đầu vào vẫn được đặt trong main.c, chưa tự nhận mọi
độ phân giải hoặc định dạng RAW.

## Kiểm tra profile lúc build

CMake tự sinh và dùng `build/FX1/generated/profile.h`; không phụ thuộc việc
nhớ chạy Python trước. Có thể chọn file riêng bằng `-DFX1_ISP_PROFILE=/path/profile.csrw`.
Xem phạm vi schema và các mode bị từ chối trong hướng dẫn. BIN là binary thô;
VP dùng ELF. Các example FX1 khác đã được xóa, chỉ giữ FX1_ISP_Image.
