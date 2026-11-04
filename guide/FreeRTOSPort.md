# Wire FreeRTOS cho 1 kiến trúc CPU mới

`cmake/freertos.cmake`'s `neon_mdf_add_freertos()` là điểm dispatch duy nhất
chọn **port** FreeRTOS-Kernel (thư mục `rtos/FreeRTOS-Kernel/portable/GCC/<PORT>/`)
theo `arch` của target — hiện đã wire **3 port cho 2 họ CPU**:
`GCC_ARM_CM0` (`FU2`), `GCC_ARM_CM4F` (`WEACT_F412`), `GCC_RISC_V_GENERIC` +
chip extension `RISCV_no_extensions` (`CH32V003F4P6`). File này giải thích
cơ chế dispatch + 2 pattern glue-code thật đã cần khi thêm port mới, để
thêm kiến trúc thứ 3 không phải đoán lại từ đầu.

Đọc `guide/AddNewMCU.md` trước nếu target còn chưa tồn tại trong SDK —
FreeRTOS chỉ là 1 bước trong quy trình thêm target đó, không phải bước đầu.

## 1. Cơ chế dispatch (`cmake/freertos.cmake`)

```cmake
if(_arch MATCHES "^cortex-m0")
  set(_freertos_port GCC_ARM_CM0)
elseif(_arch STREQUAL "cortex-m4")
  set(_freertos_port GCC_ARM_CM4F)
elseif(_arch MATCHES "^riscv")
  set(_freertos_port GCC_RISC_V_GENERIC)
  set(FREERTOS_RISCV_EXTENSION RISCV_no_extensions CACHE STRING "" FORCE)
else()
  message(FATAL_ERROR "chua wire port FreeRTOS cho arch '${_arch}'")
endif()
```

`_arch` đọc từ `NEON_MDF_TARGET_<TARGET>_ARCH` (`cmake/mcu_variants.cmake`,
xem `guide/AddNewMCU.md` mục đăng ký target). Thêm kiến trúc mới = thêm 1
nhánh `elif` chọn đúng thư mục port có sẵn trong
`rtos/FreeRTOS-Kernel/portable/GCC/` (kernel đã vendor sẵn toàn bộ port
chính thức — liệt kê `ls rtos/FreeRTOS-Kernel/portable/GCC/` để xem port
nào có sẵn cho CPU đích trước khi viết glue code gì, khả năng cao đã có).

Sau khi chọn port, `add_subdirectory(rtos/FreeRTOS-Kernel ...)` (hệ thống
CMake build chính thức của kernel — SDK KHÔNG tự liệt kê source file tay)
tạo 2 target `freertos_kernel` (code thuần C: tasks/queue/timers/heap_4...)
và `freertos_kernel_port` (`port.c`/`portasm`/`portASM.S` riêng của port đã
chọn) — cả 2 KHÔNG tự có cờ kiến trúc (`-mcpu`/`-march`...) vì không link
`neon_mdf_device_<TARGET>` — file này tự áp bằng
`neon_mdf_apply_cortex_m_arch()`/`neon_mdf_apply_riscv_arch()` ngay sau đó.
Port CPU mới đọc kỹ đoạn tương ứng để áp đúng flag cho cả 2 target, không
chỉ 1.

## 2. Pattern "Direct Routing" — khi tên handler của port khác vector table sinh chung

**Vấn đề chung**: `mcu-description`'s codegen luôn sinh vector table dùng
tên handler chung theo chuẩn CMSIS (`SVC_Handler`/`PendSV_Handler`/
`SysTick_Handler`, `weak`) — nhưng 1 số port FreeRTOS (vd `GCC_ARM_CM4F`)
tự đặt tên khác (`vPortSVCHandler`/`xPortPendSVHandler`/
`xPortSysTickHandler`) và **tự kiểm tra tên đó khớp vector table thật** lúc
`xPortStartScheduler()` chạy (`configASSERT` trong `portmacro.h`) — weak-
override tự động (dựa vào trùng tên) không xảy ra vì tên vốn đã khác nhau.

