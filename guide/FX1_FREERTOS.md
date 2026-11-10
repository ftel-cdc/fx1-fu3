# FX1 — FreeRTOS, service ISP và kiểm chứng trên VP

Build ELF không cần repo VP; chỉ các target run cần executable `fx1_soc`.

## 1. Chuẩn bị terminal

Bắt đầu tại **gốc repository SDK**: thư mục chứa `examples/`, `neon-mdf/`
và `guide/`, không phải thư mục SDK con `neon-mdf/`. Tên thư mục clone có
thể thay đổi; các lệnh không phụ thuộc tên tài khoản hoặc vị trí trên máy.
Nếu gói giải nén có thêm một thư mục bọc, đi vào thư mục chứa ba mục trên.

Cần Bash/Linux, CMake >= 3.25, Ninja, Python 3 có `venv`/`pip`, và GCC
`riscv-none-elf-gcc` hỗ trợ RV32IMAFC/ilp32f. Dùng bản xPack RISC-V của dự án
hoặc toolchain tương thích có đúng tiền tố này; đặt thư mục `bin/` theo sơ đồ
hoặc chỉ định `FX1_TOOLCHAIN_BIN`. [SETUP](../SETUP.md) hướng dẫn công cụ nền;
phần CH32V003 trong đó dùng toolchain/ABI riêng, không phải preset FX1.
Kernel, startup, linker và BSP đã ở trong SDK, không cần lấy chúng từ VP.

Bố trí ví dụ để chạy được các lệnh bên dưới:

```text
workspace/
├── neon_mdf/                         # gốc SDK; tên clone tùy chọn
│   ├── examples/
│   ├── neon-mdf/
│   └── guide/
├── fx1-dev/                          # repo/gói VP, chỉ cần khi chạy
│   ├── vp/bin/VP_FX1_V3.0/fx1_soc
│   └── vp_neon/run_vp_neon
├── toolchains/riscv-none-elf/bin/     # tùy chọn nếu compiler đã có trên PATH
└── raw/D65_raw_2688x1520_5376.raw      # dữ liệu test do dự án cung cấp
```

**Chuẩn bị Python một lần** tại gốc SDK:

```bash
python3 -m venv .venv
. .venv/bin/activate
python -m pip install kconfiglib PyYAML Pillow
```

**Mỗi terminal mới**, vào gốc SDK và chạy:

```bash
. .venv/bin/activate
export SDK_ROOT="$PWD"
export NEON_WORKSPACE="$(dirname "$SDK_ROOT")"
export FX1_PYTHON="$(command -v python3)"
export FX1_TOOLCHAIN_BIN="${FX1_TOOLCHAIN_BIN:-$NEON_WORKSPACE/toolchains/riscv-none-elf/bin}"
export PATH="$FX1_TOOLCHAIN_BIN:$PATH"
export VP_ROOT="${VP_ROOT:-$NEON_WORKSPACE/fx1-dev}"
export RAW_FILE="${RAW_FILE:-$NEON_WORKSPACE/raw/D65_raw_2688x1520_5376.raw}"
export NEON_FX1_VP_EXECUTABLE="$VP_ROOT/vp/bin/VP_FX1_V3.0/fx1_soc"

cmake --version
ninja --version
riscv-none-elf-gcc --version
"$FX1_PYTHON" -c 'import kconfiglib, yaml; from PIL import Image'
```

Nếu toolchain, VP hoặc RAW nằm nơi khác, đặt lại `FX1_TOOLCHAIN_BIN`,
`VP_ROOT`, `RAW_FILE` trước block trên. Có thể dùng đường dẫn tương đối;
nên chuyển về đường dẫn đầy đủ từ vị trí hiện tại trước khi `cd` sang example:

```bash
# Ví dụ SDK có thêm một thư mục bọc, VP nằm cách gốc SDK hai cấp:
export VP_ROOT="$(cd ../../fx1-dev && pwd)"
export RAW_FILE="$VP_ROOT/../raw/D65_raw_2688x1520_5376.raw"
export NEON_FX1_VP_EXECUTABLE="$VP_ROOT/vp/bin/VP_FX1_V3.0/fx1_soc"
```

Để **chỉ build ELF**, không cần có VP/RAW; chỉ cần compiler và Python sẵn sàng.
Trước khi chạy mô phỏng, kiểm tra VP (RAW chỉ cần cho example ảnh):

