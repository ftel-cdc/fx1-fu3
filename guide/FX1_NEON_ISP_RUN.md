# Chạy Neon ISP

App: `examples/FX1_ISP_Image`. Dùng thư viện Neon và tuning riêng tại
`profile/profile.csrw`, không dùng profile của VP.

## 1. Chuẩn bị

Cần CMake ≥ 3.25, Ninja, GCC RISC-V, Python có `kconfiglib` và `Pillow`.
Đổi đường dẫn dưới đây theo máy của bạn:

```sh
export NEON_WORKSPACE=/home/hieurose/Neon_fx1_test
export PATH="/opt/toolchains/riscv-none-elf/bin:$PATH"
```

## 2. Sửa profile và build

Sửa `examples/FX1_ISP_Image/profile/profile.csrw` nếu cần, rồi chạy:

```sh
cd "$NEON_WORKSPACE/neon_mdf-feature-porting_fx1/examples/FX1_ISP_Image"
python3 tools/generate_profile.py
cmake --preset FX1
cmake --build --preset FX1
```

Hoặc dùng `make build` để thực hiện cùng ba bước.

- Header tự sinh: `profile/profile.h` — không sửa tay.
- Firmware: `build/FX1/fx1_isp_image.elf`.
- Đổi profile phải sinh header và build lại ELF.

## 3. Chạy ảnh

```sh
cd "$NEON_WORKSPACE"
./fx1-dev/vp_neon/run_vp_neon \
  neon_mdf-feature-porting_fx1/examples/FX1_ISP_Image/build/FX1/fx1_isp_image.elf \
  D65_raw_2688x1520_5376.raw
```

Hai tham số là **ELF** và **RAW gốc**. Thay đường dẫn RAW để chạy ảnh khác.
RAW phải là **2688×1520 BGGR, RAW10 căn trái trong uint16 little-endian**.
Script tự chuyển RAW, chạy VP và xuất PNG; không cần copy ELF hay chuyển ảnh riêng.

## 4. Xem kết quả

Mở `fx1-dev/vp_neon/result/output.png`.

Cùng thư mục có `output.nv12`, `y.bin`, `uv.bin`, `input.isp16`, `vp.log`
và `report.json`. Terminal phải báo **PASS**; nếu lỗi, xem `vp.log`.
Kết quả cùng tên được ghi đè mỗi lần chạy.

Mặc định chạy **2 frame**, lấy frame cuối để GTM auto có trạng thái từ frame đầu.
Profile hiện tại xuất **1920×1080**. Tuning mặc định giữ bộ hệ số D65; ảnh khác
có thể cần tuning khác.

## 5. Clean

```sh
cd "$NEON_WORKSPACE/neon_mdf-feature-porting_fx1/examples/FX1_ISP_Image"
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
cmake --preset FX1 -B build/custom -DFX1_ISP_PROFILE=/duong/dan/profile.csrw
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
