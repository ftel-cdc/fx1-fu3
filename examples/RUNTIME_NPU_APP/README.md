# RUNTIME_NPU_APP — chạy graph NEON trên VP_FX1_V2.0

Ứng dụng này là nơi ghép firmware FreeRTOS, runtime SDK, NPU driver FX1 V2.0,
Virtual Platform và artifact do compiler tạo. Graph `.nsm` được truyền cho VP
riêng với ELF firmware; firmware không chứa model.

## Cấu trúc

```text
examples/RUNTIME_NPU_APP/
├── Makefile                         # cross-build firmware V2
├── main.cpp                         # main(), tạo inferenceTask(), gọi Graph C API
├── run.sh                           # build rồi chạy VP
├── run_vp.py                        # gọi VP, lưu UART và dừng khi PASS/FAIL
├── include/RuntimeInputProvider.h   # API tạo input int8 tổng hợp cho runtime
├── src/RuntimeInputProvider.cpp     # Gaussian int8 tổng hợp, không có đường nhận ISP
├── models/mobilenet_nano_qat_int8_nchw_native/
│   └── mobilenet_nano_qat_int8.nsm  # graph NEONGRF, chứa cả 10 fragment
├── firmware_support/FX1_V2.0/        # BSP/FreeRTOS ABI tối thiểu cho firmware V2
├── platform/virtual_platform/fx1/vp/  # VP binaries, cấu hình, giấy phép và tài liệu
└── build/FX1_V2_RUNTIME/            # artifact được tạo cục bộ khi build/run
```

Chỉ file `.nsm` graph là artifact compiler cần commit để chạy model này. Các
manifest, fragment sidecar, MLIR/NPZ, compiler log và vectors đã bỏ vì runtime
đọc fragment trực tiếp từ graph container; ứng dụng tự tạo input tổng hợp.

Virtual Platform binaries, cấu hình, tài liệu và giấy phép nằm tại
`examples/RUNTIME_NPU_APP/platform/virtual_platform/fx1/vp/`; backend NPU V2 nằm tại
`neon-mdf/components/driver/runtime_npu/platform/FX1_V2.0/`. Ví dụ
`FX1_FreeRTOS_Smoke` được dành riêng để smoke-test boot/FreeRTOS của VP V3.0.

## Build và chạy

Chỉ build firmware, không khởi chạy mô phỏng:

```bash
cd /home/balv/workspace/neon_mdf
make -C examples/RUNTIME_NPU_APP clean
make -C examples/RUNTIME_NPU_APP -j"$(nproc)"
```

Để build rồi chạy inference trên VP V2.0:

```bash
cd /home/balv/workspace/neon_mdf
./examples/RUNTIME_NPU_APP/run.sh
```

Lệnh này chạy `vp_fx1_full_soc -c default.yaml --fw <ELF> --int-flash
<model.nsm> --quantum 5000 --sim-ms 20000000`. Có thể đổi giới hạn mô phỏng
bằng `VP_QUANTUM` và `VP_SIM_MS`. Toolchain mặc định dò `riscv-none-elf-*`
trong `PATH`, sau đó dùng xPack tại `$HOME/tools/riscv`.

Runner ghi log tại
`examples/RUNTIME_NPU_APP/build/FX1_V2_RUNTIME/vp_neon_graph.log`. Các marker
mong đợi:

```text
[NEON_E2E] MDF runtime firmware booted
[NEON_E2E] VP_FX1_V2.0 graph inference start
[NEON_E2E] INPUT_READY addr=0x81000000 layout=NCHW shape=[1,3,32,32] dtype=int8 bytes=3072 hash=...
[NEON_E2E] FRAGMENT_BEGIN index=0
...
[NEON_E2E] FRAGMENT_END index=9 status=0 bytes=2
[NEON_E2E] OUTPUT_OK addr=0x82000000 bytes=2 values=... hash=...
[NEON_E2E] PASS (graph execution completed; no golden comparison for synthetic input)
```

Khi thành công, runner xác nhận đủ 10 fragment kết thúc với `status=0`, kiểm
hash output UART rồi ghi hai byte output nhận được vào
`build/FX1_V2_RUNTIME/output.bin`; giá trị int8 và hash nằm trong
`output_int8.txt`. Các file này nằm dưới `build/`, được ignore và không cần
commit. Firmware ghi output trực tiếp tại `0x82000000` trước khi gửi giá trị
qua UART. `PASS` xác nhận graph chạy hết và output khác sentinel; input tổng
hợp không được so với golden output. Thời gian mô phỏng có thể dài.

## Input tổng hợp hiện tại