```bash
test -x "$NEON_FX1_VP_EXECUTABLE"
test -f "$VP_ROOT/vp_neon/run_vp_neon"
test -f "$RAW_FILE"
```

Các biến dùng chung cho cả Bare-metal và FreeRTOS. Build ELF và run VP là
hai bước riêng; giữ giao diện runner `run_vp_neon ELF RAW`.

## 2. Build ELF và chạy init trong task

```bash
cd "$SDK_ROOT/examples/ISP_Bayer_AE"
cmake --preset FX1 -DNEON_MDF_PYTHON3="$FX1_PYTHON" \
  -DNEON_FX1_VP_EXECUTABLE="$NEON_FX1_VP_EXECUTABLE"
cmake --build --preset FX1 --target isp_bayer_ae
cmake --build --preset FX1 --target run-vp
cat build/FX1/platform.log
```

ELF: `build/FX1/isp_bayer_ae.elf`. Mong đợi
`BAYER_AEC_INIT PASS checks=16` và `[fx1_soc] RESULT PASS`.
Lệnh target `isp_bayer_ae` chỉ build. Các target `run-*` tự build cập nhật,
nạp ELF vào VP và kiểm verdict; không dùng job/profile của runner D65.

## 3. Chạy frame/AEC và service ISP

Tiếp tục trong `examples/ISP_Bayer_AE/` sau khi configure mục 2:

```bash
cmake --build --preset FX1 --target run-frame-vp
cat build/FX1/frame_aec.log
cmake --build --preset FX1 --target run-service-vp
cat build/FX1/service.log
```

| Bài | ELF trong `build/FX1/` | Marker cần có cùng VP RESULT PASS |
|---|---|---|
| Init task | `isp_bayer_ae.elf` | `BAYER_AEC_INIT PASS checks=16` |
| RAW/AEC | `isp_bayer_aec_frame.elf` | `FRAME_AEC PASS frames=7 checks=114` |
| Service | `isp_freertos_service.elf` | `ISP_FREERTOS_SERVICE PASS checks=37` |

Đối chiếu init bare-metal riêng: target `run-init-vp`, log
`build/FX1/baremetal_init.log`, 5 kiểm tra PASS. Bài này không dùng FreeRTOS.

## 4. Smoke FreeRTOS nền tảng

```bash
cd "$SDK_ROOT/examples/FX1_FreeRTOS_Smoke"
cmake --preset FX1_VP -DNEON_MDF_PYTHON3="$FX1_PYTHON" \
  -DNEON_FX1_VP_EXECUTABLE="$NEON_FX1_VP_EXECUTABLE"
cmake --build --preset FX1_VP --target run-vp
cat build/FX1_VP/vp/platform.log
```

ELF: `build/FX1_VP/fx1_freertos_smoke.elf`. Mong đợi
`SUMMARY checks=25 failed=0` và VP `RESULT PASS`; báo cáo tại
`build/FX1_VP/vp/result.json`. Kiểm tick 1 kHz từ CLINT 20 MHz, PLIC,
chuyển task, giữ f0..f31/fcsr. Hart 0 chạy FreeRTOS, hart 1 chờ.
Giữ `NEON_FX1_SAVE_FPU_CONTEXT=ON`, `configENABLE_FPU=0`: extension local
lưu FPU, không bật thêm cơ chế lưu native của kernel.

## 5. File chính

Đường dẫn tính từ gốc SDK:

