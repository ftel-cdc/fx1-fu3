# License notices

The VP executable and bundled runtime redistribute the following open-source
components. Keep this directory together with `vp/bin/` whenever the SDK is
passed on.

## Third-party components

| Component                  | License      | Shipped as                            | Notice file        |
|----------------------------|--------------|---------------------------------------|--------------------|
| Accellera SystemC 2.3.4    | Apache-2.0   | `vp/bin/VP_FX1_V2.0/libsystemc.so*`   | SYSTEMC.LICENSE, SYSTEMC.NOTICE |
| Bremen riscv-vp (RV32 ISS) | MIT          | statically linked into the VP binary  | RISCV-VP.LICENSE   |
| Berkeley SoftFloat-3       | BSD-3-Clause | statically linked into the VP binary  | SOFTFLOAT.LICENSE  |
| SAURIA NPU (upstream design)| Apache-2.0 WITH SHL-2.1 | internal SystemC NPU implementation statically linked into the VP binary | SAURIA.SHL-2.1, SAURIA.PROVENANCE.md |

The SAURIA NPU upstream design is Copyright 2022 Jordi Fornt Mas (Barcelona
Supercomputing Center). The public source tree does not include that model; the
binary distribution contains its compiled internal implementation. See
SAURIA.PROVENANCE.md.

## The VP itself

CDC-VP is Copyright 2026 The CDC-VP Authors, licensed under the Apache License,
Version 2.0. See CDC-VP.LICENSE and CDC-VP.NOTICE.
