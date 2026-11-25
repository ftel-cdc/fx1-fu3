# Neon Media Runtime – Plan

> SDK FX1 (RISC-V CVA6) · FreeRTOS · build CMake + Kconfig (kiểu ESP-IDF)
> Phạm vi giai đoạn đầu: camera (sensor → MIPI CSI-2 → ISP)

---

## 1. Mục tiêu

- Cung cấp một runtime media cho FX1, vai trò tương tự STM32 Camera Middleware (CMW) và ESP `esp_video`, nhưng có lõi chung để mở rộng sau này sang video, display và audio.
- Ứng dụng chỉ làm việc với API của runtime, không gọi trực tiếp driver CSI-2 (Camera Serial Interface 2) hay ISP (Image Signal Processor).
- Bản đầu chỉ cần chạy được pipeline camera: TPG (Test Pattern Generator) hoặc sensor → CSI-2 → DDR / ISP.

## 2. Nguyên tắc thiết kế (bắt buộc)

1. **Native FreeRTOS, không có OSAL** (Operating System Abstraction Layer). Runtime và driver gọi thẳng API FreeRTOS (task, queue, task notification, event group, các hàm `...FromISR`).
2. **Phần đặc thù SoC (System-on-Chip) nằm trong driver.** Runtime không truy cập thanh ghi và không có `#ifdef` theo chip. Mọi thông tin phần cứng được lấy qua API capability của driver.
3. **Media Runtime quản lý logic, thuật toán và luồng dữ liệu**: 3A, IQ (Image Quality), pipeline binding, buffer pool và ownership, chính sách lỗi. Driver chỉ thực thi và báo trạng thái.

Quy ước kèm theo:
- Ưu tiên cấp phát tĩnh (`xTaskCreateStatic`, `xQueueCreateStatic`).
- Priority và stack của từng task cấu hình qua Kconfig (`CONFIG_NEON_MDF_MEDIA_<TASK>_TASK_PRIORITY`, `CONFIG_NEON_MDF_MEDIA_<TASK>_TASK_STACK_SIZE`).
- ISR (Interrupt Service Routine) chỉ đọc/xoá cờ và đẩy event, không xử lý logic.
- Đặt tên theo mục 4.1.

## 3. Phân lớp

```
┌──────────────────────────────────────────────────────┐
│ Application                                          │
├──────────────────────────────────────────────────────┤
│ neon_media_runtime (nhóm component)                  │
│  camera · isp · (video · display · audio sau này)    │
│  └── core: buffer, pipeline, event, ...              │
├──────────────────────────────────────────────────────┤
│ Drivers (SoC-specific): csi, phy, isp, i2c, gpio     │
│  └── platform/<family>/ – truy cập thanh ghi         │
├──────────────────────────────────────────────────────┤
│ FreeRTOS                                             │
└──────────────────────────────────────────────────────┘
```

## 4. Cấu trúc thư mục

```
components/
├── driver/                           # một target neon_mdf_driver
│   ├── csi/                          # neon_csi.h + platform/fx1/neon_csi.c
│   ├── phy/                          # neon_phy.h + platform/fx1/ (D-PHY)
│   ├── isp/                          # neon_isp.h + platform/fx1/ (thanh ghi lấy từ ftel_isp) – xem mục 12
│   ├── i2c/                          # neon_i2c.h
│   └── gpio/                         # neon_gpio.h
├── ftel_isp/                         # ISP v1.07 + 3A của vendor – TODO: tách vào driver/isp và neon_media_runtime/isp (mục 12)
└── neon_media_runtime/               # thư mục nhóm
    ├── CMakeLists.txt                # chỉ add_subdirectory() các phần con theo Kconfig
    ├── core/
    │   ├── include/                  # kiểu chung: format, frame, event, ctrl ID
    │   ├── system/                   # init, lifecycle
    │   ├── buffer/                   # pool, cấp phát, ownership, refcount
    │   ├── pipeline/                 # node, port, binding, data flow
    │   ├── event/                    # pub/sub, dispatch
    │   ├── resource/                 # vùng nhớ, căn lề, thuộc tính cache
    │   └── clock/                    # media clock, timestamp (cho A/V sync sau này)
    ├── camera/
    │   ├── include/
    │   ├── sensor/                   # quản lý sensor + drivers/<tên_sensor>, tpg/
    │   ├── capture/                  # nhận frame từ CSI-2, xoay buffer
    │   └── control/                  # thực thi exposure/gain/focus, xử lý độ trễ áp dụng
    ├── isp/
    │   ├── include/
    │   ├── pipeline/                 # điều phối luồng ISP
    │   ├── statistics/               # nhận và chuẩn hoá statistics
    │   ├── iq/                       # calibration + tuning (tách ra khi có tool tuning)
    │   └── algorithms/               # AE, AWB, AF
    ├── video/                        # để sau
    ├── display/                      # để sau
    └── audio/                        # để sau
```