| Đường dẫn | Vai trò |
|---|---|
| `neon-mdf/rtos/FreeRTOS-Kernel/` | Kernel hiện có, dùng chung các target |
| `neon-mdf/rtos/port/fx1/` | Nối trap/PLIC và lưu context FPU FX1 |
| `neon-mdf/rtos/services/ftel_isp/` | Worker ISP, queue/ticket, IRQ, timeout và recovery |
| `neon-mdf/components/ftel_isp/include/ftel_isp.h` | API Bayer/AEC/frame độc lập OS |
| `neon-mdf/components/ftel_isp/pipeline/` | Init, submit/collect/recovery; giữ API init/stream toàn pipeline |
| `neon-mdf/mcu-description/devices/FX1.yaml` | Mô tả CPU, memory, IRQ và thanh ghi |
| `neon-mdf/arch/RISCV/Device/FTEL/FX1/` | BSP, startup/linker local, header sinh từ YAML và IRQ adapter |
| `examples/ISP_Bayer_AE/Inc/FreeRTOSConfig.h` | Cấu hình FreeRTOS cho các bài ISP |
| `examples/ISP_Bayer_AE/Src/main.c` | Init/readback trong task |
| `examples/ISP_Bayer_AE/Src/frame.c` | RAW tổng hợp → ISP → AEC |
| `examples/ISP_Bayer_AE/Src/service.c` | Kiểm client/worker, buffer, timeout/recovery |
| `examples/FX1_ISP_Image_FreeRTOS/` | RAW thật + profile Image qua cùng service FreeRTOS |
| `examples/FX1_ISP_Image_FreeRTOS/Src/main.c` | Luồng RAW/profile/DMA → job → kết quả frame → NV12 |
| `examples/FX1_ISP_Image_FreeRTOS/profile/profile.csrw` | Profile local của app FreeRTOS |
| `examples/FX1_ISP_Image_FreeRTOS/tools/generate_profile.py` | Generator local, cùng cách dùng như bare-metal |
| `examples/FX1_ISP_Image_FreeRTOS/Inc/FreeRTOSConfig.h` | Cấu hình kernel riêng của app ảnh FreeRTOS |
| `examples/FX1_ISP_Image_FreeRTOS/Tests/pipeline_checks.c` | Các ca queue/ticket/buffer/timeout/recovery, chỉ liên kết vào ELF test |

## 6. Phạm vi và ownership

Service có hai loại job dùng **cùng worker/queue/recovery**:

- `submit` / `wait`: Bayer/AEC, một frame mỗi init, có thống kê AEC như trước.
- `submit_pipeline` / `wait_pipeline`: config toàn pipeline từ Image, 1..4 frame
  mỗi init. Giữ trạng thái giữa các frame trong job, dừng stream khi job xong.
  Mỗi job tiếp theo init lại; đây chưa phải stream liên tục hoặc vòng kín 3A.

Client không gọi CSR, init/stream hoặc API driver thấp song song với worker.
ISR chỉ ACK sự kiện và notify; worker poll trạng thái DONE, xác nhận metadata,
trả slot và làm recovery. Khi chờ frame, worker block bằng notification;
init/hình học vẫn dùng các hàm chờ có giới hạn của driver.

Tối đa ba ticket, một frame phần cứng đang chạy. `wait` hết hạn không hủy job.
Chỉ task submit được wait/release; dùng sai hàm wait với loại ticket trả
`E_STATE`. Ticket DONE vẫn giữ buffer tới `release`. Recovery chưa xác nhận
thì buffer bị giữ, service chặn job mới; gọi `recover` rồi wait/release lại.

Job pipeline copy config và metadata; không copy pixel hoặc bảng LSC mà
config trỏ tới. Caller giữ bảng LSC bất biến và giữ toàn bộ bốn slot RAW/output
đã cấu hình tới release, kể cả slot chưa chạy. Service từ chối địa chỉ thuộc
vùng firmware, vùng chồng nhau và vùng đang được ticket khác giữ; áp dụng
chung cho hai loại job. Service quản lý cache hooks lúc giao/nhận DMA;
BSP VP hiện chỉ barrier, chưa thay cho clean/invalidate trên silicon có cache.

Service nhận tối đa hai bảng LSC, dành một resident để tránh nạp vào bảng
active còn giữ sau soft reset. Có thể đổi **resident vật lý**, giữ nguyên hệ số
và strength; xem `result.init.lsc.profile` để biết nơi đã nạp. Config của caller
không bị đổi. Ba bảng trả `E_UNSUPPORTED` trước queue, không bỏ qua bảng lỗi.
Bộ đếm DMA có thể giữ giá trị qua reset. Khi liên kết service FreeRTOS,
CMake bật `ISP_STREAM_ALLOW_RETAINED_COUNTS` cho driver stream: kiểm recovery
sequence/quyền slot và delta +1, không yêu cầu count tuyệt đối bằng 0.
Build Bare-metal không bật cờ này, giữ quy tắc mở stream hiện có.

Các bài Bayer/AEC cũ vẫn dùng RAW tổng hợp và bypass LSC. App pipeline dùng
RAW thật, LSC và full profile Image. Chưa nối AE/AWB/AF tự động cập nhật
exposure/gain. Fault injection chỉ được link vào ELF kiểm thử, không nằm
trong thư viện production hoặc ELF ảnh thông thường.

## 7. YAML và đổi vị trí SDK

