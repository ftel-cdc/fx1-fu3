#!/usr/bin/env bash
# export.sh - mô phỏng $IDF_PATH/export.sh của ESP-IDF.
#
# Dùng (LƯU Ý: phải "source", không phải chạy trực tiếp, để biến môi trường
# export ra được shell hiện tại thay vì mất theo subshell):
#   . ./neon-mdf/export.sh
# hoặc:
#   source ./neon-mdf/export.sh
#
# Hỗ trợ cả bash lẫn zsh (macOS mặc định zsh) - ${BASH_SOURCE[0]} không tồn
# tại/rỗng khi script này được "source" từ zsh, nên phải tự nhận diện shell
# và dùng đúng cú pháp lấy đường dẫn file hiện tại của từng shell (đã verify
# thật: dùng nhầm BASH_SOURCE dưới zsh khiến NEON_MDF_PATH suy ra sai thành
# thư mục hiện hành thay vì thư mục chứa script này).

if [ -n "${ZSH_VERSION:-}" ]; then
  _NEON_MDF_SELF="${(%):-%N}"
else
  _NEON_MDF_SELF="${BASH_SOURCE[0]:-$0}"
fi

if [ "${_NEON_MDF_SELF}" = "${0}" ] && [ -z "${ZSH_VERSION:-}" ]; then
  echo "LOI: phai 'source' script nay, khong duoc chay truc tiep." >&2
  echo "Dung:  . ${0}" >&2
  exit 1
fi

export NEON_MDF_PATH="$(cd "$(dirname "${_NEON_MDF_SELF}")" && pwd)"
echo "NEON_MDF_PATH = ${NEON_MDF_PATH}"

# Toolchain arm-none-eabi-gcc/cmake/ninja/qemu-system-arm vẫn phải tự cài
# (brew/apt) - export.sh này KHÔNG tự cài toolchain như install.sh của
# ESP-IDF (chưa cần thiết ở quy mô hiện tại, xem README.md mục yêu cầu cài đặt).