Driver theo layout chung của `components/driver` (xem `Add_New_Driver_or_Device.md`):
- Mỗi ngoại vi có một thư mục `driver/<ngoại vi>/` và một header công khai `neon_<ngoại vi>.h`.
- Backend nằm ở `platform/<family>/`, được chọn bằng `neon_mdf_driver_select_family_source()`.
- Driver đặt tên theo ngoại vi, không theo chip hay IP. Không có tầng `hal/` riêng: phần thanh ghi nằm trong `platform/<family>/` của driver, ví dụ `phy/platform/fx1/fx1_csidphy_regs.h`.

Lưu ý build:
- `neon-mdf/CMakeLists.txt` gọi `add_subdirectory(components/neon_media_runtime)` khi bật `CONFIG_NEON_MDF_MEDIA_RUNTIME`.
- `neon_media_runtime/CMakeLists.txt` chỉ `add_subdirectory()` các phần con theo Kconfig, giống `components/driver/CMakeLists.txt`.
- Mỗi phần con là một static library có tên tường minh `neon_mdf_media_<phần>` và alias `neon_mdf::media_<phần>`. Ví dụ: `neon_mdf_media_core` / `neon_mdf::media_core`. Tên thư mục ngắn (`core`, `camera`, `isp`) không gây trùng, vì target không đặt theo tên thư mục.
- Phụ thuộc khai báo bằng `target_link_libraries()`.
- TODO: `video/`, `display/`, `audio/` mới có khung thư mục (`.gitkeep`), chưa có nội dung.

### 4.1 Quy ước đặt tên

Theo cùng quy ước với `driver/` và `neon_camera_runtime`. Mọi tên công khai đều có tiền tố `neon_media_`.

| Loại | Quy ước | Ví dụ |
|---|---|---|
| Hàm của core | `neon_media_<đối tượng>_<động từ>()` | `neon_media_pool_init()`, `neon_media_event_subscribe()` |
| Hàm của phần con | `neon_media_<phần>_<động từ>()` | `neon_media_camera_start()`, `neon_media_isp_configure()` |
| Kiểu | `neon_media_<tên>_t` | `neon_media_frame_t`, `neon_media_format_t` |
| Bảng ops | `neon_media_<tên>_ops_t`, giống `neon_phy_ops_t` | `neon_media_ctrl_ops_t`, `neon_media_sensor_ops_t` |
| Handle | `neon_media_<tên>_handle_t`, giống `neon_csi_handle_t` | `neon_media_camera_handle_t` |
| Enum và macro | `NEON_MEDIA_<NHÓM>_<TÊN>` | `NEON_MEDIA_EVENT_FRAME_DONE`, `NEON_MEDIA_CTRL_EXPOSURE_US` |
| Mã trả về | `neon_status_t` (`NEON_OK`, `NEON_BUSY`, ...) của `driver/neon_types.h` | Không tạo kiểu lỗi riêng |
| File | `neon_media_<tên>.h` / `.c` | `core/include/neon_media_frame.h` |
| Include guard | `NEON_MEDIA_<TÊN>_H` | `NEON_MEDIA_FRAME_H` |
| CMake target | `neon_mdf_media_<phần>`, alias `neon_mdf::media_<phần>` | `neon_mdf_media_camera` |
| Kconfig | `CONFIG_NEON_MDF_MEDIA_<...>` | `CONFIG_NEON_MDF_MEDIA_CAMERA`, `CONFIG_NEON_MDF_MEDIA_3A_TASK_PRIORITY` |

Phần `isp` và `camera` của runtime không dùng `neon_isp_*` hay `neon_camera_*`. Hai tiền tố đó đã thuộc về driver ISP và `neon_camera_runtime` hiện có.

## 5. Quy tắc phụ thuộc

```
neon_mdf_media_camera ──► neon_mdf_media_core
        │
        └───────────────► neon_mdf_driver (csi, i2c, gpio)

neon_mdf_media_isp ─────► neon_mdf_media_core
        │
        └───────────────► neon_mdf_driver (isp)
```

- `core` không link component media nào khác.
- Các component media không link lẫn nhau. Mọi liên kết đi qua `core/pipeline`, event và control ID.
- 3A trong `isp` chỉnh sensor qua control ID của core (`neon_media_ctrl_ops_t`), không gọi sensor driver trực tiếp.

## 6. core

