#!/bin/bash
#
# Install sports-score-ticker as systemd services on Raspberry Pi.
# Run as: sudo ./deploy/install.sh
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
SKIP_BUILD="${SKIP_BUILD:-0}"

echo "=== Sports Score Ticker Installer ==="
echo "Project directory: $PROJECT_DIR"

# Check we're running as root
if [ "$EUID" -ne 0 ]; then
    echo "Error: Please run as root (sudo ./deploy/install.sh)"
    exit 1
fi

# Detect the actual user; fall back to root if run directly as root (not via sudo)
REAL_USER="${SUDO_USER:-root}"
REAL_HOME=$(eval echo "~$REAL_USER")
echo "Installing for user: $REAL_USER (home: $REAL_HOME)"

# 1. Install Python dependencies
echo ""
echo "--- Installing Python dependencies ---"
pip3 install --break-system-packages -r "$PROJECT_DIR/fetcher/requirements.txt"

# 2. Build the native LED stack unless a prebuilt binary was pushed
if [ "$SKIP_BUILD" = "1" ]; then
    echo ""
    echo "--- Skipping LED build (SKIP_BUILD=1) ---"
    if [ ! -x "$PROJECT_DIR/display/build/led_ticker" ]; then
        echo "Error: display/build/led_ticker is missing or not executable"
        exit 1
    fi
else
    RGB_LIB="$PROJECT_DIR/display/libs/rpi-rgb-led-matrix"
    if [ ! -f "$RGB_LIB/lib/librgbmatrix.a" ]; then
        echo ""
        echo "--- Building rpi-rgb-led-matrix ---"
        make -C "$RGB_LIB/lib" -j$(nproc)
    fi

    echo ""
    echo "--- Building LED ticker ---"
    mkdir -p "$PROJECT_DIR/display/build"
    cd "$PROJECT_DIR/display/build"
    cmake ..
    make -j$(nproc)
    cd "$PROJECT_DIR"
fi

# 3. Create logos directory
mkdir -p "$PROJECT_DIR/logos"
chown "$REAL_USER:$REAL_USER" "$PROJECT_DIR/logos"

# 4. Update service files with actual paths
echo ""
echo "--- Installing systemd services ---"

# Generate service files with correct paths
sed "s|/home/pi/sports-score-ticker|$PROJECT_DIR|g; s|User=pi|User=$REAL_USER|g; s|Group=pi|Group=$REAL_USER|g" \
    "$SCRIPT_DIR/score-fetcher.service" > /etc/systemd/system/score-fetcher.service

sed "s|/home/pi/sports-score-ticker|$PROJECT_DIR|g" \
    "$SCRIPT_DIR/led-ticker.service" > /etc/systemd/system/led-ticker.service

# 5. Enable and start services
systemctl daemon-reload
systemctl enable score-fetcher.service
systemctl enable led-ticker.service

echo ""
echo "--- Starting services ---"
systemctl start score-fetcher.service
sleep 2
systemctl start led-ticker.service

echo ""
echo "=== Installation complete! ==="
echo ""
echo "Service status:"
systemctl status score-fetcher.service --no-pager -l || true
echo ""
systemctl status led-ticker.service --no-pager -l || true
echo ""
echo "Useful commands:"
echo "  sudo systemctl status score-fetcher   # Check fetcher status"
echo "  sudo systemctl status led-ticker       # Check display status"
echo "  sudo journalctl -u score-fetcher -f    # View fetcher logs"
echo "  sudo journalctl -u led-ticker -f       # View display logs"
echo "  sudo systemctl restart score-fetcher   # Restart fetcher"
echo "  sudo systemctl restart led-ticker      # Restart display"
echo "  sudo systemctl stop led-ticker score-fetcher  # Stop everything"
