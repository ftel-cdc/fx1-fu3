# Chạy Neon ISP

App: `examples/FX1_ISP_Image`. Dùng thư viện Neon và tuning riêng tại
`profile/profile.csrw`, không dùng profile của VP.

## 1. Chuẩn bị

Thực hiện [bước chuẩn bị terminal](FX1_FREERTOS.md#1-chuẩn-bị-terminal) từ
gốc SDK: Python local, compiler trên PATH và các biến `SDK_ROOT`,
`VP_ROOT`, `RAW_FILE`, `FX1_PYTHON`. Bước này dùng chung cho hai app ảnh.
Build ELF không cần repo VP hoặc ảnh RAW.

## 2. Sửa profile và build

Sửa `examples/FX1_ISP_Image/profile/profile.csrw` nếu cần, rồi chạy:

```sh
cd "$SDK_ROOT/examples/FX1_ISP_Image"
python3 tools/generate_profile.py
cmake --preset FX1 -DNEON_MDF_PYTHON3="$FX1_PYTHON"
cmake --build --preset FX1
```

Hoặc dùng `make build` để thực hiện cùng ba bước.

- Header tự sinh: `profile/profile.h` — không sửa tay.
- Firmware: `build/FX1/fx1_isp_image.elf`.
- Đổi profile phải sinh header và build lại ELF.

## 3. Chạy ảnh

```sh
"$VP_ROOT/vp_neon/run_vp_neon" \
  "$SDK_ROOT/examples/FX1_ISP_Image/build/FX1/fx1_isp_image.elf" \
  "$RAW_FILE"
```

Hai tham số là **ELF** và **RAW gốc**. Thay đường dẫn RAW để chạy ảnh khác.
RAW phải là **2688×1520 BGGR, RAW10 căn trái trong uint16 little-endian**.
Script tự chuyển RAW, chạy VP và xuất PNG; không cần copy ELF hay chuyển ảnh riêng.

## 4. Xem kết quả

Mở `$VP_ROOT/vp_neon/result/output.png`.

Cùng thư mục có `output.nv12`, `y.bin`, `uv.bin`, `input.isp16`, `vp.log`
và `report.json`. Terminal phải báo **PASS**; nếu lỗi, xem `vp.log`.
Kết quả cùng tên được ghi đè mỗi lần chạy.

Mặc định chạy **2 frame**, lấy frame cuối để GTM auto có trạng thái từ frame đầu.
Profile hiện tại xuất **1920×1080**. Tuning mặc định giữ bộ hệ số D65; ảnh khác
có thể cần tuning khác.

## 5. Clean

```sh
cd "$SDK_ROOT/examples/FX1_ISP_Image"
make clean
```

Xóa `build/FX1/` và cache; giữ source, profile và kết quả ảnh. Sau đó build lại
như bước 2.

## Tùy chọn thử nghiệm

Chạy các lệnh configure/build sau tại thư mục example:

```sh
# Một frame, giữ cùng profile
cmake --preset FX1 -B build/FX1-1frame -DFX1_IMAGE_FRAME_COUNT=1
cmake --build build/FX1-1frame
```

Truyền ELF trong thư mục build tương ứng cho `run_vp_neon`. Script tự lấy
frame cuối, hỗ trợ 1..4 frame. `make clean` chỉ xóa build mặc định `build/FX1/`.

BLC được chọn tại `profile/profile.csrw`: `BLC_CTRL 0x1` bật,
`BLC_CTRL 0x0` tắt. Sau khi sửa, chạy lại bước 2 để cập nhật ELF.

## Phạm vi profile và build tự động

CMake tự sinh `build/FX1/generated/profile.h` khi `.csrw` hoặc generator đổi;
đây là header compiler dùng. Header `profile/profile.h` do lệnh Python tạo là
bản xem trước. Vì vậy chỉ chạy build cũng không dùng nhầm tuning cũ.

Muốn build profile khác mà giữ profile mặc định:

```sh
cmake --preset FX1 -B build/custom -DFX1_ISP_PROFILE="$SDK_ROOT/examples/FX1_ISP_Image/profile/profile.csrw"
cmake --build build/custom
```

Tạo đủ `.elf`, `.elf.map`, `.elf.bin`; VP chạy ELF. BIN là binary thô,
chưa phải image BootROM/flash đóng gói.

Không phải mọi CSR profile đều hỗ trợ: schema hiện ánh xạ 62 tên thanh ghi,
BLC group 0, LSC resident 1/2, EE resident 0, Gamma đủ 256 mẫu, GTM auto và
CH0 NV12. Có thể đổi gain/CCM, các bảng và enable của khối trong schema.
Giữ đúng thứ tự lệnh nạp/confirm. Register lạ, LUT thiếu hoặc các mode chưa
hỗ trợ sẽ bị từ chối. Cấu hình còn phải qua kiểm tra tham số của driver khi chạy.
Input app vẫn cố định 2688×1520; runner nhận BGGR RAW10 căn trái. Đổi Bayer
trong profile phải tương ứng dữ liệu RAW (runner không tự nhận Bayer).