### 6.1 Kiểu chung (`include/`)
- `neon_media_format_t`: width, height, pixfmt (RAW10/12/16, YUV...), bayer order, stride, cờ HDR (High Dynamic Range).
- `neon_media_frame_t`: media type, địa chỉ, kích thước, format, `frame_id`, timestamp, exposure index (Long/Short), con trỏ metadata, trạng thái (ok/err), refcount, pool chủ quản.
- `neon_media_event_t`: `NEON_MEDIA_EVENT_FRAME_START`, `NEON_MEDIA_EVENT_FRAME_DONE`, `NEON_MEDIA_EVENT_FRAME_DROPPED`, `NEON_MEDIA_EVENT_STATS_READY`, `NEON_MEDIA_EVENT_ERROR`... kèm `frame_id` và timestamp.
- `neon_media_ctrl_id_t` và `neon_media_ctrl_ops_t`: `set(id, val, at_frame)`, `get(id, &val)`. Các ID ví dụ: `NEON_MEDIA_CTRL_EXPOSURE_US`, `NEON_MEDIA_CTRL_GAIN_MDB`, `NEON_MEDIA_CTRL_APPLY_DELAY`, `NEON_MEDIA_CTRL_FLIP`, `NEON_MEDIA_CTRL_FPS`, `NEON_MEDIA_CTRL_HDR_RATIO`.

### 6.2 Buffer
- Pool tĩnh hoặc cấp phát lúc init, căn lề theo capability driver báo lên.
- Ownership: `NEON_MEDIA_BUFFER_FREE` → `_HW` → `_READY` → `_APP` → `_FREE`, có refcount khi nhiều consumer.
- Pool cạn thì không nạp buffer cho phần cứng; phần cứng tự drop frame, runtime đếm và phát `NEON_MEDIA_EVENT_FRAME_DROPPED`.
- Cache: runtime gọi API cache của driver (invalidate/clean), hoặc đặt pool trong vùng non-cacheable. Lựa chọn dựa trên capability.

### 6.3 Pipeline
- Node có port vào/ra; binding giữa các port.
- Hai loại liên kết:
  - **Liên kết phần cứng** (CSI-2 → ISP qua AXI4-Stream, Advanced eXtensible Interface): chỉ đàm phán format và giữ thứ tự start/stop, không có buffer.
  - **Liên kết qua buffer** (qua DDR): có queue, refcount, backpressure.
- Start theo thứ tự từ cuối luồng về đầu luồng (ISP → CSI-2 → sensor stream-on); stop theo thứ tự ngược lại.

### 6.4 Event
- Pub/sub. Callback chạy trong context của bên phát, nên bên nhận chỉ copy event vào queue của mình rồi return.

### 6.5 Resource và clock
- `resource`: vùng nhớ, căn lề, thuộc tính cache; dựa trên capability driver, không hardcode.
- `clock`: một media clock chung để gắn timestamp; về sau dùng cho đồng bộ A/V (Audio/Video).

## 7. camera

- **sensor**: bảng ops `neon_media_sensor_ops_t` (`init`, `set_mode`, `start`, `stop`, `set_ctrl`, `get_ctrl`, `get_caps`). Sensor driver không đặc thù SoC nên đặt tại `sensor/drivers/<tên>`. TPG được làm như một sensor ảo cùng interface.
- **capture**: nhận buffer-done từ driver CSI-2, xoay pool N buffer vào các slot DMA (Direct Memory Access), gắn metadata (`frame_id`, timestamp, VC (Virtual Channel)/exposure), phát frame.
- **control**: hiện thực `neon_media_ctrl_ops_t` cho sensor; xử lý độ trễ áp dụng (`at_frame`) và kiểm tra giới hạn của sensor.

## 8. isp

- **pipeline**: cấu hình input theo format của camera, start/stop theo thứ tự của core.
- **statistics**: nhận statistics từ driver ISP, gắn `frame_id`, bỏ statistics của frame lỗi hoặc frame bị drop.
- **algorithms**: AE (Auto Exposure), AWB (Auto White Balance), AF (Auto Focus). Phần thuần thuật toán không đụng thanh ghi hay RTOS, để chạy test trên PC được bằng statistics dump.
- **iq**: dữ liệu calibration theo sensor/module (black level, LSC (Lens Shading Correction), ma trận màu) và cơ chế tuning.
- Task 3A riêng, priority và stack qua Kconfig (`CONFIG_NEON_MDF_MEDIA_3A_TASK_PRIORITY`, `CONFIG_NEON_MDF_MEDIA_3A_TASK_STACK_SIZE`); có option tắt task để gọi tay khi debug.

## 9. Mô hình task (FreeRTOS)