**Triệu chứng nếu bỏ qua** (đã gặp thật lúc bring-up `WEACT_F412`):
`xPortStartScheduler()` treo vĩnh viễn tại `configASSERT` — xác nhận qua
đọc PC bằng SWD lúc debug, không phải HardFault hay lỗi rõ ràng khác.

**Fix thật** (xem `examples/FreeRTOS_Blinky/Src/main.c`, app tự viết —
KHÔNG sửa `startup_<target>.s` dùng chung cho mọi app):

```c
void vPortSVCHandler(void);
void xPortPendSVHandler(void);
void xPortSysTickHandler(void);

__attribute__((naked)) void SVC_Handler(void)     { __asm volatile("b vPortSVCHandler"); }
__attribute__((naked)) void PendSV_Handler(void)  { __asm volatile("b xPortPendSVHandler"); }
void SysTick_Handler(void) { xPortSysTickHandler(); }
```

`SVC_Handler`/`PendSV_Handler` PHẢI `naked` + nhảy bằng lệnh `b` (không
`bl`/gọi hàm C bình thường) — `vPortSVCHandler`/`xPortPendSVHandler` là hàm
"naked" của chính port (tự quản lý `EXC_RETURN`/stack để chuyển task), cần
giữ nguyên ngữ cảnh exception y như được nhảy vào trực tiếp từ vector
table. `SysTick_Handler` thì không (chỉ tăng tick, không đụng
`EXC_RETURN`), gọi hàm C bình thường là đủ.

Đồng thời `FreeRTOSConfig.h` phải tắt `configCHECK_HANDLER_INSTALLATION` (0)
cho port kiểu này — bật mặc định giả định "Indirect Routing" (tên khớp
sẵn), không hợp pattern trên (xem comment trong
`tools/templates_freertos/Inc/FreeRTOSConfig.h`).

**Khi nào cần lo việc này**: kiểm tra `portable/GCC/<PORT>/port.c` của port
mới — có `configASSERT(pxVectorTable[...] == vPortSVCHandler)` hay tương tự
không. `GCC_ARM_CM0` (FU2) KHÔNG có bước này (tên trùng sẵn, weak-override
tự động hoạt động) — không phải mọi port Cortex-M đều cần glue này.

## 3. Pattern "CLINT-less" — khi MCU không có bộ định thời chuẩn RISC-V

**Vấn đề chung**: port `GCC_RISC_V_GENERIC` mặc định giả định có **CLINT**
(Core Local Interrupter — bộ định thời `mtime`/`mtimecmp` chuẩn RISC-V) để
tự sinh tick interrupt. MCU vendor-specific (vd WCH's QingKe V2A trên
`CH32V003F4P6`) có thể không có CLINT — dùng bộ định thời + interrupt
controller riêng của vendor (PFIC ở đây) — port cần được cho biết qua chip
extension **`RISCV_no_extensions`** (`FREERTOS_RISCV_EXTENSION`, xem mục 1
— đã có sẵn trong kernel vendor, KHÔNG cần tự viết extension, chỉ cần chọn
đúng cái) → 2 hàm `weak` của port bắt buộc app tự ghi đè:

- **`vPortSetupTimerInterrupt()`** (`port.c`) — tự cấu hình bộ định thời
  thật + bật interrupt cho nó qua interrupt controller thật. Gọi 1 lần từ
  `xPortStartScheduler()`, TRƯỚC `xPortStartFirstTask()` (an toàn để đổi
  `mtvec` ở đây — chưa có ngắt nào được bật). Ví dụ thật (`CH32V003F4P6`,
  xem `examples/FreeRTOS_Blinky/Src/main.c` cho code đầy đủ + toàn bộ tra
  cứu datasheet): đặt `mtvec` sang **Direct mode** (bit`[1:0]=00`, MỌI
  exception/interrupt nhảy về 1 địa chỉ `freertos_risc_v_trap_handler` của
  `portASM.S` — khác "address mode" vector-table-riêng-từng-IRQ mà
  `startup_<target>.s` dùng chung cấu hình sẵn lúc reset), cấu hình bộ đếm
  vendor-specific, bật ngắt qua interrupt controller vendor-specific.

