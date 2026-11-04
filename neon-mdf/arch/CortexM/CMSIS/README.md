# Core/Arch/CortexM/CMSIS

- `CMSIS_5/` — vendor từ repo chính thức của ARM
  (`https://github.com/ARM-software/CMSIS_5`), chỉ chứa sparse
  `CMSIS/Core/Include` (để không kéo theo toàn bộ monorepo
  DSP/NN/RTOS2/device packs... mà SDK này không cần). Không sửa tay bất kỳ
  file nào trong này.

  **Trạng thái hiện tại:** thư mục này đang là **vendor thuần** (file đã copy
  sẵn, không phải git submodule sống — repo gốc chưa `git init`). Nếu muốn
  tái lập lại thành git submodule sau khi `git init` repo này (để dễ
  `git submodule update --remote` nhận fix từ ARM sau này):
  ```sh
  rm -rf Core/Arch/CortexM/CMSIS/CMSIS_5   # xoá bản vendor thuần hiện tại
  git submodule add https://github.com/ARM-software/CMSIS_5.git Core/Arch/CortexM/CMSIS/CMSIS_5
  cd Core/Arch/CortexM/CMSIS/CMSIS_5
  git sparse-checkout init --cone
  git sparse-checkout set CMSIS/Core/Include
  ```

- `Device/<Family>/` — header/linker/startup sinh tự động từ
  `mcu-description/` (xem `mcu-description/tools/generate.py`), vd `Device/FU2/`.
  Không phân theo vendor (`<Vendor>/<Family>/`) — chỉ 1 cấp tên family cho gọn.
  Không sửa tay các file sinh ra (chỉ `system_<DEVICE>.c/.h` là viết tay).
