# Repository Guidelines

## Project Structure & Module Organization
`fetcher/` contains the Python service that polls ESPN, writes `/tmp/scores.json`, and serves the Flask API plus the web UI. `display/` contains the C++17 LED renderer; active code lives in `display/src/`, build files in `display/CMakeLists.txt`, and the vendored `rpi-rgb-led-matrix` submodule in `display/libs/`. `web/` holds the static frontend (`index.html`, `static/ticker.js`, `static/style.css`). Runtime config lives in `config/ticker.json`, deployment assets in `deploy/`, and setup/reference docs in `docs/`. Generated team logos belong in `logos/`.

## Build, Test, and Development Commands
Install Python dependencies with `pip3 install -r fetcher/requirements.txt`.
Build the LED library with `cd display/libs/rpi-rgb-led-matrix && make -j$(nproc)`.
Build the display app with `cd display && mkdir -p build && cd build && cmake .. && make -j$(nproc)`.
Run the fetcher locally with `python3 fetcher/score_fetcher.py`.
Run the LED app on Raspberry Pi hardware with `sudo ./display/build/led_ticker`.
Install the production services with `sudo ./deploy/install.sh`.

## Coding Style & Naming Conventions
Follow the existing style in each area: 4-space indentation in Python, C++, HTML, CSS, and JavaScript. Python uses `snake_case` for functions and variables, plus short module-level docstrings. C++ uses `snake_case` for files/functions and `PascalCase` for types such as `LogoCache` and `ScoreData`. Keep config keys lowercase with underscores, matching `config/ticker.json`. There is no formatter config checked in, so keep changes stylistically consistent with surrounding code and prefer small, readable functions.

## Testing Guidelines
There is currently no automated test suite under `tests/` or `ctest`. For changes, verify the affected path directly: start `python3 fetcher/score_fetcher.py`, open the web UI at `http://localhost:5001`, and confirm `/api/scores` and `/api/config` respond correctly. For display changes, rebuild `display/` and validate on Pi hardware when possible. If you add tests, place Python tests under `tests/` with `test_*.py` names and wire C++ tests into CMake.

## Commit & Pull Request Guidelines
Recent commits use short, imperative summaries such as `Add web frontend with LED panel simulator and config editor` and `Rewrite display app from C to C++17`. Keep commit subjects concise, capitalized, and action-oriented. Pull requests should describe user-visible behavior, note config or hardware impacts, link related issues, and include screenshots for `web/` changes. Call out any manual verification steps, especially when hardware-only behavior could not be exercised locally.