Configure target FX1 đọc YAML và sinh `FX1.h`, `mcu_device.h`,
`fx1_registers.h`, `FX1.svd`. Startup/linker vẫn dùng file local, không sinh
lại bằng backend WCH. Kiểm metadata riêng từ gốc SDK:

```bash
"$FX1_PYTHON" neon-mdf/mcu-description/tools/check_fx1.py --cc riscv-none-elf-gcc
```

Nguồn YAML: `neon-mdf/mcu-description/devices/FX1.yaml`; checker so header/SVD
với regmap/BSP local, không tự xác nhận register map silicon.
Khi chuyển thư mục, configure lại với `--fresh` trong đúng example rồi
truyền lại Python/VP. Không dùng CMake cache từ máy cũ.

## 8. RAW thật qua FreeRTOS — giữ runner ELF/RAW hiện có

Sau bước chuẩn bị ở mục 1, Python chạy runner cần thêm `Pillow`. RAW mẫu:
`D65_raw_2688x1520_5376.raw`, 2688×1520 BGGR RAW10 căn trái trong uint16 LE.
RAW có thể nằm ngoài SDK và được chọn bằng `RAW_FILE`.

**Build ELF** từ gốc SDK:

```bash
cd "$SDK_ROOT/examples/FX1_ISP_Image_FreeRTOS"
python3 tools/generate_profile.py
cmake --preset FX1 -DNEON_MDF_PYTHON3="$FX1_PYTHON"
cmake --build --preset FX1
```

ELF: `examples/FX1_ISP_Image_FreeRTOS/build/FX1/fx1_isp_image_freertos.elf`.
Không cần repo VP để configure/build. App dùng `Inc/FreeRTOSConfig.h` của
chính nó; startup, linker, kernel, port và driver đều ở Neon.

Ba lệnh sau `cd` giống bare-metal: sinh header xem trước → configure → build.
Có thể dùng `make build` để thực hiện cùng ba bước. Đây là build ELF;
lệnh ở phần Run VP mới nạp và chạy firmware.

**Run VP** bằng runner hai tham số hiện có:

```bash
"$VP_ROOT/vp_neon/run_vp_neon" \
  "$SDK_ROOT/examples/FX1_ISP_Image_FreeRTOS/build/FX1/fx1_isp_image_freertos.elf" \
  "$RAW_FILE"
```

Kết quả tại `$VP_ROOT/vp_neon/result/`: `output.png`, `output.nv12`, `vp.log`,
`report.json`. Phải có `PIPELINE_FREERTOS PASS`, `IMAGE_RESULT=PASS` và
`[fx1_soc] RESULT PASS`. Runner ghi đè kết quả mỗi lần chạy như trước.
Luồng Image bare-metal và các lệnh trong [FX1 Neon ISP Run](FX1_NEON_ISP_RUN.md)
giữ nguyên; đổi ELF là chọn app FreeRTOS.

Profile mặc định của app này nằm tại
`examples/FX1_ISP_Image_FreeRTOS/profile/profile.csrw`. Bare-metal giữ profile
riêng tại `examples/FX1_ISP_Image/profile/profile.csrw`; hai bản hiện có cùng
hệ số, sửa bản nào chỉ ảnh hưởng example dùng bản đó.

- `python3 tools/generate_profile.py` tạo `profile/profile.h` để xem trước.
- CMake tự sinh và biên dịch với `build/FX1/generated/profile.h`. Vì vậy có
  thể bỏ bước Python riêng nếu chỉ cần build ELF; không sửa các header sinh ra.
- Muốn đối chiếu ảnh phải dùng cùng RAW, hệ số profile và số frame.

Chọn profile riêng/số frame bằng các tùy chọn đã có:

```bash
cd "$SDK_ROOT/examples/FX1_ISP_Image_FreeRTOS"
cmake --preset FX1 -B build/custom \
  -DFX1_ISP_PROFILE="$SDK_ROOT/examples/FX1_ISP_Image_FreeRTOS/profile/profile.csrw" \
  -DFX1_IMAGE_FRAME_COUNT=2 -DNEON_MDF_PYTHON3="$FX1_PYTHON"
cmake --build build/custom
```

Nếu dùng cache từ máy cũ hoặc trước khi tách profile local, configure lại
một lần trong example để lấy đúng đường dẫn và Python hiện tại:

```bash
cmake --preset FX1 --fresh -DNEON_MDF_PYTHON3="$FX1_PYTHON"
cmake --build --preset FX1
```