- **`freertos_risc_v_application_interrupt_handler()`** (`portASM.S`) — vì
  `portasmHAS_MTIME=0` (do chọn `RISCV_no_extensions`), port KHÔNG còn tự
  nhận diện "đây có phải tick timer không" giúp mình nữa (logic đó chỉ tồn
  tại trong nhánh `portasmHAS_MTIME!=0`) — app phải tự đọc nguyên nhân ngắt
  (thanh ghi báo nguyên nhân thật của kiến trúc, vd `mcause` trên RISC-V
  chuẩn) và tự gọi `xTaskIncrementTick()`/`vTaskSwitchContext()` khi đúng là
  tick timer — bỏ qua (không loop vô hạn) cho ngắt khác để không "treo" oan
  nếu 1 ngắt lạ bị bật nhầm.

`portYIELD()` (dùng bởi `vTaskDelay()`/... khi cần chuyển task chủ động
NGOÀI ngắt) hoạt động độc lập với CLINT/interrupt-controller — là
synchronous exception chuẩn của kiến trúc (`ecall` trên RISC-V), không cần
glue code thêm.

**Khi nào cần lo việc này**: MCU RISC-V không có CLINT chuẩn (đọc kỹ
reference manual phần interrupt controller/timer trước khi giả định) —
kiểm tra `rtos/FreeRTOS-Kernel/portable/GCC/RISC-V/chip_specific_extensions/`
xem đã có sẵn extension khớp thiết bị chưa (`RISCV_no_extensions` dùng
được cho bất kỳ MCU nào tự lo timer/interrupt hoàn toàn, không riêng WCH).

## 4. `FreeRTOSConfig.h` theo kiến trúc

`tools/templates_freertos/Inc/FreeRTOSConfig.h` (file của APP, không phải
SDK — mỗi app tự có bản riêng, sửa thoải mái) dùng `#if defined(<TARGET>)`
để chọn field khác nhau theo port — port mới cần đọc kỹ `portmacro.h` của
chính nó xem cần macro `config*` nào bắt buộc (vd `GCC_ARM_CM4F` bắt buộc
`configMAX_SYSCALL_INTERRUPT_PRIORITY`/`configPRIO_BITS` khớp NVIC thật;
`GCC_ARM_CM0` thì không, vì port này tự cố định priority bằng hằng số).
Nếu toolchain bare-metal của kiến trúc mới không có libc thật (như
`riscv-none-elf-gcc` build cho `ilp32e` — không có multilib, xem
`guide/AddNewMCU.md`), `freertos_kernel`/`freertos_kernel_port` (dùng
`memcpy`/`memset`/`strlen` không điều kiện trong `heap_4.c`/`tasks.c`/...)
cần shim libc tối thiểu tự cấp — xem `arch/RISCV/Core/Inc/libc_shim/` +
`cmake/freertos.cmake`'s nhánh `_arch MATCHES "^riscv"` làm mẫu.

## 5. Checklist thêm port FreeRTOS cho kiến trúc mới

1. `ls rtos/FreeRTOS-Kernel/portable/GCC/` — port chính thức đã có sẵn cho
   CPU đích chưa (khả năng cao có, kernel vendor rất đầy đủ).
2. Thêm nhánh `elif` trong `neon_mdf_add_freertos()` (mục 1) chọn đúng thư
   mục port + (nếu RISC-V) đúng chip extension.
3. Áp đúng cờ kiến trúc cho CẢ `freertos_kernel` VÀ `freertos_kernel_port`
   (2 target riêng, không tự thừa hưởng nhau).
4. Đọc `port.c`/`portmacro.h` của port mới — có `configASSERT` kiểm tra tên
   handler khớp vector table không (mục 2)? Có giả định CLINT/timer chuẩn
   không, MCU đích có thật không (mục 3)?
5. Nếu toolchain không có libc thật, thêm shim (mục 4).
6. Viết `FreeRTOSConfig.h` cho app test đầu tiên, điền đủ `config*` bắt
   buộc theo `portmacro.h`.
7. Build + verify thật trên board (không tin build sạch) — chạy 2 task
   preemptive tối thiểu (giống `examples/FreeRTOS_Blinky`) để chứng minh
   scheduler/tick/context-switch hoạt động thật, không chỉ compile qua.
