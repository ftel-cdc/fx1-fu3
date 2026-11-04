# Build ELF Neon, chép sang VP và xuất ảnh D65

Để tái tạo ảnh giống lần chạy `run_vp.sh isp`, dùng **FX1_ISP_ProfileReplay**.
App nhúng toàn bộ profile CSR cùng WB/DG/CCM D65, chạy 2 frame và xuất
1920×1080. Build Neon độc lập; bạn chép file và gọi VP bằng tay.

| App | Driver/cấu hình | Đầu ra |
|---|---|---|
| `FX1_ISP_ProfileReplay` | Reference loader được chép vào app; full profile, INIT/SEAL/APPLY | 1920×1080, 2 frame; đã khớp NV12 với run_vp.sh |
| `FX1_ISP_Image` | Component `ftel_isp` Neon, legacy; WB/DG/CCM D65 và các khối khác mặc định | 2688×1520, 1 frame; màu có thể khác full profile |

ProfileReplay là app đối chiếu; chưa bổ sung profile mode vào component
`ftel_isp` Neon. Các bước 1–6 dưới đây dùng ProfileReplay.

## 1. Chuẩn bị công cụ

Cần CMake >= 3.25, Ninja, Python 3, kconfiglib, Pillow (xuất PNG) và GCC
RISC-V hỗ trợ `rv32imafc_zicsr_zifencei / ilp32f`. VP V3.0 cần chạy được
trên máy đích. Nếu chưa có các gói Python, cài trong môi trường Python của bạn:

```sh
python3 -m pip install cmake ninja kconfiglib pillow
export PATH="/opt/toolchains/riscv-none-elf/bin:$PATH"
```

Riêng máy workspace hiện tại đã cài tạm công cụ build trong `/tmp`, dùng:

```sh
export PYTHONPATH="/tmp/neon-build-tools${PYTHONPATH:+:$PYTHONPATH}"
export PATH="/tmp/neon-build-tools/bin:/opt/toolchains/riscv-none-elf/bin:$PATH"
```

Cách `/tmp` chỉ dùng khi thư mục đó còn tồn tại; nó không phải dependency
được ghi vào source Neon. Máy khác dùng đường dẫn compiler đã cài tại máy đó.

## 2. Build ELF trong Neon

```sh
cd /home/hieurose/Neon_fx1_test/neon_mdf-feature-porting_fx1/examples/FX1_ISP_ProfileReplay
cmake --preset FX1
cmake --build --preset FX1
```

Đầu ra trong `build/FX1/`:

```text
fx1_isp_profile_replay.elf       # file chép sang VP
fx1_isp_profile_replay.elf.map   # báo cáo bố trí bộ nhớ
fx1_isp_profile_replay.elf.bin   # binary thô, không dùng cho --fw
```

Profile đã nhúng trong `Src/profile_data.h`; không cần build `profile.bin`
hoặc chép `job.bin`. Nếu thay `Src/d65_full_profile.csrw`, sinh lại header rồi build:

```sh
python3 tools/embed_profile.py
cmake --build --preset FX1
```

Không cần build lại `fx1_soc` hay firmware FreeRTOS CLI để chạy ELF Neon.
Cảnh báo linker RWX là do layout DDR của BSP nhập vào; mục STACK 100% là vùng
stack được đặt trước, không phải số đo sử dụng stack thực tế.

## 3. Chép ELF và chuẩn bị RAW

Ví dụ dưới đây dành cho cùng workspace. Nếu VP ở máy khác, dùng thao tác
chép file của bạn và thay đường dẫn tương ứng.

```sh
cd /home/hieurose/Neon_fx1_test
mkdir -p fx1-dev/test-fw/d65-profile
cp neon_mdf-feature-porting_fx1/examples/FX1_ISP_ProfileReplay/build/FX1/fx1_isp_profile_replay.elf \
   fx1-dev/test-fw/d65-profile/
```

Ảnh gốc `D65_raw_2688x1520_5376.raw` là BGGR 2688×1520, RAW10 căn trái
trong uint16 little-endian. Firmware nhận RAW12 căn phải, nên chuyển mỗi mẫu
bằng `raw16 >> 4`. Lệnh sau giữ nguyên file gốc và kiểm tra kích thước/bit thấp:

```sh
python3 - <<'PY'
from pathlib import Path
import array, sys
src = Path('/home/hieurose/Neon_fx1_test/D65_raw_2688x1520_5376.raw')
dst = Path('/home/hieurose/Neon_fx1_test/fx1-dev/test-fw/d65-profile/input.isp16')
data = src.read_bytes()
if len(data) != 2688 * 1520 * 2:
    raise SystemExit('Sai kích thước RAW, cần 8171520 byte')
samples = array.array('H')
samples.frombytes(data)
if sys.byteorder != 'little':
    samples.byteswap()
if any(value & 63 for value in samples):
    raise SystemExit('Đầu vào không đúng RAW10 căn trái trong uint16')
converted = array.array('H', (value >> 4 for value in samples))
if sys.byteorder != 'little':
    converted.byteswap()
dst.write_bytes(converted.tobytes())
print(dst)
PY
```

