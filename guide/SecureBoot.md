# Signing GT32U2086 firmware

Install the optional signing dependency in the Python environment CMake uses:

```sh
python -m pip install cryptography
```

From an application directory such as `examples/Printf_Log`:

```sh
cmake --preset GT32U2086 -DNEON_MDF_SECURE_BOOT=ON
cmake --build --preset GT32U2086
```

Signing uses the ECDSA P-256 private key at `neon-mdf/tools/key.pem` by default.
Pass `-DNEON_MDF_SIGNING_KEY=/absolute/path/to/key.pem` to use a different key,
or `-DNEON_MDF_SIGNING_KEY=` to clear a cached override.

The build produces the raw payload, `<app>_image.bin` (unsigned), and
`<app>_image_signed.bin`. With signing enabled, the standard UART flash target
selects the signed image. Reconfigure with `-DNEON_MDF_SECURE_BOOT=OFF` to
return the build and flash workflow to unsigned images.

To sign an existing unsigned internal Flash image from the application directory:

```sh
python ../../neon-mdf/tools/gt32u2086_secure_image.py build/GT32U2086/printf_log_image.bin -o build/GT32U2086/printf_log_image_signed.bin
```

The signing tool verifies its output before replacing the destination.
BootROM v5 uses the fixed signing domain `FU2-IMAGE-V1` and authenticates
internal Flash images against the public key provisioned into NVR. The CMake
flag controls image signing. Provisioning the public key and secure policy is
a separate factory operation. BootROM v5 does not authenticate QSPI XIP images.
