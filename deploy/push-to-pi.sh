#!/usr/bin/env bash

set -euo pipefail

if [ $# -lt 1 ]; then
    echo "Usage: $0 <user@host> [remote-project-dir]"
    exit 1
fi

PI_HOST="$1"
PI_PROJECT_DIR="${2:-/home/pi/sports-score-ticker}"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
LOCAL_BUILD_DIR="${BUILD_DIR:-$PROJECT_DIR/display/build-pi}"
LOCAL_BINARY="$LOCAL_BUILD_DIR/led_ticker"

if [ ! -x "$LOCAL_BINARY" ]; then
    echo "Error: missing cross-built binary: $LOCAL_BINARY"
    echo "Run deploy/cross-build-pi.sh first."
    exit 1
fi

echo "==> Preparing remote directories"
ssh "$PI_HOST" "mkdir -p '$PI_PROJECT_DIR/display/build' '$PI_PROJECT_DIR/logos'"

echo "==> Syncing project files"
rsync -az --delete \
    --exclude '.git/' \
    --exclude '__pycache__/' \
    --exclude '.DS_Store' \
    --exclude 'display/build/' \
    --exclude 'display/build-pi/' \
    --exclude 'logos/*.ppm' \
    "$PROJECT_DIR/" "$PI_HOST:$PI_PROJECT_DIR/"

echo "==> Uploading Raspberry Pi binary"
rsync -az "$LOCAL_BINARY" "$PI_HOST:$PI_PROJECT_DIR/display/build/led_ticker"

echo "==> Installing on Raspberry Pi without rebuilding"
ssh "$PI_HOST" "cd '$PI_PROJECT_DIR' && sudo SKIP_BUILD=1 ./deploy/install.sh"

echo "Deploy complete:"
echo "  Host: $PI_HOST"
echo "  Path: $PI_PROJECT_DIR"
