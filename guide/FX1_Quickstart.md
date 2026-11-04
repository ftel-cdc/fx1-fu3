# FX1 ISP trong Neon MDF — build độc lập

Target `FX1` dùng bản sao BSP VP_FX1_V3.0 nằm trong SDK. Không cần thư mục
`fx1-dev`, project standalone `FX1_ISP`, executable VP hoặc dataset để build.
Build không gọi VP, không chép file ra ngoài SDK và không flash thiết bị.

## Chuẩn bị

Cần CMake >= 3.25, Ninja, Python 3 với `kconfiglib`, và GCC bare-metal RISC-V
hỗ trợ `rv32imafc_zicsr_zifencei / ilp32f` (đã build bằng xPack 15.2.0).
Đặt thư mục bin compiler trong PATH, ví dụ:

```sh
export PATH="/opt/toolchains/riscv-none-elf/bin:$PATH"
python3 -m pip install cmake ninja kconfiglib
```

Nếu đã có các công cụ này thì không cần cài lại. Kiểm tra `cmake --version`,
`ninja --version`, `riscv-none-elf-gcc --version`, `python3 -c 'import kconfiglib'`.
Có thể đổi prefix bằng `-DTOOLCHAIN_PREFIX=<prefix>` lúc configure, với build
folder mới khi đổi compiler. Không cần export NEON_MDF_PATH cho các app dưới đây.

## Build

Từ gốc repository Neon:

```sh
cd examples/FX1_ISP_Init
cmake --preset FX1
cmake --build --preset FX1
```

| App | Firmware trong `build/FX1/` | Chức năng |
|---|---|---|
| `FX1_Smoke` | `fx1_smoke.elf` | Startup, console, main/exit; tắt component ISP |
| `FX1_ISP_Init` | `fx1_isp_init.elf` | Init tổng với lựa chọn LSC 1/2 tường minh |
| `FX1_ISP_FullInit` | `fx1_isp_fullinit.elf` | Các ca init/re-init/sai cấu hình, giữ verdict giới hạn LSC 0 |
| `FX1_ISP_Frame` | `fx1_isp_frame.elf` | Tự sinh RAW12 64×48, 5 frame qua slot 0/1/2/3/0, CH0 NV12 |

Mỗi ELF có thêm `<tên>.elf.map` và `<tên>.elf.bin`. App ISP bật
`CONFIG_NEON_MDF_FTEL_ISP=y` trong sdkconfig. Muốn thay option dùng
`cmake --build build/FX1 --target menuconfig`, sau đó configure lại.
Chỉ dùng preset FX1 cho những app này.

App Frame tạo RAW và seed guard ngay trong firmware, không cần `--load` hoặc
file RAW bên ngoài. Firmware so pixel Y theo công thức độc lập, UV trung tính,
padding/guard, frame counter/tag/timestamp và ownership. Đây là kiểm tra mẫu
RAW tổng hợp, chưa phải kiểm tra ảnh thật hoặc calibration sản phẩm. Nó chưa
thay thế oracle host so toàn bộ vùng DDR và dữ liệu RAW sau chạy.

## Chép ELF và chạy VP bằng tay

Tự chép ELF mong muốn vào máy/thư mục VP. Từ thư mục chứa `fx1_soc` và ELF đã chép:

```sh
./fx1_soc --fw ./fx1_isp_init.elf --ddr-boot preinit --timeout-ms 100
./fx1_soc --fw ./fx1_isp_fullinit.elf --ddr-boot preinit --timeout-ms 100
./fx1_soc --fw ./fx1_isp_frame.elf --ddr-boot preinit --timeout-ms 100
```

Đây là các lệnh thủ công, không nằm trong build graph Neon. Không dùng
`run_vp.sh smoke` của firmware CLI cho các ELF này. VP chạy ELF; MAP và BIN
không phải đầu vào `--fw`.

Kiểm tra log có `[PASS] FW_BOOT`, marker tương ứng
`SMOKE_RESULT=PASS`, `INIT_LSC12_RESULT=PASS`, `INIT_FULL_RESULT=PASS` hoặc
`FRAME_RESULT=PASS`, cùng `[fx1_soc] RESULT PASS` và exit code 0. Với FullInit,
đọc thêm các dòng `INIT_CASE`: trường hợp default ba bảng LSC có thể là
`KNOWN_LIMIT`; không kết luận toàn bộ mặc định đã PASS chỉ từ verdict tổng.
Timeout, exception hoặc chỉ in banner đều chưa phải PASS.

