# Camera Runtime FX1

Cập nhật 27/11/2026: session core, register backend **scalar profile tối thiểu** và
camera FreeRTOS worker đã có implementation. IRQ integration vẫn **IN_PROGRESS**:
chưa có bằng chứng xử lý ảnh end-to-end trên MAS v1.07 hardware/VP tương thích.
Không coi các capability trong hợp đồng backend là kết quả kiểm chứng silicon.

## Phạm vi implementation

- Core ở `src/`, public headers ở `include/`; không include FreeRTOS hoặc MMIO.
  Lifecycle, NEXT/EXACT, một transaction pending, bốn slot và một frame in-flight.
- Lease generation/64-bit serial, acquire FIFO, history tám completion. READY và
  CONSUMER giữ output riêng; không skip hardware rotation head đang consumer-held.
- `camera_events.c` xử lý APPLIED/OUTPUT/DRAINED trước khi trả FAULT. Một lỗi
  EXACT không được che successful retirement trong cùng batch. Snapshot diagnostics
  vẫn cập nhật khi backend trả NO_DATA/PENDING; không suy completion từ notification.
- Backend `driver/isp/platform/fx1/fx1_isp_backend.*` dùng MMIO/cache/lifecycle hooks.
  RAW12 Linear DDR replay, 32..3840 × 32..2160, kích thước chẵn, alignment 16 byte,
  CH0 NV12 full resolution, địa chỉ 32 bit và bốn rotation slots. CH1/2 tắt.
- Payload `fx1_isp_scalar_profile_t` ABI 1: WB R/Gr/Gb/B và digital gain Q8,
  BT.601 limited-range CSC, demosaic theo scalar defaults của INIT. Gr/Gb dùng
  cùng giá trị `wb_g`. Các block cần SRAM/table và AEC/AWB/AF tắt. Không nhận
  legacy `profile.csrw`, binary IQ hoặc danh sách ghi thanh ghi tùy ý.
- Baseline thật: stopped DMA recovery để xác lập rotation 0 → stable records →
  exit profile mode → geometry từng bước/PREP_BUSY → output/DMA setup → profile
  mode → INIT/load scalar/SEAL → CHECK_BUSY clear + sealed bit → APPLY. Không
  cần table confirm khi tất cả table consumers tắt. Submit chỉ nhận sau APPLY
  accepted, cho ingress chạy để baseline activate; APPLIED đọc từ active bank/ID/frame.
- Dynamic NEXT/EXACT dùng bank không active/pending/referenced, UNSEAL/INIT/load/
  SEAL/CHECK_BUSY/APPLY; check NEXT_FRAME cả lúc queue và trước command. Hardware
  acceptance/error vẫn có thẩm quyền. Tune rejection giữ diagnostics và fault,
  không tự phát reset hoặc giả APPLIED. Payload bất biến tới APPLIED hoặc drain.

## IRQ, statistics và recovery

ISR chỉ mask ba IRQ enables tại IP, fence và notify worker. COMMON ERROR, DMA
CFG/IDMA_DONE/ODMA_DONE/AXI, STREAM_ERROR khi có INPUT_STREAM, và CH0 DONE/ERROR được enable; không bật
IDMA_UNDERRUN, IDMA_START, flow-control hay STATS_READY cho scalar profile.
`TUNE_CAP_FULL_V107` lấy canonical RO mask (`0x00FFFFFF`); `begin` vẫn đọc version,
TUNE_ID và capability thực, chỉ yêu cầu các feature đang dùng, không yêu cầu INPUT_STREAM.

Worker đọc cả COMMON/DMA/OUTPUT và status từng engine kể cả khi IRQ masked.
W1C là captured ∩ serviced ∩ canonical mask. ACK không clear ODMA DONE hoặc set
FREE. Backend đọc ingress/profile/tag/timestamps dưới DONE, kiểm tra tag, identity
trùng và abort records; invalidate output trước publish đúng một lần. Consumer
return clear DONE; FREE chỉ set ở submit đúng head sau cache prepare.

Statistics có policy **discard**, không xuất payload cho 3A: acquire tối đa hai
READY banks/engine/lượt, lưu frame/profile identity và discard counter rồi release
lease. Đọc status/drop của AEC/AWB/AF; không ghép latest của các engine. Profile
không bật statistics và không quảng bá chức năng coherent payload/3A. Bật engine
hoặc thuật toán cần implementation payload copy/geometry/LO-HI riêng.

Recovery không gửi reset khi BUSY. Stable snapshot yêu cầu BUSY=0, sequence và
status không đổi sau copy tối đa bốn records. `DIAG_RECOVERY` giữ bản copy ổn định;
`DIAG_RECOVERY_LIVE` giữ BUSY/sequence hiện tại. AUX giữ LSC/EE; DIAG_STREAM giữ stream/drop chỉ khi hardware có INPUT_STREAM.
Terminal AXI tự recovery; flow-control và stats/stream drops chỉ là diagnostics;
abort records được xử lý như failed contexts, không publish successful OUTPUT.

