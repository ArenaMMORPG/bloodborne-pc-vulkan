#!/usr/bin/env bash
# Packages ONLY the open-source prebuilt Linux runtime binaries (out/bb-probe, out/bb-gpu-capabilities,
# out/gpu/libbbgpu.so, out/gpu/libbbnet20.so, out/gpu/libbbnet.so) into a clean archive for GitHub Releases.
# Contains ZERO bytes of proprietary Sony/FromSoftware game assets or eboot.bin.
set -euo pipefail
cd -- "$(dirname -- "$0")/.."

mkdir -p dist
ARCHIVE="dist/bloodborne-pc-vulkan-linux-x86_64-prebuilt.tar.gz"

tar -czvf "$ARCHIVE" \
    play.sh \
    launcher.sh \
    run.sh \
    out/bb-probe \
    out/bb-gpu-capabilities \
    out/gpu/libbbgpu.so \
    out/gpu/libbbnet20.so \
    out/gpu/libbbnet.so

echo "=== Created GitHub Release Archive: $ARCHIVE ($(du -h "$ARCHIVE" | cut -f1)) ==="