Hoặc chép bản đã chuyển có sẵn sau lần kiểm thử:

```sh
cp fx1-dev/sw/bootloader/test/VP_FX1_V3.0/isp/d65/input.isp16 fx1-dev/test-fw/d65-profile/input.isp16
```

Chỉ dùng một trong hai cách. Không chuyển bit lần nữa nếu file đã là
`input.isp16` RAW12. Không nạp trực tiếp RAW gốc căn trái bằng `--load`.

## 4. Chạy trên VP

Chạy từ **gốc thư mục fx1-dev**:

```sh
cd /home/hieurose/Neon_fx1_test/fx1-dev
./vp/bin/VP_FX1_V3.0/fx1_soc \
  --fw ./test-fw/d65-profile/fx1_isp_profile_replay.elf \
  --ddr-boot preinit --timeout-ms 2000 \
  --load 0x81000000:./test-fw/d65-profile/input.isp16 \
  --dump 0x88000000:4085760:./test-fw/d65-profile/y.bin \
  --dump 0x88400000:2042880:./test-fw/d65-profile/uv.bin \
  > ./test-fw/d65-profile/vp.log 2>&1
rc=$?
echo "VP exit code: $rc"
cat ./test-fw/d65-profile/vp.log
```

`preinit` để VP khởi tạo DDR và nạp ELF trước khi CPU chạy. Không cần
bootloader trong ELF này. Không dùng `--ddr-boot firmware` cho ELF link vào DDR.
Không dùng `run_vp.sh isp` cho app này: app tự chạy khi vào main, không nhận CLI.

## 5. Biết firmware chạy đúng

Yêu cầu exit code **0**, đồng thời log có:

```text
CONFIG_RC = 0x00000000
TUNE_ERROR = 0x00000000
START_RC = 0x00000000
FRAME_DONE = 0x00000001
PROFILE_ID = 0x00000001
FRAME_DONE = 0x00000002
PROFILE_ID = 0x00000001
PROFILE_REPLAY_RESULT=PASS
[fx1_soc] RESULT PASS
```

Chỉ có FW_BOOT hoặc xuất được file dump chưa đủ kết luận PASS. Frame đầu làm
nóng GTM; ảnh dump cuối là kết quả sau frame thứ hai.

## 6. Chuyển dump thành NV12 và PNG

Vẫn từ gốc `fx1-dev`, chạy đoạn dưới (cần Pillow). Đầu ra có stride 2688 nhưng
chỉ 1920 pixel/hàng, vì vậy phải bỏ padding trước khi tạo ảnh.

### 6.1. Full-range BT.601

Đoạn dưới giải mã Y 0–255 và chroma toàn dải, tâm 128, bằng Pillow.
Nó tạo `output.nv12` và `output.png` (ảnh full-range). Full-range phù hợp
với cấu hình hiện tại theo CSR: `TUNE_CSC_CTRL.FULL_RANGE` reset bằng 1 và
profile không ghi đè; chưa có readback runtime của bit này.

```sh
python3 - <<'PY'
from pathlib import Path
from PIL import Image
import hashlib
folder = Path('test-fw/d65-profile')
w, h, stride = 1920, 1080, 2688
y_dump = (folder / 'y.bin').read_bytes()
uv_dump = (folder / 'uv.bin').read_bytes()
if len(y_dump) != 4085760 or len(uv_dump) != 2042880:
    raise SystemExit('Dump không đủ kích thước; kiểm tra log VP')
y = b''.join(y_dump[row*stride:row*stride+w] for row in range(h))
uv = b''.join(uv_dump[row*stride:row*stride+w] for row in range(h//2))
packed = y + uv
(folder / 'output.nv12').write_bytes(packed)
# Full-range: không trừ Y=16, không kéo giãn theo dải 219/224.
luma = Image.frombytes('L', (w, h), y)
u = Image.frombytes('L', (w//2, h//2), uv[0::2]).resize((w, h), Image.Resampling.NEAREST)
v = Image.frombytes('L', (w//2, h//2), uv[1::2]).resize((w, h), Image.Resampling.NEAREST)
Image.merge('YCbCr', (luma, u, v)).convert('RGB').save(folder / 'output.png')
print('PNG:', folder / 'output.png')
print('NV12 bytes:', len(packed))
print('SHA-256:', hashlib.sha256(packed).hexdigest())
PY
```

### 6.2. Limited-range BT.601 — đối chiếu preview của isp_pack.py

Chạy **sau bước 6.1**, vẫn từ gốc `fx1-dev`. Đoạn này đọc cùng `output.nv12`
đã bỏ padding, giải mã Y 16–235 và U/V 16–240 theo công thức `isp_pack.py`.
Chroma dùng nearest-neighbour, làm tròn một lần cuối. Ảnh ghi riêng thành
`output_limited.png`, không ghi đè `output.png` full-range.

