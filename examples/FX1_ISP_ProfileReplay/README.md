# ELF nhúng toàn bộ profile D65, đối chiếu run_vp.sh

**Hướng dẫn từng bước:** [Build ELF, chép sang VP và xuất ảnh D65](../../guide/FX1_ELF_TO_VP.md). Bao gồm lệnh build, copy, chuyển RAW, chạy VP và kiểm tra PASS.

App này chạy bare-metal trong Neon, nhúng 1128 CSR writes của shared profile
và ghi đè WB/DG/CCM theo dòng D65 người dùng cung cấp. Bảng LSC/Gamma/GTM/EE,
các khối khác, profile mode và resize giữ giống lần chạy run_vp.sh đã lưu.
WB Q8 = 591/256/393; DG Q8 = 557; CCM hex =
388/E9E/FD9/F4C/37C/F37/FEE/EB5/35D. Exposure chỉ áp một lần qua DG.

Đây là **app đối chiếu dùng reference driver snapshot** trong Reference/.
Nó dùng startup/linker/runtime của Neon, không dùng FreeRTOS hoặc CLI. Nó không
link component ftel_isp legacy và không chứng minh component đó đã hỗ trợ
profile mode. File nguồn/hash reference có ở Reference/PROVENANCE.json.
Không có đường dẫn hoặc dependency build tới fx1-dev. Không sửa source VP.

## Build

Từ thư mục example, với dependency như guide/FX1_Quickstart.md:

```sh
cmake --preset FX1
cmake --build --preset FX1
```

Chép `build/FX1/fx1_isp_profile_replay.elf` sang VP bằng tay. Không cần chép
job.bin/profile.bin vì cả job lẫn profile đã nhúng trong ELF.
File profile nguồn cục bộ: Src/d65_full_profile.csrw. Sau khi chỉnh bảng này,
chạy `python3 tools/embed_profile.py` rồi build lại; generator không cần VP.

## Input và chạy

Ảnh gốc `D65_raw_2688x1520_5376.raw` là RAW10 căn trái trong uint16 LE.
Đổi thành RAW12 căn phải bằng dịch từng mẫu uint16 `>> 4`, không sửa ảnh gốc.
Có thể dùng mẫu cùng dữ liệu `sw/bootloader/test/VP_FX1_V3.0/isp/d65/input.isp16` trong gói VP, hoặc chuyển từ RAW gốc theo `guide/FX1_ELF_TO_VP.md`.
Không nạp trực tiếp ảnh gốc căn trái vào firmware này.

Từ gốc gói VP, với ELF và input.isp16 đã tự chép vào test-fw:

```sh
./vp/bin/VP_FX1_V3.0/fx1_soc \
  --fw ./test-fw/fx1_isp_profile_replay.elf \
  --ddr-boot preinit --timeout-ms 2000 \
  --load 0x81000000:./test-fw/input.isp16 \
  --dump 0x88000000:4085760:./test-fw/y.bin \
  --dump 0x88400000:2042880:./test-fw/uv.bin
```

Yêu cầu exit 0, CONFIG_RC/TUNE_ERROR/START_RC bằng 0, FRAME_DONE lần lượt
1 và 2, PROFILE_ID=1, PROFILE_REPLAY_RESULT=PASS và [fx1_soc] RESULT PASS.
Đầu ra resize 1920x1080, stride 2688. Để tạo packed NV12: lấy 1920 byte đầu
mỗi hàng trong 1080 hàng Y và 540 hàng UV; ghép Y rồi UV. Decode full-range
BT.601 giống isp_pack.py. Không ghép nguyên toàn bộ y.bin/uv.bin thành ảnh.

## Kết quả kiểm chứng

04/11/2026: chạy đúng RAW D65 đã chuyển, profile mode, 2 frame. Packed NV12
3110400 byte khớp từng byte với kết quả run_vp.sh cùng profile. SHA-256:
`4822cf3199fef425980e47a2c357e97745517f3f39ef50066c248224ba912627`.
Ảnh/log/ELF và báo cáo ở workspace `artifacts/d65_full_profile_elf/`.
Đây là đối chiếu hai đường thực thi trên cùng VP, chưa phải oracle độc lập
hoặc xác nhận chất lượng màu trên silicon.

## Thử tắt riêng BLC và LSC

Giữ nguyên profile nguồn và build một variant riêng:

```sh
cmake --preset FX1 -B build/FX1-blc-lsc-off -DFX1_PROFILE_BYPASS_BLC_LSC=ON
cmake --build build/FX1-blc-lsc-off
```

Firmware sao chép CSR stream và chỉ clear bit enable của BLC_CTRL/LSC_CTRL
trước khi nạp/seal profile. Các LUT/hệ số vẫn được nạp; hai khối bị bypass.
Option mặc định OFF, bản build FX1 thông thường giữ profile bật BLC/LSC.
Dùng ELF trong `build/FX1-blc-lsc-off/` với cùng lệnh load/dump ở trên.
Đã chạy 2 frame PASS, BLC_CTRL/LSC_CTRL readback 0. Kết quả lần thử tại
`fx1-dev/test-fw/d65-profile/blc-lsc-off/`: PNG full/limited, NV12 và log;
ảnh limited để so với preview limited của run_vp.

## Thử chỉ tắt LSC, giữ BLC bật

```sh
cmake --preset FX1 -B build/FX1-lsc-off -DFX1_PROFILE_BYPASS_LSC=ON
cmake --build build/FX1-lsc-off
```

Dùng ELF từ `build/FX1-lsc-off/`, giữ nguyên lệnh load/dump. Không bật đồng
thời option bypass BLC+LSC. Chỉ bit enable của LSC_CTRL bị clear, các tham số
còn lại giữ nguyên; readback phải BLC_CTRL=1, LSC_CTRL=0. Lần thử đã đạt
2 frame PASS; ảnh/log nằm tại `fx1-dev/test-fw/d65-profile/lsc-off/`.

## Tắt từng khối riêng lẻ

Chọn một trong: BLC, LSC, BPC, WB, DG, CCM, GAMMA, GTM, NR_2D, EE,
CNF, AEC, AWB, AF, RESIZER. Ví dụ chỉ tắt Gamma:

```sh
cmake --preset FX1 -B build/single-off-gamma -DFX1_PROFILE_DISABLE_BLOCK=GAMMA
cmake --build build/single-off-gamma
```

Không kết hợp với hai option bypass cũ. Firmware chỉ clear bit enable trong
các writes của đúng khối đã chọn (Gamma có cả write xác nhận LUT và write
bật); các bit khác và LUT/hệ số giữ nguyên. Nó kiểm tra readback khối đó OFF
và 14 khối còn lại ON trước khi chạy frame. Giá trị NONE giữ full profile.
IFMT, demosaic, CSC là các bước bắt buộc; DMA/output phải hoạt động để xuất
ảnh nên không đưa vào thử nghiệm tắt. CH1/CH2 vẫn tắt như baseline.

Kết quả batch ở `fx1-dev/test-fw/d65-profile/single-block-off/`: mỗi khối có
ELF, build.log, vp.log, output.nv12, output_limited.png và report.json. summary.json
lưu PASS/FAIL từng ca; comparison.jpg/index.html để xem ảnh cạnh nhau. PNG dùng
limited-range giống preview run_vp, không sửa tuning để làm đẹp. Khi tắt
RESIZER, kích thước ảnh đổi theo geometry thực; đọc OUTPUT_WIDTH/HEIGHT
trong log thay vì cố định 1920x1080. AEC/AWB/AF là thống kê, không phải các
thuật toán tự động điều chỉnh phơi sáng, WB hoặc thấu kính.
