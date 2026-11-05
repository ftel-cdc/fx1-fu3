#!/usr/bin/env bash
# export.sh - simulates ESP-IDF's $IDF_PATH/export.sh.
#
# Use (NOTE: must be "source", not run directly, to make the export environment visible to the current
# shell because of subshell loss):
#   . ./neon-mdf/export.sh
# or:
#   source ./neon-mdf/export.sh
#
# Support for both bash and zsh (default macOS zsh) - ${BASH_SOURCE[0]} does not exist/empty when this
# script is "sourced" from zsh, so it must identify the shell itself and use the correct syntax to
# get the current file path of each shell (verified true: mistakenly using BASH_SOURCE under zsh
# causes NEON_MDF_PATH to deduce incorrectly as the current directory seen because the directory
# contains this script).

if [ -n "${ZSH_VERSION:-}" ]; then
  _NEON_MDF_SELF="${(%):-%N}"
else
  _NEON_MDF_SELF="${BASH_SOURCE[0]:-$0}"
fi

if [ "${_NEON_MDF_SELF}" = "${0}" ] && [ -z "${ZSH_VERSION:-}" ]; then
  echo "error: must 'source' this script, not run directly." >&2
  echo "Use: . ${0}" >&2
  exit 1
fi

export NEON_MDF_PATH="$(cd "$(dirname "${_NEON_MDF_SELF}")" && pwd)"
echo "NEON_MDF_PATH = ${NEON_MDF_PATH}"

# The arm-none-eabi-gcc/cmake/ninja/qemu-system-arm toolchain still has to be installed manually
# (brew/apt) - This export.sh does NOT install the toolchain itself like ESP-IDF's install.sh (not
# required at current scale, see README.md installation requirements item).