Đây chỉ là hai cách giải mã cùng dữ liệu, không thay tham số ISP. Nếu dữ liệu
thực là full-range, giải mã limited có thể tăng tương phản và mất chi tiết
tối/sáng; nhìn sắc hơn không đồng nghĩa chính xác hơn.

```sh
python3 - <<'PY'
from pathlib import Path
from PIL import Image
folder = Path('test-fw/d65-profile')
w, h = 1920, 1080
packed = (folder / 'output.nv12').read_bytes()
if len(packed) != w * h * 3 // 2:
    raise SystemExit('NV12 sai kích thước; chạy bước 6.1 trước')
y, uv = packed[:w*h], packed[w*h:]
kr, kb = 0.299, 0.114  # BT.601, không phải BT.709.
kg = 1 - kr - kb
# Limited-range: chuẩn hóa Y bằng 255/219, chroma bằng 255/224.
ylut = [(value - 16) / 219.0 * 255 for value in range(256)]
r_v = [2 * (1 - kr) * (value - 128) / 224.0 * 255 for value in range(256)]
b_u = [2 * (1 - kb) * (value - 128) / 224.0 * 255 for value in range(256)]
g_v = [-kr * 2 * (1 - kr) * (value - 128) / 224.0 * 255 / kg for value in range(256)]
g_u = [-kb * 2 * (1 - kb) * (value - 128) / 224.0 * 255 / kg for value in range(256)]
clip = bytes(0 if value < 0 else 255 if value > 255 else value
             for value in range(-1024, 1280))
rgb = bytearray(w * h * 3)
for row in range(h):
    for col in range(0, w, 2):
        offset = (row // 2) * w + col
        u, v = uv[offset], uv[offset + 1]
        # Giữ thứ tự phép cộng và làm tròn giống isp_pack.py.
        rv, bu, gc = r_v[v], b_u[u], g_v[v] + g_u[u]
        for x in (col, col + 1):
            yy = ylut[y[row*w + x]]
            dest = (row*w + x) * 3
            rgb[dest] = clip[int(yy + rv + 1024.5)]
            rgb[dest+1] = clip[int(yy + gc + 1024.5)]
            rgb[dest+2] = clip[int(yy + bu + 1024.5)]
Image.frombytes('RGB', (w, h), bytes(rgb)).save(folder / 'output_limited.png')
print('PNG:', folder / 'output_limited.png')
PY
```

Hai ảnh nằm trong `fx1-dev/test-fw/d65-profile/`: `output.png` (full-range)
và `output_limited.png` (limited-range). File `output.nv12` không bị thay đổi.

Với đúng RAW D65, profile nhúng hiện tại và VP đã kiểm chứng, packed NV12
có 3110400 byte và SHA-256:

```text
4822cf3199fef425980e47a2c357e97745517f3f39ef50066c248224ba912627
```

Hash này đã khớp kết quả `run_vp.sh` cùng profile. Đổi ảnh, tuning hoặc model
VP có thể đổi hash; đây là bằng chứng hồi quy của cấu hình cụ thể, không phải
oracle độc lập hay kết luận calibration sản phẩm.

## 7. Nếu muốn dùng driver ftel_isp của Neon

Dùng app đang mở trong IDE, `FX1_ISP_Image`:

```sh
cd /home/hieurose/Neon_fx1_test/neon_mdf-feature-porting_fx1/examples/FX1_ISP_Image
cmake --preset FX1 -DFX1_IMAGE_ALL_BLOCKS=ON -DFX1_IMAGE_D65_PROFILE=ON
cmake --build --preset FX1
```

Chép `build/FX1/fx1_isp_image.elf` vào `fx1-dev/test-fw/d65-profile/`. Dùng cùng
RAW12 đã chuyển, chạy từ gốc `fx1-dev`:

```sh
./vp/bin/VP_FX1_V3.0/fx1_soc \
  --fw ./test-fw/d65-profile/fx1_isp_image.elf \
  --ddr-boot preinit --timeout-ms 2000 \
  --load 0x81000000:./test-fw/d65-profile/input.isp16 \
  --dump 0x88000000:6128640:./test-fw/d65-profile/image.nv12 \
  > ./test-fw/d65-profile/image.log 2>&1
rc=$?
echo "VP exit code: $rc"
cat ./test-fw/d65-profile/image.log
```

Yêu cầu exit 0, `IMAGE_RESULT=PASS` và `[fx1_soc] RESULT PASS`. Đây là packed
NV12 2688×1520, dùng `examples/FX1_ISP_Image/tools/nv12_to_png.py` để xuất PNG:

```sh
python3 /home/hieurose/Neon_fx1_test/neon_mdf-feature-porting_fx1/examples/FX1_ISP_Image/tools/nv12_to_png.py \
  ./test-fw/d65-profile/image.nv12 ./test-fw/d65-profile/image.png
```

App này chỉ áp WB/DG/CCM D65 trên cấu hình còn lại mặc định của driver,
không tái hiện full profile; không dùng hash hoặc kích thước của ProfileReplay
để đánh giá output này.
