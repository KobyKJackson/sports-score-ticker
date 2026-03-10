# Sports Score Ticker

A real-time sports score ticker built for Raspberry Pi with RGB LED matrix panels. Fetches live scores from ESPN and displays them as a scrolling ticker with team logos, scores, betting lines, game time, and venue.

## Hardware

| Component | Specification |
|-----------|--------------|
| Computer | Raspberry Pi 4 (recommended) |
| Driver Board | [Electrodragon RGB Matrix Panel Drive Board v2](https://www.electrodragon.com/product/rgb-matrix-panel-drive-board-for-raspberry-pi-v2/) |
| LED Panels | 10x P4 Indoor SMD RGB (64x32 pixels, 256x128mm, 1/16 scan) |
| Display Resolution | 320x64 pixels (2 rows of 5 panels) |

### Panel Layout

```
Chain 1 ──► [P1][P2][P3][P4][P5]   ← Row 0 (top)
Chain 2 ──► [P6][P7][P8][P9][P10]  ← Row 1 (bottom)
            ◄── 320 pixels wide ──►
            64 pixels tall (32 per row)
```

Two parallel chains of 5 panels each, connected to the Electrodragon driver board. Chain 1 on HUB75 output 1, Chain 2 on HUB75 output 2.

## Sports Supported

- **NBA** - National Basketball Association
- **NFL** - National Football League
- **MLB** - Major League Baseball
- **NHL** - National Hockey League
- **College Football** (NCAAF/FBS)
- **College Basketball** (NCAAM)

## Architecture

```
┌─────────────────┐       /tmp/scores.json       ┌──────────────────┐
│  Python Fetcher  │ ──── writes JSON ──────────► │  C Display App   │
│  (score_fetcher) │                              │  (led_ticker)    │
│                  │                              │                  │
│  ESPN APIs ─►    │                              │  ─► RGB Matrix   │
│  Parse scores    │                              │  Scrolling ticker│
│  Betting lines   │                              │  Team logos      │
└─────────────────┘                               └──────────────────┘
     systemd service                                  systemd service
     runs as pi user                                  runs as root
```

1. **Python Fetcher** (`fetcher/`) - Polls ESPN APIs every 30s, writes structured JSON to `/tmp/scores.json`
2. **C Display App** (`display/`) - Reads JSON, renders scrolling ticker on LED matrix using `rpi-rgb-led-matrix`

## Quick Start

### 1. Clone and initialize

```bash
git clone --recursive https://github.com/KobyKJackson/sports-score-ticker.git
cd sports-score-ticker
```

### 2. Build the LED matrix library

```bash
cd display/libs/rpi-rgb-led-matrix
make -j$(nproc)
cd ../../..
```

### 3. Build the display application

```bash
cd display
mkdir build && cd build
cmake ..
make -j$(nproc)
cd ../..
```

### 4. Install Python dependencies

```bash
pip3 install -r fetcher/requirements.txt
```

### 5. Run

```bash
# Terminal 1: Start the score fetcher
python3 fetcher/score_fetcher.py

# Terminal 2: Start the LED display (requires root for GPIO)
sudo ./display/build/led_ticker
```

### 6. (Optional) Install as services

```bash
sudo ./deploy/install.sh
```

## Configuration

Edit `config/ticker.json` to customize:

```json
{
  "sports": ["nba", "nfl", "mlb", "nhl", "ncaaf", "ncaam"],
  "scroll_speed": 1,
  "update_interval_seconds": 30,
  "brightness": 80,
  "timezone": "America/New_York"
}
```

See [docs/configuration.md](docs/configuration.md) for full configuration reference.

## Project Structure

```
sports-score-ticker/
├── fetcher/                  # Python score fetcher
│   ├── score_fetcher.py      # Main fetcher entry point
│   ├── espn.py               # ESPN API client
│   ├── models.py             # Data models
│   └── requirements.txt      # Python dependencies
├── display/                  # C++ LED display application
│   ├── src/
│   │   ├── main.cpp          # Entry point (uses rpi-rgb-led-matrix C++ API)
│   │   ├── ticker.hpp/cpp    # Scrolling ticker renderer class
│   │   ├── score_reader.hpp/cpp # JSON parser with std::string/vector
│   │   └── logo_cache.hpp/cpp   # Team logo PPM cache class
│   ├── libs/
│   │   └── rpi-rgb-led-matrix # Git submodule
│   └── CMakeLists.txt        # CMake build (C++17)
├── config/
│   └── ticker.json           # Runtime configuration
├── deploy/
│   ├── install.sh            # Service installer
│   ├── score-fetcher.service  # systemd unit
│   └── led-ticker.service    # systemd unit
├── logos/                    # Team logo PPM files (auto-downloaded)
└── docs/
    ├── hardware-setup.md     # Wiring and panel assembly guide
    └── configuration.md      # Configuration reference
```

## Hardware Setup

See [docs/hardware-setup.md](docs/hardware-setup.md) for detailed wiring instructions and panel assembly.

## License

MIT