Không cần bootloader với `preinit`: VP khởi tạo DDR, nạp ELF và thả CPU chạy.
Startup vẫn bắt buộc. `--ddr-boot firmware` yêu cầu chương trình từ ROM/SRAM tự
khởi tạo DDR; các ELF ở đây link vào DDR, không dùng cho mode đó.

## Thiết kế port và giới hạn

- `cmake/fx1.cmake` tạo device library từ startup/runtime nội bộ, preprocess
  linker nội bộ và đưa linker vào helper `neon_mdf_add_executable()`.
- `arch/RISCV/Device/FTEL/FX1/` giữ startup, linker template, runtime, libc và
  memory map. Import trực tiếp BSP thay vì chạy generator startup WCH. FX1
  chưa có YAML/generator riêng; chỉnh memory map tại header BSP duy nhất.
- CPU có hai hart, hart 0 chạy main, hart 1 chờ release; mỗi hart stack 64 KiB.
  Linker bắt đầu image tại `0x80000000`, stack tại `0x80400000`; `STACK 100%`
  trong báo cáo linker là đặt trước toàn bộ stack, không phải đo stack runtime.
- Linker BSP dùng vùng IMAGE RWX; GNU ld có cảnh báo RWX LOAD segment. Đây là
  layout DDR của BSP được nhập nguyên bản, không bật bảo vệ bộ nhớ.
- `neon_mdf_driver` của FX1 là INTERFACE mang device/Kconfig; không tuyên bố
  đã có Neon GPIO/UART HAL. Console trong app dùng BSP `fx1_puts()`.
- Component `components/ftel_isp` giữ driver/pipeline/algorithm và regmap v1.07;
  app link `neon_mdf_ftel_isp` tường minh. Source và hash import ở `PORTING.md`.
- Map là hợp đồng VP provisional pending HAS. Cache hooks của BSP chỉ fence
  trên VP; chưa phải triển khai cache maintenance cho silicon.
- Hiện hỗ trợ bare-metal/polling; gọi helper FreeRTOS với FX1 báo lỗi rõ ràng.
  Tick/trap/context FPU, bootloader/OTA và image BootROM chưa được port.

## Kiểm chứng bản triển khai

Ngày 04/11/2026: bốn app đã configure/build bằng CMake/Ninja, kiểm tra ELF
RISC-V single-float ABI và không còn undefined symbol; sinh đủ ELF/MAP/BIN.
Frame đã clean build trong cây Neon tách riêng ở `/tmp`, không chứa VP hoặc
standalone ISP. App CH32 tối thiểu cũng configure/build hồi quy thành công.
Regmap trong Neon đã đối chiếu workbook CSR `.xlsm` gốc: 573 thanh ghi, 819
trường, 9252 hằng số và 135 alias khớp (parser audit chuẩn hóa width dạng
`16.0` của Excel thành số nguyên). Chưa chạy lại toàn bộ host suite của SDK
standalone. Chưa kiểm chứng các target ARM vì chưa có toolchain ARM tại máy.

ELF Neon mới **chưa chạy trên VP**; phần chép file và chạy thủ công để người dùng
thực hiện. Kết quả init PASS trước đây trong kế hoạch thuộc phép thử standalone,
không phải verdict runtime của các app Neon này.

Trên máy triển khai hiện tại, CMake/Ninja/kconfiglib được cài tạm trong `/tmp`
để kiểm chứng, có thể dùng trong phiên này bằng:

```sh
export PYTHONPATH="/tmp/neon-build-tools${PYTHONPATH:+:$PYTHONPATH}"
export PATH="/tmp/neon-build-tools/bin:/opt/toolchains/riscv-none-elf/bin:$PATH"
```

Các đường dẫn `/tmp` này không được ghi vào source/preset; môi trường máy khác
cài dependency bình thường theo mục Chuẩn bị.

## Đối chiếu toàn bộ profile với run_vp.sh

Xem [FX1_ISP_ProfileReplay](../examples/FX1_ISP_ProfileReplay/README.md): app
bare-metal nhúng full D65 CSR profile và job, dùng reference loader snapshot
riêng để tái hiện INIT/SEAL/APPLY. NV12 khớp từng byte với run_vp.sh. App này
không thay thế hoặc bổ sung profile-mode vào component ftel_isp legacy.

Hướng dẫn thao tác đầy đủ: [Build ELF, chép sang VP và xuất ảnh D65](FX1_ELF_TO_VP.md).
