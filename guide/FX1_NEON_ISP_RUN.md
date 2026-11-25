# Chạy Neon ISP

App: `examples/FX1_ISP_Image`. Dùng thư viện Neon và tuning riêng tại
`profile/profile.csrw`, không dùng profile của VP.

Example hiện bật binary 3A trong `sdkconfig`. Xem
[tích hợp 3A](../examples/FX1_ISP_Image/README.md#tích-hợp-binary-3a) để biết vị
trí `.a`/hai `.bin`, chế độ replay, AWB held và lựa chọn thử unscreened AWB.
Pipeline vẫn dùng `.csrw`; startup WB lấy từ tuning binary khi 3A được bật.

## Contract CSR/MAS ngày 24/11/2026

Source hiện nhắm CSR XLSX 626 thanh ghi/886 field và MAS ngày 24/11/2026.
`COMMON_VER_ID` vẫn là `0x00010007`; driver còn yêu cầu capability INPUT_PAD
và INPUT_STREAM. VP cũ 621/867 chưa chạy được firmware này. Các kết quả VP
ngày 23/11 ở cuối trang là baseline trước cập nhật, không xác nhận contract mới.
Chỉ build và chạy host checks trong lúc chờ VP mới; chưa xác nhận chất lượng ảnh,
handshake stream hay timing/recovery trên VP mới.

- IFMT: `ifmt.crop=false` mặc định padding cùng màu CFA; DDR giữ kích thước
  chẵn có giới hạn 3840×2160. AXI4-Stream không replay được hàng parity cuối:
  pad-height là `min(H-SV,2160)&~1`. `crop=true` giữ geometry legacy.
- Hai profile example đặt rõ `COMMON_INPUT_FORMAT 0x4` (RAW12, crop) để giữ
  geometry lưới AEC/AWB hiện có. Chuyển sang pad cần tính lại vùng thống kê,
  crop/resize và dữ liệu UE/OE ROI; không tái dùng fixture một cách mặc định.
- CNF dùng `CNF_GF_EPS` 16 bit, đơn vị luma². Example chọn startup default 200;
  đây không phải phép chuyển đổi tương đương từ hai threshold cũ. Generator từ
  chối `CNF_CHROMA_TH`/`CNF_LUMA_TH`; profile bên ngoài cần chuyển rõ ràng.
- LSC: UQ3.18 nonzero, unity `0x40000`, tối đa `0x1FFFFF`. CCM matrix/offset
  signed Q3.9; offset code 512 cộng một mức pixel. Không dùng quy ước offset
  theo pixel trực tiếp của extension VP cũ.
- `ifmt.source=IFMT_SOURCE_STREAM`, `ifmt.drop` chọn nguồn và hold/drop.
  `ifmt_read_stream_status()` đọc lỗi/counter; clear lỗi stream và DMA_ERR.STREAM
  là hai thao tác riêng. `isp_init()` cấu hình nguồn và bỏ IDMA buffers/stride
  khi stream. API submit/release `isp_stream_*` vẫn dành cho DDR; caller live
  quản lý admission, output ownership và bật DMA riêng sau init. `DMA_CTRL.IDMA_EN`
  bật đường vào cho cả hai nguồn; không đánh dấu IDMA buffer VALID khi live.
- `aec_global_stats_t` bổ sung `ue_percent_q8`/`oe_percent_q8` của frame leased;
  100% = `0x6400`. Đây là thống kê AEC toàn frame, không thay UE/OE từng ROI AWB.
- Chờ `LSC_PROFILE_STATUS.PREP_BUSY` trước ghi IDLE kế tiếp; chờ
  `TUNE_STATUS.CHECK_BUSY` sau xác nhận bảng. Dùng COMMON_STATUS.BUSY khi chờ
  hết frame; BUSY riêng của block không đảm bảo toàn pipeline đã drain.

Từ gốc SDK, đặt `CSR_FILE` trỏ tới workbook mới rồi tái sinh:

```sh
python3 neon-mdf/mcu-description/tools/import_fx1_isp.py \
  --csr "$CSR_FILE" \
  --regmap neon-mdf/arch/RISCV/Device/FTEL/FX1/Include/fx1_isp/fx1_isp_csr.h \
  --generate-regmap --driver-regmaps neon-mdf/components/driver/isp/registers
python3 neon-mdf/mcu-description/tools/generate.py --device FX1
python3 neon-mdf/mcu-description/tools/check_fx1.py --csr "$CSR_FILE"
make -C neon-mdf/tests/camera_hal test
```

Kiểm tra phần mềm ngày 25/11/2026:

- CSR trực tiếp ↔ regmap ↔ YAML ↔ SVD: PASS, 626 register/886 field,
  10.283 hằng số C; tái sinh lần hai khớp từng byte.
- Host camera HAL và contract mới: PASS, 1.255 checks, 0 lỗi; gồm 82 checks
  cho geometry, giới hạn hệ số, capability, timeout, stream readout và AEC lease.
- Generator từ chối profile CNF cũ, LSC zero/8x và register không ánh xạ.
- GCC RISC-V 15.2: ISP Image 3A bật/5 frame, Image 3A tắt/1 frame với camera
  runtime bật, Image FreeRTOS và các target ISP_Bayer_AE đều cross-build PASS.
- Chưa chạy firmware mới trên VP. Các kiểm tra host dùng MMIO fixture, không
  xác nhận timing/handshake của RTL hoặc VP mới.

Khi VP mới sẵn sàng, kiểm chứng pad/crop cả bốn CFA, DDR/stream hold/drop,
short/long line và early SOF, IRQ stream bit 7, timeout/check completion,
LSC zero/max/active-load rejection, AEC lease/Q7.8 và DMA recovery. Sau đó chạy
A/A_awb 5 frame, lưu từng frame riêng và so ảnh với baseline cũ; CNF mới có
thuật toán khác nên không yêu cầu ảnh khớp byte.

## 1. Chuẩn bị

Thực hiện [bước chuẩn bị terminal](FX1_FREERTOS.md#1-chuẩn-bị-terminal) từ
gốc SDK: Python local, compiler trên PATH và các biến `SDK_ROOT`,
`VP_ROOT`, `RAW_FILE`, `FX1_PYTHON`. Bước này dùng chung cho hai app ảnh.
Build ELF không cần repo VP hoặc ảnh RAW.

Trong workspace `Neon_fx1`, repo VP nằm tại `fx1/`:

```sh
export SDK_ROOT=/home/hieurose/Neon_fx1/neon_mdf
export VP_ROOT="$(cd "$SDK_ROOT/../fx1" && pwd)"
```

Đặt `RAW_FILE` trỏ tới ảnh RAW gốc muốn chạy.

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
RAW phải là **2688×1520 BGGR, uint16 little-endian**, đúng **8.171.520 byte**.
Lệnh trên mặc định nhận RAW10 căn trái. Các định dạng được hỗ trợ:

| `--input-format` | Dữ liệu trong uint16 LE | Chuyển sang RAW12 để nạp ISP |
|---|---|---|
| `raw10-left` (mặc định) | RAW10 ở bit 15..6, bit 5..0 bằng 0 | Dịch phải 4 bit |
| `raw10` | RAW10 ở bit 9..0, giá trị 0..1023 | Dịch trái 2 bit |
| `raw12` | RAW12 ở bit 11..0, giá trị 0..4095 | Giữ nguyên |

Ví dụ khi `RAW_FILE` chứa RAW12 căn phải:

```sh
"$VP_ROOT/vp_neon/run_vp_neon" --input-format raw12 \
  "$SDK_ROOT/examples/FX1_ISP_Image/build/FX1/fx1_isp_image.elf" \
  "$RAW_FILE"
```

Script tự chuyển RAW, chạy VP và xuất PNG; không cần copy ELF hay chuyển ảnh riêng.

## 4. Xem kết quả

Mở `$VP_ROOT/vp_neon/result/output.png`.

Cùng thư mục có `output.nv12`, `y.bin`, `uv.bin`, `input.isp16`, `vp.log`
và `report.json`. Terminal phải báo **PASS**; nếu lỗi, xem `vp.log`.
Kết quả cùng tên được ghi đè mỗi lần chạy.

Mặc định chạy **2 frame**, lấy frame cuối để GTM auto có trạng thái từ frame đầu.
Profile mặc định xuất **1920×1080** và dùng bộ hệ số D65; ảnh khác
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

BLC dùng scale riêng từng kênh. Profile mặc định chọn group G0 bằng
`BLC_GAIN_SEL 0x100` và đặt scale như sau:

```text
BLC_SCALE_G0_R  0x11112
BLC_SCALE_G0_GR 0x11112
BLC_SCALE_G0_GB 0x11112
BLC_SCALE_G0_B  0x11112
```

## Phạm vi profile và build tự động

CMake tự sinh `build/FX1/generated/profile.h` khi `.csrw` hoặc generator đổi;
đây là header compiler dùng. Header `profile/profile.h` do lệnh Python tạo là
bản xem trước. Khi profile thay đổi, lệnh build tự sinh lại header được biên dịch.

Muốn build profile khác mà giữ profile mặc định:

```sh
cmake --preset FX1 -B build/custom -DFX1_ISP_PROFILE="$SDK_ROOT/examples/FX1_ISP_Image/profile/profile.csrw"
cmake --build build/custom
```

Tạo đủ `.elf`, `.elf.map`, `.elf.bin`; VP chạy ELF. BIN là binary thô,
chưa phải image BootROM/flash đóng gói.

Profile mặc định dùng 65 tên thanh ghi. Generator hỗ trợ
BLC group 0, LSC resident 1/2, EE resident 0, Gamma đủ 256 mẫu, GTM auto và
CH0 NV12. Có thể đổi gain/CCM, các bảng và enable của khối trong schema.
Giữ đúng thứ tự lệnh nạp/confirm. Register lạ, LUT thiếu hoặc các mode chưa
hỗ trợ sẽ bị từ chối. Cấu hình còn phải qua kiểm tra tham số của driver khi chạy.
Driver BLC hỗ trợ G0..G7; generator profile của app ánh xạ G0.
LSC dùng UQ3.18, hệ số hợp lệ `0x000001..0x1FFFFF` (nhỏ hơn 8x).
Input app cố định 2688×1520; runner hỗ trợ ba cách đóng gói RAW ở mục 3.
Đổi Bayer trong profile phải tương ứng dữ liệu RAW (runner không tự nhận Bayer).


## Kiểm chứng sau port ISP và 3A (23/11/2026)

Ba thay đổi được tích hợp từ `349b493`, `c5536af`, `c95aa1b`; không lấy merge commit
`46f4fb1`. Giữ CSI MAS v1.4 của nhánh drivers. ISP dùng 621 register, 867 field;
BLC có 8 profile và scale riêng R/Gr/Gb/B. Metadata được hợp nhất trước khi sinh lại header/SVD.

- `python3 neon-mdf/mcu-description/tools/check_fx1.py`: PASS, 10.143 hằng số.
- Cross-build: ISP Image với 3A bật/tắt, 1/5 frame, strict, unscreened; Image FreeRTOS,
  ISP_Bayer_AE và cấu hình bật camera runtime đều PASS.
- VP: 3A 1/2/5 frame, unscreened 4 frame và executable fixture AWB PASS.
  Strict từ chối init khi binary IMX675 không khớp mode RAW của app.
- Bốn bài VP của ISP_Bayer_AE (init bare-metal, init FreeRTOS, frame AEC, service) PASS.
- FreeRTOS platform smoke build được nhưng VP dừng ở check 20,
  `masked PLIC source stays pending` (FAIL `0xf414`). Đã tái hiện cùng lỗi trên
  bản sạch `dc08f7e` trước port; chưa sửa trong phạm vi tích hợp ISP/3A.
- Ảnh D65 tĩnh hai frame của bản port bare-metal, FreeRTOS và source `c95aa1b`
  khớp từng byte. Hash và phiên bản VP nằm trong README của FX1_ISP_Image.

Binary 3A hiện thuộc đường `ftel_isp`; camera runtime vẫn dùng 3A reference.
Exposure/gain trong replay là đề xuất, không xác nhận sensor đã áp dụng.
Runner ngoài SDK `fx1/vp_neon/run_vp_neon` chỉ xuất ảnh 1..4 frame; firmware hỗ trợ
hơn 4 frame bằng quay vòng DMA slot. Bài 5 frame được kiểm tra bằng VP trực tiếp.
