# Một ảnh D65: bật các khối hoặc bypass BLC/LSC

**Hướng dẫn từng bước:** [Build ELF, chép sang VP và xuất ảnh D65](../../guide/FX1_ELF_TO_VP.md). Bao gồm lệnh build, copy, chuyển RAW, chạy VP và kiểm tra PASS.


**Cập nhật:** mặc định `FX1_IMAGE_ALL_BLOCKS=ON`: bật BLC, LSC, BPC, WB, DG,
CCM, Gamma, GTM, 2DNR, EE, CNF và thống kê AEC/AWB/AF. Firmware đọc lại
14 bit enable và yêu cầu tất cả bật. IFMT/demosaic/CSC vẫn nằm trong pipeline.
Giữ CH0 NV12 full-size để so đúng ảnh trước; CH1/CH2 không bật thêm. Các vùng
D-WDR/3D-TNR dự trữ không có implementation trong pipeline driver hiện tại.
Tham số các khối giữ default; bật thống kê AWB/AF chưa phải thuật toán tự
cân bằng trắng/lấy nét. Không dùng calibration mới hay profile.bin.

Để build lại cấu hình cũ BLC/LSC OFF và các khối khác mặc định:

```sh
cmake --preset FX1 -DFX1_IMAGE_ALL_BLOCKS=OFF -DFX1_IMAGE_D65_PROFILE=OFF
cmake --build --preset FX1
```

Lần chạy mới ON trên cùng input D65 đạt `IMAGE_RESULT=PASS`, exit 0;
ảnh/log/ELF/report ở workspace `artifacts/all_blocks_on/`. Ảnh OFF trước đó
vẫn giữ tại `artifacts/blc_lsc_off/`. Cấu hình ON mới chạy một frame, chưa
kiểm chứng hội tụ GTM tự động qua nhiều frame hoặc so golden pixel.

Build độc lập từ thư mục example:

```sh
cmake --preset FX1 -DFX1_IMAGE_ALL_BLOCKS=ON
cmake --build --preset FX1
```

Chép `build/FX1/fx1_isp_image.elf` sang VP bằng tay. App này khác Frame tổng
hợp: cần một RAW đầu vào 2688×1520 BGGR, RAW12 right-aligned trong uint16 LE.
Mẫu `isp/d65/input.isp16` trong gói VP V3.0 đã đúng định dạng đó; không dịch
bit lần nữa. Không cần `profile.bin`: firmware tự cấu hình ISP.

Từ gốc gói `fx1-dev`, sau khi chép ELF vào `test-fw`:

```sh
./vp/bin/VP_FX1_V3.0/fx1_soc \
  --fw ./test-fw/fx1_isp_image.elf \
  --ddr-boot preinit --timeout-ms 2000 \
  --load 0x81000000:./sw/bootloader/test/VP_FX1_V3.0/isp/d65/input.isp16 \
  --dump 0x88000000:6128640:./test-fw/output.nv12 \
  > ./test-fw/vp.log 2>&1
```

Yêu cầu process exit 0, BLC_CTRL/LSC_CTRL đúng mode (ON: bit 0 bằng 1;
OFF: bit 0 bằng 0), `IMAGE_RESULT=PASS` và
`[fx1_soc] RESULT PASS`. Bài kiểm tra xác nhận một frame, tag, counters,
không có lỗi DMA ngoài INPUT_PAUSED khi kết thúc một frame; stop phải clear
cờ pause, DMA_ERR cuối bằng 0. BLC/LSC giữ trạng thái đã chọn sau stop. LSC vẫn nạp bảng
1/2 trong bước init để giữ hợp đồng init hiện tại, khối chỉ bypass khi chọn mode OFF.

Đổi NV12 sang PNG bằng công cụ độc lập (cần Pillow), không thuộc build:

```sh
python3 tools/nv12_to_png.py /path/to/output.nv12 /path/to/output.png
```

Đã chạy thành công 04/11/2026 với ELF build từ Neon, VP V3.0 và mẫu D65.
Kết quả lần thử lưu ở workspace `artifacts/blc_lsc_off/`: ELF, log, NV12,
PNG và report/hash/command. Cấu hình còn lại giữ mặc định driver; WB/Gamma
chưa được hiệu chuẩn nên ảnh tối và ngả xanh. Không dùng golden kèm mẫu VP
vì golden đó dùng profile khác. PASS xác nhận bypass và transport/frame,
chưa phải xác nhận chất lượng màu hoặc so pixel toàn ảnh với oracle.

## Profile D65 người dùng cung cấp

Mặc định mới: `FX1_IMAGE_D65_PROFILE=ON`. Header `Src/d65_profile.h` áp WB
UQ4.8 = 591/256/393, exposure_scale qua DG UQ4.8 = 557, CCM signed SQ2.9
= [904,-354,-39,-180,892,-201,-18,-331,861]. Chín giá trị CCM tương ứng đúng
hex trong CSV. Không nhân exposure vào CCM lần nữa.

```sh
cmake --preset FX1 -DFX1_IMAGE_ALL_BLOCKS=ON -DFX1_IMAGE_D65_PROFILE=ON
cmake --build --preset FX1
```

Muốn tái tạo ảnh all-blocks trước khi áp profile, configure
`-DFX1_IMAGE_ALL_BLOCKS=ON -DFX1_IMAGE_D65_PROFILE=OFF`.

Đã kiểm chứng readback WB/DG và cả chín hệ số CCM rồi chạy frame trên VP;
artifact ở `artifacts/d65_user_profile/`. Profile fit ban đầu dùng black level
RAW10=64; lần port này chỉ áp WB/exposure/CCM theo dòng CSV, giữ BLC/LSC và
Gamma/GTM mặc định của app. Vì khác pipeline và lượng tử hóa, không tuyên bố
ảnh sẽ giống công cụ fit hoặc đã đạt calibration sản phẩm. Không sửa file VP.
