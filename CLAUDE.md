# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What This Project Does

A real-time sports score ticker for Raspberry Pi + RGB LED matrix. Three components work together:
- **Python fetcher** polls ESPN APIs and writes scores to `/tmp/scores.json`
- **C++ display app** reads that file and renders a scrolling ticker on a 320x64 LED matrix
- **Web frontend** provides a browser-based LED simulator and config editor

## Build Commands

### Python Fetcher
```bash
pip3 install -r fetcher/requirements.txt
python3 fetcher/score_fetcher.py
```

### C++ Display App
```bash
# First time: build the rpi-rgb-led-matrix submodule
cd display/libs/rpi-rgb-led-matrix && make -j$(nproc)

# Build the display app
cd display && mkdir -p build && cd build && cmake .. && make -j$(nproc)

# Run (requires root for GPIO)
# Use these flags on Pi 4 to minimize flicker. isolcpus=3 must be set in /boot/firmware/cmdline.txt.
taskset -c 3 ./display/build/led_ticker \
  --led-no-hardware-pulse \
  --led-slowdown-gpio=4 \
  --led-pwm-lsb-nanoseconds=200 \
  --led-pwm-bits=9
```

### Production Install
```bash
sudo ./deploy/install.sh   # builds everything and installs systemd services
```

### View Logs
```bash
sudo journalctl -u score-fetcher -f
sudo journalctl -u led-ticker -f
```

## Architecture

### IPC: Shared JSON File
The Python fetcher and C++ display app communicate exclusively via `/tmp/scores.json`. The fetcher writes atomically (temp file + rename); the display app reads every 5 seconds. No sockets or queues.

### Configuration
`config/ticker.json` is read at startup by both components. The Flask API (`/api/config` PUT) modifies and persists it at runtime. Environment variable overrides: `TICKER_CONFIG`, `TICKER_DATA_FILE`, `TICKER_BRIGHTNESS`, `TICKER_WEB_PORT`.

### Python Fetcher (`fetcher/`)
- `score_fetcher.py` — entry point; runs background fetch thread + Flask server on port 5001
- `espn.py` — ESPN API client supporting NBA, NFL, MLB, NHL, NCAAF, NCAAM
- `models.py` — dataclasses: `Team`, `Game`, `Odds`, `ScoreBoard`
- Flask API serves the web frontend from `web/` and exposes REST endpoints for scores and config

### C++ Display App (`display/src/`)
- `main.cpp` — matrix initialization, main render loop at 40 FPS
- `ticker.hpp/cpp` — scrolling ticker renderer
- `score_reader.hpp/cpp` — JSON parser for scores.json
- `logo_cache.hpp/cpp` — PPM logo loader/cache
- Requires `display/libs/rpi-rgb-led-matrix` (git submodule) to be built first
- CMake requires C++17; links against `librgbmatrix.a`, libm, libpthread, librt

### Web Frontend (`web/`)
- Single-page app: `index.html` + `static/ticker.js` + `static/style.css`
- Pixel-accurate 320x64 canvas simulator with zoom (2x–5x), FPS counter, grid overlay
- Config editor talks to the Flask API via XHR

### Team Logos
PPM files in `logos/` downloaded via `fetcher/download_logos.py`. Used by the C++ app for 28x28 pixel team logo rendering.

## Hardware Target
Raspberry Pi 4, Electrodragon RGB driver board, 10× P4 LED panels arranged as 2 rows of 5 (320×64 total). See `docs/hardware-setup.md` for wiring.
