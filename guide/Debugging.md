# Debug qua VS Code — cách hoạt động + gotcha đã gặp thật

`vscode-extension`'s lệnh **NEON-MDF: Debug** thay thế hoàn toàn
`.vscode/launch.json` truyền thống — không cấu hình debug rồi cho từng
project, extension tự đọc executable/target qua CMake File API. File này
giải thích cơ chế bên dưới + 2 bug thật đã gặp và fix trên phần cứng, để
không tái phát khi sửa `debug.ts`/thêm kiến trúc mới.

## Hiện trạng hỗ trợ

**Chỉ Cortex-M** (`FU2`/`WEACT_F412`, qua OpenOCD/SWD) hoạt động thật —
`CH32V003F4P6` (RISC-V) **chưa wire** (cần OpenOCD fork riêng của WCH,
chưa xác nhận hoạt động — xem `README.md` mục giới hạn). Chọn target RISC-V
rồi bấm Debug sẽ throw lỗi rõ ràng (`debugArch.ts`'s
`resolveDebugArchProfile()`), không âm thầm dùng nhầm cấu hình ARM.

## Quy trình khi bấm "NEON-MDF: Debug"

1. Đảm bảo đã configure (`ensureConfigured`), đọc danh sách executable thật
   trong build dir qua CMake File API (`cmakeFileApi.ts`) — nhiều hơn 1
   executable thì hỏi chọn qua QuickPick.
2. **Flash TRƯỚC bằng đúng target CMake** (`flash_jtag` cho bản factory,
   `flash_ota` cho bản OTA) — KHÔNG để cortex-debug tự flash bằng lệnh
   `load` gốc của GDB (xem mục "Gotcha #1" dưới). Chọn đúng target CMake qua
   manifest thật `<builddir>/neon_mdf_debug_targets.txt` (ghi bởi
   `cmake/flash_targets.cmake`, dòng `factory=<tên>`/`ota=<tên>`/
   `bootloader=<tên>`) — KHÔNG đoán qua tên executable có kết thúc `_ota`
   hay không (cách cũ, có bug thật: `examples/OTA_Demo`'s
   `ota_demo_ota_bad` dùng chung slot OTA nhưng tên không kết thúc `_ota`,
   đã sửa 2026-08-11). Executable không khớp tên nào trong manifest bị từ
   chối debug rõ ràng thay vì đoán liều. Chỉ fallback về đoán theo tên khi
   target không có bootloader (không có manifest, vd `FU2`).
3. Suy ra file `.cfg` openocd (`cmake/openocd/<target>_openocd.cfg`) và
   `.svd` (`mcu-description/generated/<target>.svd`) theo đúng quy ước tên
   — không cấu hình tay.
4. Khởi động phiên debug (`vscode.debug.startDebugging`) với
   `servertype: "openocd"`, `runToEntryPoint: "main"`,
   `preLaunchCommands`/`postLaunchCommands` lấy từ `debugArch.ts` (mục
   "Gotcha #2").

## Gotcha #1 — GDB `load` xoá mất `image_header_t`

**Triệu chứng đã gặp thật**: debug chạy nhưng KHÔNG dừng ở `main()`, step
báo `"Cannot find bounds of current function"` — trông như đang debug nhầm
code khác.

**Nguyên nhân thật** (xác nhận qua đọc trực tiếp flash bằng SWD trước/sau):
GDB `load` chỉ ghi đúng nội dung ELF (vector table + code, bắt đầu từ
`<partition>_VECTOR_BASE`) — nhưng `image_header_t` (16 byte magic/size/
crc32, nằm NGAY TRƯỚC vector table, CÙNG 1 sector flash với code) không
phải 1 phần của ELF (host tooling tự ghép thêm lúc build blob để flash,
xem `guide/BootloaderOTA.md`). Ghi flash bắt buộc erase cả sector trước —
`load` xoá mất header mà không ghi lại (không biết nó tồn tại) → header
thành `0xFFFFFFFF` → bootloader thấy header không hợp lệ → không dám nhảy
vào app → CPU kẹt ở bootloader, breakpoint đặt theo symbol của app.elf
không bao giờ khớp.