Graph `.nsm` chứa metadata tensor; model này có input shape `[1,3,32,32]`,
layout NCHW, dtype int8 và input scale `0.006742109...`. Mặc định
`RuntimeNpuAppPrepareInput()` sinh 3,072 số
int8 giả ngẫu nhiên theo phân phối Gaussian rời rạc gần đúng, mean 0 và độ lệch
chuẩn khoảng 41 mức lượng tử (xấp xỉ 0.276 ở miền thực theo input scale), rồi
ghi trực tiếp vào `CDC_RAW_IN0 = 0x81000000`. Seed mặc định `0x4E454F4E`
(`"NEON"`) giúp lặp lại cùng input khi chạy lại VP. Đây là input thử nghiệm,
không phải frame camera hoặc vector golden. Output int8 shape `[1,2]` được ghi
vào `CDC_ISP_OUT0 = 0x82000000`.
Ứng dụng hiện không có API nhận frame hoặc kết nối với ISP; input luôn được tạo
trực tiếp tại `CDC_RAW_IN0` trước khi gọi `NeonGraphRun()`.

## Chạy từng bước

Từ thư mục SDK:

```bash
cd /home/balv/workspace/neon_mdf
```

1. Xác nhận graph đầu vào có tồn tại và là container `NEONGRF`:

   ```bash
   test -s examples/RUNTIME_NPU_APP/models/mobilenet_nano_qat_int8_nchw_native/mobilenet_nano_qat_int8.nsm
   head -c 7 examples/RUNTIME_NPU_APP/models/mobilenet_nano_qat_int8_nchw_native/mobilenet_nano_qat_int8.nsm
   ```

   Hợp đồng input của model này là NCHW `[1,3,32,32]`, int8, 3,072 byte.
   Graph container tự chứa cả 10 fragment; không cần thư mục sidecar.
2. Build firmware (không chạy VP):

   ```bash
   make -C examples/RUNTIME_NPU_APP clean
   make -C examples/RUNTIME_NPU_APP -j"$(nproc)"
   ```

   ELF tạo tại `examples/RUNTIME_NPU_APP/build/FX1_V2_RUNTIME/neon_graph_vp_v2.elf`.
   File `.nsm` vẫn là input riêng, không nằm trong firmware.
3. Build rồi khởi chạy VP V2 và inference:

   ```bash
   ./examples/RUNTIME_NPU_APP/run.sh
   ```

   Runner truyền ELF và `.nsm` riêng cho `vp_fx1_full_soc`. Firmware chép model
   từ IFLASH `0x04000000` sang BLOB `0x8C000000`, sinh input trực tiếp ở
   `0x81000000`, rồi gọi `NeonGraphRun()`.
4. Theo dõi UART trong terminal. `FRAGMENT_BEGIN`/`FRAGMENT_END` phải lần lượt
   đi từ index 0 đến 9; mỗi `FRAGMENT_END` có `status=0`. Sau đó cần thấy
   `OUTPUT_OK` tại `0x82000000` và `PASS`.
5. Đọc kết quả host-side:

   ```bash
   cat examples/RUNTIME_NPU_APP/build/FX1_V2_RUNTIME/output_int8.txt
   od -An -t d1 examples/RUNTIME_NPU_APP/build/FX1_V2_RUNTIME/output.bin
   ```

   `output.bin` là hai byte int8 được khôi phục từ output UART; firmware đã
   ghi cùng hai byte đó tại địa chỉ DDR `0x82000000` trước khi báo thành công.

Có thể chỉnh thời lượng/quantum VP bằng `VP_SIM_MS` và `VP_QUANTUM`, ví dụ
`VP_SIM_MS=20000000 VP_QUANTUM=5000 ./examples/RUNTIME_NPU_APP/run.sh`.

## Lần chạy model

1. `Makefile` cross-compile `main.cpp`, `RuntimeInputProvider.cpp`, runtime,
   fallback CPU, NPU V2 graph adapter, FreeRTOS và BSP thành
   `build/FX1_V2_RUNTIME/neon_graph_vp_v2.elf`.
2. Runner truyền ELF và file `.nsm` riêng cho `vp_fx1_full_soc`. VP đặt model
   ngoài ELF tại IFLASH `0x04000000`; `inferenceTask()` kiểm tra magic/size
   rồi chép model vào vùng BLOB DDR `0x8C000000` trước khi gọi parser.
3. `main()` trong `main.cpp` khởi tạo PLIC/UART, tạo `inferenceTask` rồi bật
   scheduler FreeRTOS.
4. Task tạo input/output, gọi `NPUV4Runtime::open()`, rồi lần lượt gọi
   `NeonGraphLoad()`, `NeonGraphGetInfo()`, `NeonGraphPrepare()` và
   `NeonGraphRun()`.
5. `GraphRunner` xử lý từng fragment trong graph; `ModelLoader` đọc payload
   NEONNSM từ container, `OperandBinder` gắn tensor và `Executor` dispatch opcode
   qua adapter NPU V2 hoặc CPU fallback.
6. Ứng dụng kiểm tra output sentinel, in địa chỉ/hash qua UART, rồi runner lưu
   log và kết thúc tiến trình VP khi gặp PASS/FAIL.

Xem [`components/neon_runtime/README.md`](../../neon-mdf/components/neon_runtime/README.md)
để biết ranh giới runtime/driver, bản đồ bộ nhớ, file gọi và các giới hạn tích hợp.