| Ngữ cảnh | Thuộc | Việc làm |
|---|---|---|
| ISR CSI-2 | driver | Đọc/xoá `IRQ_STATUS`, notify capture task |
| ISR ISP | driver | Báo statistics ready |
| capture task | camera | Buffer done, xoay pool, phát `NEON_MEDIA_EVENT_FRAME_DONE` / `NEON_MEDIA_EVENT_FRAME_DROPPED` |
| 3A task | isp | Statistics → thuật toán → `ctrl->set(...)` |
| control (trong capture task hoặc task riêng) | camera | Ghi I2C xuống sensor đúng thời điểm frame |
| app task | app | `neon_media_camera_get_frame()` / `neon_media_camera_release_frame()` |

## 10. Ghi chú đặc thù FX1 (theo HAS v1.2 / MAS v1.4)

Các điểm này được driver CSI-2 thể hiện thành capability hoặc API; runtime chỉ sử dụng.

- DMA của CSI-2 có 2 buffer ping-pong; ownership do phần cứng quản lý (`HW_OWNED`, `SW_OWNED_VALID`, `SW_OWNED_ERROR`; trong `neon_csi.h` là `NEON_CSI_BUFFER_HW_OWNED`, `NEON_CSI_BUFFER_SW_VALID`, `NEON_CSI_BUFFER_SW_ERROR`).
- Địa chỉ base được chốt khi DMA claim buffer, nên runtime có thể đổi địa chỉ của buffer mỗi lần trả nó lại cho phần cứng. Đây là cơ sở cho pool N buffer không cần copy. `neon_csi_release_frame()` hiện chưa nhận địa chỉ mới, cần bổ sung.
- Metadata và embedded data của từng VC nằm trong vùng metadata của buffer.
- Staggered HDR: VC0/VC1 là Long/Short; fusion làm ở ISP. Runtime ghép cặp theo `frame_id`.
- `dma_format` chỉ được đổi khi DMA đã tắt và hết busy. Máy trạng thái của runtime phải ép thứ tự này.
- Sau soft reset phải đọc `BUFFER_OWNERSHIP` để thu hồi các buffer software đang giữ.
- Cache D-cache write-back của CVA6 và DMA không coherent: cần chốt cơ chế (vùng non-cacheable qua PMA (Physical Memory Attributes) hay invalidate theo dòng).

## 11. Lộ trình

| Giai đoạn | Nội dung | Tiêu chí xong |
|---|---|---|
| M0 | Khung component, quy tắc build, `core/include` | Build sạch, chiều phụ thuộc đúng |
| M1 | Driver CSI-2 + TPG chạy trên FreeRTOS, dump RAW | Frame TPG đúng trong DDR |
| M2 | `core/buffer` + `camera/capture`, pool N buffer | Stream liên tục, không leak, đếm drop đúng |
| M3 | Sensor thật qua I2C, `camera/control` | Ảnh RAW từ sensor, chỉnh exposure/gain được |
| M4 | `core/pipeline` + `isp/pipeline` | Pipeline CSI-2 → ISP chạy, start/stop ổn định |
| M5 | Statistics + AE/AWB + IQ | Ảnh hội tụ phơi sáng và cân bằng trắng |
| M6 | Error recovery, HDR | Tự phục hồi khi có lỗi, ghép cặp Long/Short đúng |
| Sau | video / display / audio, `core/clock` cho A/V sync | Thêm từng component theo nhu cầu |

## 12. Câu hỏi mở

- [x] Driver ISP cho `neon_mdf_media_isp` là `driver/isp` (`neon_isp.h`), cùng layout với `csi` và `phy`.
- [ ] TODO: tách `components/ftel_isp` thành hai phần, rồi bỏ component này:
  - Phần thanh ghi (các khối Bayer/RGB/YUV, output, DMA, đọc statistics, `registers/`) thành backend `driver/isp/platform/fx1/`. Hiện backend FX1 mới có simulation.
  - Phần thuật toán (`isp_3a`, AE tham chiếu, `ftel_isp_auto` và thư viện `libisp_3a.a`) chuyển vào `neon_media_runtime/isp/algorithms`.
- [ ] `neon_isp.h` có phải đổi để khớp ISP FX1 không: 4 slot xoay vòng, một frame chạy mỗi lần, WB chỉ áp khi ISP rảnh?
- [ ] ISP có output DMA riêng không, hay app chỉ nhận RAW từ CSI-2?
- [ ] CVA6 trên FX1 có hỗ trợ Zicbom (lệnh quản lý cache theo dòng) không?
- [ ] Có cần tool tuning IQ trên PC không? Nếu có, giao thức gì (UART/USB)?
- [ ] API công khai cho app: kiểu gọn như CMW, hay kiểu V4L2 (Video for Linux Two)?
