#!/usr/bin/env python3
"""gen_config_header.py - sinh sdkconfig.h + sdkconfig.cmake từ file Kconfig +
sdkconfig, dùng kconfiglib (thư viện ESP-IDF/Zephyr cũng dùng, không tự viết
parser Kconfig từ đầu).

Dùng (thường được gọi từ CMake, xem cmake/kconfig.cmake):
  python3 gen_config_header.py --kconfig Kconfig --sdkconfig /path/to/sdkconfig \\
      --header-out build/config/sdkconfig.h --cmake-out build/config/sdkconfig.cmake

Quy ước đặt tên giống hệt ESP-IDF: symbol trong Kconfig KHÔNG có tiền tố
(vd `NEON_MDF_HAL_UART`), macro/biến sinh ra tự động thêm `CONFIG_` (vd
`CONFIG_NEON_MDF_HAL_UART`).
"""
import argparse
import os

import kconfiglib


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--kconfig", required=True, help="File Kconfig gốc (mainmenu)")
    ap.add_argument("--sdkconfig", required=True, help="File lưu lựa chọn của app - tự tạo với default nếu chưa có")
    ap.add_argument("--header-out", required=True)
    ap.add_argument("--cmake-out", required=True)
    args = ap.parse_args()

    kconf = kconfiglib.Kconfig(args.kconfig)

    if os.path.isfile(args.sdkconfig):
        kconf.load_config(args.sdkconfig)

    os.makedirs(os.path.dirname(os.path.abspath(args.sdkconfig)), exist_ok=True)
    # Luôn ghi lại sdkconfig (giống ESP-IDF): option mới thêm sẽ tự điền
    # default vào file, option cũ không còn tồn tại sẽ bị dọn khỏi file.
    kconf.write_config(args.sdkconfig)

    os.makedirs(os.path.dirname(os.path.abspath(args.header_out)), exist_ok=True)
    os.makedirs(os.path.dirname(os.path.abspath(args.cmake_out)), exist_ok=True)

    header_lines = [
        "/* Auto-generated bởi neon-mdf/tools/kconfig/gen_config_header.py - DO NOT EDIT BY HAND. */",
        "#ifndef SDKCONFIG_H",
        "#define SDKCONFIG_H",
        "",
    ]
    cmake_lines = [
        "# Auto-generated bởi neon-mdf/tools/kconfig/gen_config_header.py - DO NOT EDIT BY HAND.",
        "",
    ]

    for sym in kconf.unique_defined_syms:
        name = f"CONFIG_{sym.name}"
        if sym.type in (kconfiglib.BOOL, kconfiglib.TRISTATE):
            if sym.str_value in ("y", "m"):
                header_lines.append(f"#define {name} 1")
                cmake_lines.append(f"set({name} ON)")
            else:
                cmake_lines.append(f"set({name} OFF)")
        elif sym.type == kconfiglib.STRING:
            header_lines.append(f'#define {name} "{sym.str_value}"')
            cmake_lines.append(f'set({name} "{sym.str_value}")')
        elif sym.type in (kconfiglib.INT, kconfiglib.HEX):
            header_lines.append(f"#define {name} {sym.str_value}")
            cmake_lines.append(f"set({name} {sym.str_value})")

    header_lines += ["", "#endif /* SDKCONFIG_H */"]

    with open(args.header_out, "w", encoding="utf-8") as f:
        f.write("\n".join(header_lines) + "\n")
    with open(args.cmake_out, "w", encoding="utf-8") as f:
        f.write("\n".join(cmake_lines) + "\n")

    print(f"sdkconfig: {args.sdkconfig}")
    print(f"sdkconfig.h: {args.header_out}")
    print(f"sdkconfig.cmake: {args.cmake_out}")


if __name__ == "__main__":
    main()
