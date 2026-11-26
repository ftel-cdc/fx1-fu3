# RUNTIME_NPU_APP — run graph NEON trên VP_FX1_V2.0

Ứng dụng chạy firmware FreeRTOS, runtime SDK, NPU driver FX1, Virtual Platform và Graph Fragment do Compiler cung cấp sẵn.
ISP cung cấp frame RGB 640×640×3 (layout HWC) được ghi vào `CDC_RAW_IN0 (0x81000000)`. Sau khi báo DMA hoàn tất, Runtime chuẩn hóa sang NCHW `[1,3,640,640]` theo tiêu chuẩn model yêu cầu.

## Yêu cầu tài nguyên

Model yêu cầu duy nhất 1 input `[1,3,640,640]`, thứ tự kênh RGB. 
Runtiem thực hiện các bước:

1. Kiểm tra frame nguồn: `[1,640,640,3]`, layout NHWC, RGB uint8 và địa chỉ hợp lệ trong vùng `CDC_RAW_IN0`.
2. Chuẩn hóa layout input: `dst[(c*640 + y)*640 + x] = src[y*row_stride + 3*x + c]`.
3. ÁP dụng lượng tử hóa theo `.nsm`, trong trường hợp `/examples/RUNTIME_NPU_APP/models/yolov8n_full_integer_quant.nsm`:`q = pixel - 128`; model UINT8 scale `1.0`, zero point `0`.
4. Bind DLTensor layout `[1,3,640,640]` vào input graph. Frame HWC lưu nguyên vẹn cho tới khi graph chạy xong và được giải phóng.

## Memory map

Bộ nhớ DRAM trên VP_FX1_V2.0 dung lượng 256 MiB bắt đầu tại `0x80000000` được phân vùng:

| Vùng | Địa chỉ | Nội dung |
|---|---:|---|
| `CDC_FW` | `0x80000000-0x80FFFFFF` | FreeRTOS firmware và heap runtime. |
| `CDC_RAW_IN0` | `0x81000000–0x817FFFFF` | ISP ghi frame RGB. |
| `CDC_RAW_IN0` | `0x81800000–0x81FFFFFF` | Runtime chuẩn hóa frame RGB input. |
| `CDC_ISP_OUT0` | `0x82000000–0x83FFFFFF` | Output tensor của graph. |
| `CDC_VPU_OUT0` | `0x84000000–0x85FFFFFF` | Kênh truyền DMA phụ (dự phòng). |
| `CDC_NPU_WGT0` | `0x86000000–0x87FFFFFF` | Bộ đệm đích nạp trọng số qua kênh DMA ch 0. |
| `CDC_NPU_WORK0` | `0x88000000–0x8BFFFFFF` | Vùng nhớ làm việc chính của NPU. |
| `FX1_HAS_WIN_BASE` | `0x8B000000-0x8BFFFFFF` | Cửa sổ trung chuyển HasNpuTop: Bố trí `[Padded Input][W][Bias][Scale][Shift][LUT][Output]`. |
| `BLOB` | `0x8C000000–0x8EFFFFFF` | Model `.nsm` dùng bởi GraphLoader trong graph lifetime. |
| `ARENA` | `0x8F000000–0x8FFFFFFF` | Tensor Arena: Không gian tái sử dụng bộ nhớ cho toàn bộ các tensor trung gian (`INTERNAL`). |

## Build và chạy inference

Chỉ build firmware, không chạy mô phỏng:

```bash
cd workspace/neon_mdf
make -C examples/RUNTIME_NPU_APP clean
make -C examples/RUNTIME_NPU_APP -j"$(nproc)"
```

Build firmware và chạy inference trên VP_FX1_V2.0:

```bash
cd workspace/neon_mdf
./examples/RUNTIME_NPU_APP/run.sh
```

Runner ghi log tại:
`examples/RUNTIME_NPU_APP/build/FX1_V2_RUNTIME/vp_neon_graph.log`.