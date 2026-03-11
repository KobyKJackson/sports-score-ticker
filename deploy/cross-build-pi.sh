#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
RGB_DIR="$PROJECT_DIR/display/libs/rpi-rgb-led-matrix"
BUILD_DIR="${BUILD_DIR:-$PROJECT_DIR/display/build-pi}"
TOOLCHAIN_FILE="${TOOLCHAIN_FILE:-$PROJECT_DIR/toolchains/raspberry-pi.cmake}"
PI_TOOLCHAIN_PREFIX="${PI_TOOLCHAIN_PREFIX:-aarch64-linux-gnu}"
PI_BUILD_TYPE="${PI_BUILD_TYPE:-Release}"

PI_CC="${PI_CC:-${PI_TOOLCHAIN_PREFIX}-gcc}"
PI_CXX="${PI_CXX:-${PI_TOOLCHAIN_PREFIX}-g++}"
PI_AR="${PI_AR:-${PI_TOOLCHAIN_PREFIX}-gcc-ar}"

if ! command -v "$PI_CC" >/dev/null 2>&1; then
    echo "Error: cross compiler not found: $PI_CC"
    exit 1
fi

if ! command -v "$PI_CXX" >/dev/null 2>&1; then
    echo "Error: cross compiler not found: $PI_CXX"
    exit 1
fi

CC_CMD="$PI_CC"
CXX_CMD="$PI_CXX"
if [ -n "${PI_SYSROOT:-}" ]; then
    CC_CMD="$CC_CMD --sysroot=$PI_SYSROOT"
    CXX_CMD="$CXX_CMD --sysroot=$PI_SYSROOT"
fi

echo "==> Building rpi-rgb-led-matrix for Raspberry Pi"
make -C "$RGB_DIR/lib" clean
make -C "$RGB_DIR/lib" -j"$(getconf _NPROCESSORS_ONLN)" \
    CC="$CC_CMD" \
    CXX="$CXX_CMD" \
    AR="$PI_AR"

echo "==> Configuring cross-build in $BUILD_DIR"
cmake -S "$PROJECT_DIR/display" -B "$BUILD_DIR" \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE" \
    -DCMAKE_BUILD_TYPE="$PI_BUILD_TYPE" \
    -DRGB_INCLUDE_DIR="$RGB_DIR/include" \
    -DRGB_STATIC_LIBRARY="$RGB_DIR/lib/librgbmatrix.a"

echo "==> Building led_ticker"
cmake --build "$BUILD_DIR" -j"$(getconf _NPROCESSORS_ONLN)"

echo "Cross-build complete:"
echo "  $BUILD_DIR/led_ticker"
