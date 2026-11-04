# syntax=docker/dockerfile:1.7
#
# Embedded firmware build environment.
# Linux equivalent of the macOS setup: Xcode CLT + Homebrew + CMake + Ninja
# + Arm GNU Toolchain + OpenOCD + Python/PyYAML.
#
# Homebrew and Xcode Command Line Tools have no analogue here — apt is the
# package manager, and build-essential provides gcc/g++/make.

FROM ubuntu:24.04

ARG DEBIAN_FRONTEND=noninteractive

# Bump this when Arm publishes a new release. Check:
#   https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads
ARG ARM_TOOLCHAIN_VERSION=15.2.rel1

# Match these to your host user so bind-mounted build output isn't root-owned.
ARG USERNAME=dev
ARG UID=1000
ARG GID=1000

# ---------------------------------------------------------------------------
# Base toolchain
# ---------------------------------------------------------------------------
#   build-essential  -> host gcc/g++/make (what Xcode CLT gave you as Clang)
#   cmake, ninja-build -> `brew install cmake ninja`
#   python3-yaml     -> PyYAML for the Kconfig codegen script. Ubuntu 24.04
#                       enforces PEP 668, so `pip3 install pyyaml` fails
#                       without --break-system-packages. Use the distro
#                       package instead.
#   openocd          -> JTAG/SWD flashing (see caveat in README re: macOS USB)
#   qemu-system-arm  -> Cortex-M emulation (mps2-an385/an386 etc.)
#   gdb-multiarch    -> debugger; the Arm toolchain's own gdb is also on PATH
RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        make \
        git \
        ca-certificates \
        curl \
        wget \
        xz-utils \
        cmake \
        ninja-build \
        python3 \
        python3-pip \
        python3-venv \
        python3-yaml \
        openocd \
        qemu-system-arm \
        gdb-multiarch \
        libusb-1.0-0 \
        libhidapi-hidraw0 \
        libftdi1-2 \
        srecord \
        device-tree-compiler \
        openssh-client \
        sudo \
        less \
        nano \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir --break-system-packages kconfiglib


# ---------------------------------------------------------------------------
# Arm GNU Toolchain (arm-none-eabi) — official Arm build, not the distro one
# ---------------------------------------------------------------------------
# This is the fix for the Homebrew problem: the official tarball ships newlib,
# so <stdint.h>, <string.h> and the rest of the C library headers are present.
# Host arch is detected so this image builds natively on both Apple Silicon
# (aarch64) and Intel/AMD (x86_64) — no QEMU emulation of the compiler.
RUN set -eux; \
    case "$(uname -m)" in \
        x86_64)  ARM_HOST=x86_64 ;; \
        aarch64) ARM_HOST=aarch64 ;; \
        *) echo "Unsupported host architecture: $(uname -m)" >&2; exit 1 ;; \
    esac; \
    url="https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_TOOLCHAIN_VERSION}/binrel/arm-gnu-toolchain-${ARM_TOOLCHAIN_VERSION}-${ARM_HOST}-arm-none-eabi.tar.xz"; \
    echo "Fetching ${url}"; \
    mkdir -p /opt/arm-none-eabi; \
    curl -fSL --retry 3 --retry-delay 5 "$url" -o /tmp/arm-toolchain.tar.xz; \
    tar -xJf /tmp/arm-toolchain.tar.xz -C /opt/arm-none-eabi --strip-components=1; \
    rm -f /tmp/arm-toolchain.tar.xz

ENV PATH="/opt/arm-none-eabi/bin:${PATH}"
ENV ARM_TOOLCHAIN_ROOT="/opt/arm-none-eabi"

# Fail the build early if newlib is missing — the exact failure mode that made
# the Homebrew formula unusable.
RUN set -eux; \
    printf '#include <stdint.h>\n#include <string.h>\nint main(void){ return (int)(uint8_t)0; }\n' > /tmp/probe.c; \
    arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb --specs=nosys.specs \
        /tmp/probe.c -o /tmp/probe.elf; \
    arm-none-eabi-size /tmp/probe.elf; \
    rm -f /tmp/probe.c /tmp/probe.elf

# ---------------------------------------------------------------------------
# Non-root user
# ---------------------------------------------------------------------------
# Ubuntu 24.04 ships a stock `ubuntu` account already sitting on UID 1000,
# which collides with the usual host UID. Remove it first.
RUN set -eux; \
    if id -u ubuntu >/dev/null 2>&1; then userdel -r ubuntu; fi; \
    if ! getent group "${GID}" >/dev/null; then groupadd -g "${GID}" "${USERNAME}"; fi; \
    useradd -m -u "${UID}" -g "${GID}" -s /bin/bash "${USERNAME}"; \
    echo "${USERNAME} ALL=(ALL) NOPASSWD:ALL" > "/etc/sudoers.d/${USERNAME}"; \
    chmod 0440 "/etc/sudoers.d/${USERNAME}"

USER ${USERNAME}
WORKDIR /workspace

# GitHub host keys, so an SSH clone doesn't stall on a host-verification
# prompt. Credentials are never baked in — mount an agent socket at runtime.
RUN set -eux; \
    mkdir -p "${HOME}/.ssh"; \
    chmod 700 "${HOME}/.ssh"; \
    ssh-keyscan -t rsa,ecdsa,ed25519 github.com >> "${HOME}/.ssh/known_hosts" 2>/dev/null; \
    chmod 600 "${HOME}/.ssh/known_hosts"

CMD ["/bin/bash"]
