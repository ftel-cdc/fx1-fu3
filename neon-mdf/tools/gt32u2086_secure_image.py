#!/usr/bin/env python3
"""Sign a packaged GT32U2086 internal Flash image with a P-256 private key."""

import argparse
import os
from pathlib import Path
import tempfile

import gt32u2086_image as image


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path, help="unsigned BootROM image")
    parser.add_argument("-o", "--output", type=Path, required=True)
    parser.add_argument("--key", type=Path, default=Path(__file__).resolve().with_name("key.pem"),
                        help="P-256 private key (default: SDK tools/key.pem)")
    args = parser.parse_args()

    try:
        from cryptography.hazmat.primitives.asymmetric import ec

        if any(args.output.resolve() == path.resolve() or
               (args.output.exists() and args.output.samefile(path))
               for path in (args.input, args.key)):
            raise ValueError("output must differ from the input image and key")
        source = args.input.read_bytes()
        fields, problems = image.describe(source)
        if problems:
            raise ValueError("; ".join(problems))
        if fields["flash_addr"] != image.FLASH_IMAGE_BASE:
            raise ValueError("BootROM v5 does not authenticate XIP images")
        if fields["signature_size"] or fields["crc32_bypass"]:
            raise ValueError("expected an unsigned image with payload CRC enabled")

        key = image.load_private_key(args.key)
        if not isinstance(key, ec.EllipticCurvePrivateKey) or not isinstance(key.curve, ec.SECP256R1):
            raise ValueError("signing requires an ECDSA P-256 private key")
        signed = image.build_image(source[image.HEADER_SIZE:], fields["flash_addr"],
                                   fields["app_version"], key=key)
        _, problems = image.describe(signed, key.public_key())
        if problems:
            raise ValueError("; ".join(problems))
        with tempfile.TemporaryDirectory(dir=args.output.parent) as directory:
            temporary = Path(directory) / "signed.bin"
            temporary.write_bytes(signed)
            os.replace(temporary, args.output)
        print(f"{args.output}: ECDSA P-256 signature verified")
    except ImportError:
        parser.exit(1, "ERROR: signing requires cryptography in the build Python environment\n")
    except (OSError, ValueError, TypeError) as error:
        parser.exit(1, f"ERROR: {error}\n")


if __name__ == "__main__":
    main()