**Fix**: flash trước bằng target CMake thật (bước 2 ở trên, giữ nguyên
header) + `loadFiles: []` trong cấu hình debug (`debug.ts`) để cortex-debug
KHÔNG tự `load` lại lần nữa. **Chú ý**: phải là `[]` (mảng rỗng), KHÔNG
phải `false` — đã đọc trực tiếp source cortex-debug: `false` vẫn chạy
`target-download` bình thường, chỉ mảng/list rỗng mới thực sự bỏ qua bước
download.

## Gotcha #2 — `monitor reset halt` không đáng tin cậy

**Triệu chứng đã gặp thật**: đôi khi halt SAU khi đã chạy vài lệnh đầu của
`Reset_Handler` (SP không còn nguyên vẹn đỉnh RAM), không nhất quán giữa
các lần thử — dù cùng 1 lệnh `monitor reset halt`.

**Fix** (`debugArch.ts`'s `CORTEX_M_PROFILE`): bật
`DEMCR.VC_CORERESET` (bit0 của `0xE000EDFC`, đặc tả ARMv7-M) TRƯỚC khi
debug adapter tự reset:

```
preLaunchCommands:  ["monitor halt", "monitor mww 0xE000EDFC 0x00000001"]
postLaunchCommands: ["monitor mww 0xE000EDFC 0x00000000"]   # tat lai, khong anh huong lan reset thu cong sau
```

Đây là cơ chế **phần cứng thật** của chính CPU (Cortex Debug control Block)
— CPU tự động vào Debug state ngay lúc lấy reset vector, TRƯỚC khi chạy bất
kỳ lệnh nào của `Reset_Handler`, bất kể reset được kích bởi cơ chế nào (SRST
vật lý hay AIRCR phần mềm) — không phụ thuộc polling/timing của OpenOCD như
`monitor reset halt`. Đã verify 4/4 lần lặp lại cho kết quả giống hệt nhau
(SP nguyên vẹn đỉnh RAM).

## `debugArch.ts` — kiến trúc profile theo CPU

```ts
interface DebugArchProfile {
  debugType: string;              // "cortex-debug"
  preLaunchCommands: string[];
  postLaunchCommands: string[];
}
```

Tách riêng phần **đặc thù kiến trúc CPU** (`debugType`, lệnh workaround
reset — thanh ghi `0xE000EDFC` là System control Space của ARMv7-M, KHÔNG
tồn tại trên RISC-V) khỏi phần **chung mọi kiến trúc** trong `debug.ts` (đọc
executable qua CMake File API, quy ước đường dẫn `.cfg`/`.svd`, flash-trước-
rồi-attach để tránh Gotcha #1 — vấn đề của chính thiết kế header/flashmap
SDK này, không đặc thù ARM).

Thêm profile RISC-V khi có board CH32V003F4P6 thật + đã chốt công cụ debug
(OpenOCD fork của WCH hay công cụ khác) — viết `RISCV_PROFILE` mới trong
`debugArch.ts`, đăng ký thêm nhánh trong `resolveDebugArchProfile()`. RISC-V
Debug Spec dùng Debug Module riêng (`dmcontrol`/`dmstatus`...) — địa chỉ/ngữ
nghĩa hoàn toàn khác `DEMCR`, không copy nguyên workaround Cortex-M sang.

## Troubleshooting

| Triệu chứng | Kiểm tra |
|---|---|
| "Cannot find bounds of current function" ngay khi debug | Rất có thể lại là Gotcha #1 — xác nhận `loadFiles: []` (không phải `false`) còn trong `debug.ts`. |
| Debug dừng ở địa chỉ lạ, SP không phải đỉnh RAM | Gotcha #2 — xác nhận `preLaunchCommands` có ghi `0xE000EDFC`. |
| "khong tim thay file openocd cfg" | Target đang chọn chưa có `cmake/openocd/<target>_openocd.cfg` — RISC-V hiện chưa có file này (xem mục "Hiện trạng hỗ trợ"). |
| Không khởi động được phiên debug | Extension `marus25.cortex-debug` chưa cài. |
