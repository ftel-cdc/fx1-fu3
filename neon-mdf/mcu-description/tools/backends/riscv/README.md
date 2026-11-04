# RISC-V backend — RESERVED, chưa triển khai

Thư mục này giữ chỗ cho backend sinh code (header/linker/startup) khi SDK hỗ
trợ thêm kiến trúc RISC-V (xem `mcu-description/schema/arch/riscv.schema.yaml`
và `Core/Arch/RISCV/README.md`).

Khi triển khai, dự kiến cần các file tương ứng với `backends/cortex_m/`:
- `gen_riscv_header.py` — sinh header register struct (peripheral logic dùng
  chung schema/common, chỉ core/CSR khác Cortex-M)
- `gen_linker_script.py` — linker script RISC-V (khác cú pháp MEMORY/SECTIONS
  một phần do ABI/relocation model khác)
- `gen_startup.py` — startup + trap vector (PLIC/CLINT thay vì NVIC, xem ghi
  chú trong riscv.schema.yaml)

`generate.py` ở thư mục cha đã dispatch theo `device.arch` — chỉ cần thêm
`elif device.arch.startswith("riscv"):` trỏ vào backend này khi sẵn sàng.
