# CLAUDE.md

## Project Overview

Sports Score Ticker — a Raspberry Pi-based LED matrix display system that shows live sports scores with team logos. The system has three main components:

1. **Python Backend** (`src/main.py`) — Flask API that fetches live sports data from the ESPN API
2. **C++ LED Application** (`src/led-app/`) — Renders a scrolling ticker on an RGB LED matrix
3. **React UI** (`test-ui/`) — Web-based configuration interface

## Repository Structure

```
src/
  main.py              # Flask REST API server (sports data provider)
  nba.py               # NBA data fetching from ESPN API
  nfl.py               # NFL data fetching from ESPN API
  utilities.py         # Shared helper functions
  led-app/
    CMakeLists.txt     # CMake build config (C++17)
    src/               # C++ source: display objects, rendering, image handling
    tests/             # C++ tests (currently placeholder)
    data/images/       # Team logo images (PNG/PPM)
    libs/              # rpi-rgb-led-matrix submodule
test-ui/               # React app (Material-UI, Axios)
scripts/               # Image conversion and example code
other/                 # 3D printing files for LED matrix bracket
```

## Build & Run

### C++ LED Application

```bash
cd src/led-app
mkdir -p build && cd build
cmake ..
make
```

**Dependencies:** ImageMagick (Magick++), libcurl, rpi-rgb-led-matrix (git submodule)

### Python Backend

```bash
cd src
python main.py
```

**Dependencies:** Flask, flask-cors, requests, Pillow

### React UI

```bash
cd test-ui
npm install
npm start
```

## Key Technical Details

- **Display:** 320x64 pixels (10 chained 64x32 LED panels) with U-mapper and 180-degree rotation
- **Hardware:** Raspberry Pi with Adafruit HAT
- **C++ Standard:** C++17, built with CMake (Debug mode)
- **JSON parsing:** nlohmann/json (header-only, vendored at `src/led-app/src/json.hpp`)
- **Sports data source:** ESPN v2 Scoreboard API
- **Image formats:** PNG for source images, PPM for LED matrix rendering

## C++ Architecture

The display uses a composable object model:

- `ObjectTypeClass` — abstract interface for displayable objects
- `BaseObjectClass` — base with position, type, value
- `TextObjectClass` — renders text using rgb_matrix fonts
- `ImageObjectClass` — renders images via Magick++
- `MultiObjectClass` — composite container for multiple objects
- `ObjectGroupClass` — represents one ticker item (a game), manages scroll position
- `ObjectGroupManagerClass` — manages all groups with threaded updates
- `DisplayManagerClass` — coordinates rendering to the LED matrix

## Testing

- C++ tests are scaffolded but commented out in CMakeLists.txt (`tests/test_dummy.cpp`)
- React tests available via `npm test` in `test-ui/`
- No automated CI configured

## Code Style

- C++ follows a Google-derived style (see `scripts/object-class-example/.clang-format`): tabs for indentation, no column limit
- Python uses standard conventions
- React uses Create React App defaults