STOP chặn submit; backend tiếp tục retirement/stats/pending recovery, chờ idle rồi
CANCEL và một DMA-scope recovery để drain/reset rotation. Chỉ DRAINED sau đúng
sequence hoàn tất, hết hardware references/builders/stats và không có DONE chưa
reconcile. Consumer-held DONE còn được giữ. Timeout/storm giữ claim/quarantine;
không coi timeout là drain và không reset lặp. Sau drain và mọi lease return,
backend mask IP, synchronously unregister PLIC handler, rồi release shared claim.

## FreeRTOS API và lifetime

Bật `CONFIG_NEON_MDF_CAMERA_RUNTIME=y`, `CONFIG_NEON_MDF_CAMERA_FX1_BACKEND=y`.
Sau `neon_mdf_add_freertos(...)`, gọi `neon_mdf_add_camera_freertos()` và link
`neon_mdf::camera_freertos`. Không tự khởi động service `ftel_isp` cũ.

`neon_camera_freertos_create` dùng caller-owned zero-initialized storage ổn định.
Allocation chỉ ở startup (queue/task); worker tồn tại suốt application, hỗ trợ
stop/restart, chưa có API destroy. Không free/move service sau stop. Tất cả API
service chạy trong task trên hart 0. Core/ backend đều do worker duy nhất truy cập.

`enqueue` copy command, trả ticket; `collect` trả reply/completion rồi giải phóng
request storage. Có tám request/result slots hữu hạn; caller phải collect tất cả
ticket đã nhận. BUSY không nhận command/lease return, caller giữ quyền và thử lại.
ACQUIRE reply sở hữu lease tới RETURN thành công; payload APPLY vẫn là borrowed
immutable storage. START/STATUS không yêu cầu generation, các command khác kiểm
tra generation; ticket serial chặn stale collect. Reply có saved hardware probe.

STOP có mailbox riêng, không cần request slot, ưu tiên trước hai commands mỗi
lượt; mỗi lượt poll tối đa bốn lần. Poll timer tiếp tục khi không có IRQ và khi
quarantine. Deadline tuyệt đối từ pending work đầu tiên, không gia hạn bởi wake-up.
Re-arm/recheck giữ critical section hart 0; event sau recheck được notification
lưu tới atomic wait. Storm budget hết chuyển timeout/quarantine và giữ IRQ masked.

Core vẫn link độc lập qua `neon_mdf::camera_runtime`. Backend có thể dùng bare-metal
owner với port hooks riêng; `release` hook phải detach ISR trước khi trả claim.
Driver trực tiếp ghi MMIO ngoài giao thức vẫn phải được ứng dụng serialize.

## Kiểm chứng 27/11/2026

```sh
make -C neon-mdf/tests/camera_hal test
```

597 ISP + 82 contract + 302 session + 177 IRQ/backend + 636 service checks =
**1794 checks, 0 failures**. Session/backend/service chạy thêm ASan+UBSan pass;
LeakSanitizer không chạy được dưới sandbox ptrace, đã tắt leak detection cho lượt
ASan/UBSan. Fake kernel/MMIO chứng minh policy/sequencing phần mềm, không pixel RTL.

Cross-build RV32 pass `FX1_ISP_Image` (default và 64×48), IRQ port probe, và
example cũ `FX1_ISP_Image_FreeRTOS` (còn warning initializer `crop` từ trước).
Test artifacts/logs ở `/tmp/fx1-camera-irq-*`, không thêm build artifacts vào repo.

VP revision `36ad26a4b200232ff49570c2b31f1120ccec5bd3`:

- `fx1_irq_port_probe.c`: PASS với IRQ CFG thật; ISR mask giữ sticky COMMON=2,
  DMA=9, deferred ACK sau PLIC complete, duplicate registration bị từ chối,
  unregister đúng handler/user và re-register an toàn với stale pending.
- Camera example: **UNSUPPORTED**, ISP_VERSION=`0x00010007`,
  TUNE_ID=`0x54554E45`, TUNE_CAPS=`0x003FFFFF` thiếu INPUT_PAD. Không sửa VP hoặc
  giảm capability check để chạy legacy. Chưa có frame image end-to-end evidence.

Giới hạn còn lại: full IQ/table loaders và confirmations, coherent stats payload
cho 3A, CSI/sensor timing, common-scope recovery/reload tự động, fault injection và
pixel comparison trên VP v1.07 đầy đủ/board. BSP cache hooks hiện chỉ fence cho
VP; silicon có D-cache cần cache maintenance/coherency được xác nhận. Xem
`MAS_MAPPING.md` và mục 15.9 của kế hoạch để tách phần đã làm với nghiệm thu còn thiếu.

Đường compatibility `neon_camera_legacy.h` chỉ build khi CAMERA_LEGACY=y; không
là fallback của profile backend. Chỉ example `FX1_ISP_Image` được chuyển đổi.