Sau đó tiếp tục dùng các lệnh build thông thường ở trên.

Đưa ELF ở `build/custom/` vào cùng lệnh runner. Mặc định hai frame để GTM auto
có trạng thái từ frame đầu; một frame có thể cho output khác hai frame.

### Kiểm tra ownership và recovery toàn pipeline

Trong example FreeRTOS đã configure:

```bash
cmake --build --preset FX1 --target fx1_isp_pipeline_checks
"$VP_ROOT/vp_neon/run_vp_neon" \
  "$SDK_ROOT/examples/FX1_ISP_Image_FreeRTOS/build/FX1/fx1_isp_pipeline_checks.elf" \
  "$RAW_FILE"
```

`Tests/pipeline_checks.c/.h` chỉ được thêm vào target
`fx1_isp_pipeline_checks`; ELF ảnh mặc định `fx1_isp_image_freertos.elf`
không liên kết fault injection. ELF test kiểm buffer chồng nhau giữa
Bayer/pipeline, wait sai loại, ticket stale,
client timeout, frame timeout, recovery hết hạn/giữ buffer, recovery tường minh,
copy config/metadata và chạy tiếp sau lỗi. Mặc định hai frame có
`PIPELINE_FREERTOS PASS checks=38`. ELF ảnh thường có 13 kiểm tra; bài service
Bayer/AEC cũ vẫn có 37 kiểm tra. Không dùng ELF fault test làm firmware sản phẩm.

### Đối chiếu Bare-metal và FreeRTOS

Build cả hai app với cùng profile/số frame, rồi chạy cùng `RAW_FILE`. Runner
chỉ giữ kết quả lần chạy cuối, nên lưu NV12 của Bare-metal trước:

```bash
cd "$SDK_ROOT/examples/FX1_ISP_Image"
python3 tools/generate_profile.py
cmake --preset FX1 -DNEON_MDF_PYTHON3="$FX1_PYTHON"
cmake --build --preset FX1

mkdir -p "$SDK_ROOT/examples/FX1_ISP_Image_FreeRTOS/build/compare"
"$VP_ROOT/vp_neon/run_vp_neon" \
  "$SDK_ROOT/examples/FX1_ISP_Image/build/FX1/fx1_isp_image.elf" \
  "$RAW_FILE"
cp "$VP_ROOT/vp_neon/result/output.nv12" \
  "$SDK_ROOT/examples/FX1_ISP_Image_FreeRTOS/build/compare/baremetal.nv12"

"$VP_ROOT/vp_neon/run_vp_neon" \
  "$SDK_ROOT/examples/FX1_ISP_Image_FreeRTOS/build/FX1/fx1_isp_image_freertos.elf" \
  "$RAW_FILE"
cmp "$SDK_ROOT/examples/FX1_ISP_Image_FreeRTOS/build/compare/baremetal.nv12" \
  "$VP_ROOT/vp_neon/result/output.nv12"
```

`cmp` trả mã 0 và không in khác biệt nghĩa hai NV12 khớp từng byte. Đối chiếu
bản tích hợp với D65 mặc định hai frame đã PASS; đây là kiểm chứng tính
tương đương, chưa thay cho đánh giá màu với golden độc lập. Không so hash
PNG vì đó là ảnh xem trước chuyển từ NV12.

## 9. Pull source hoặc chuyển máy

- Chỉ cần source, profile, kernel/BSP local và công cụ đã nêu. Không cần mang
  theo ELF, log hoặc `build/` từ máy tác giả.
- Nếu đã có cache từ vị trí cũ, chạy `cmake --preset FX1 --fresh` trong đúng
  example rồi build lại. Với smoke dùng preset `FX1_VP` và truyền lại
  `-DNEON_MDF_PYTHON3="$FX1_PYTHON"` cùng
  `-DNEON_FX1_VP_EXECUTABLE="$NEON_FX1_VP_EXECUTABLE"`.
- Không đưa `.venv/`, `build/`, ELF và ảnh/log sinh ra vào source control.
- Không thấy compiler: kiểm tra `FX1_TOOLCHAIN_BIN` và PATH. Thiếu Python
  package: kích hoạt `.venv` và chạy lại lệnh pip ở mục 1.
- Thiếu VP hoặc RAW: kiểm tra `VP_ROOT`, `RAW_FILE`; lỗi này thuộc bước run,
  không phải yêu cầu của bước build ELF.
