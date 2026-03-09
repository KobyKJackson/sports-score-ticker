#!/usr/bin/env python3
"""Sports score fetcher - polls ESPN APIs and writes JSON for the LED display."""

import json
import logging
import os
import signal
import sys
import tempfile
import time
from pathlib import Path

from espn import fetch_scoreboard

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
log = logging.getLogger("score_fetcher")

DEFAULT_CONFIG = {
    "sports": ["nba", "nfl", "mlb", "nhl", "ncaaf", "ncaam"],
    "update_interval_seconds": 30,
    "data_file": "/tmp/scores.json",
    "show_betting": True,
    "show_venue": True,
    "timezone": "America/New_York",
}

running = True


def signal_handler(sig, frame):
    global running
    log.info("Shutting down...")
    running = False


def load_config() -> dict:
    """Load configuration from ticker.json, falling back to defaults."""
    config_path = os.environ.get("TICKER_CONFIG")
    if not config_path:
        # Look relative to the project root
        project_root = Path(__file__).resolve().parent.parent
        config_path = project_root / "config" / "ticker.json"

    config = dict(DEFAULT_CONFIG)
    try:
        with open(config_path) as f:
            user_config = json.load(f)
            config.update(user_config)
            log.info("Loaded config from %s", config_path)
    except FileNotFoundError:
        log.warning("Config file not found at %s, using defaults", config_path)
    except json.JSONDecodeError as e:
        log.error("Invalid JSON in config file: %s", e)

    # Environment variable overrides
    if data_file := os.environ.get("TICKER_DATA_FILE"):
        config["data_file"] = data_file

    return config


def fetch_all_scores(sports: list) -> list:
    """Fetch scores for all configured sports and return combined list."""
    all_scoreboards = []
    for sport in sports:
        log.info("Fetching %s scores...", sport.upper())
        board = fetch_scoreboard(sport)
        if board.games:
            all_scoreboards.append(board.to_dict())
            log.info("  Got %d %s games", len(board.games), sport.upper())
        else:
            log.info("  No %s games right now", sport.upper())
    return all_scoreboards


def write_scores(data: list, data_file: str):
    """Atomically write scores JSON to the data file.

    Uses write-to-temp + rename for atomic updates so the C reader
    never sees a partially written file.
    """
    output = {"scoreboards": data, "timestamp": time.time()}

    dir_name = os.path.dirname(data_file) or "/tmp"
    try:
        fd, tmp_path = tempfile.mkstemp(dir=dir_name, suffix=".json")
        with os.fdopen(fd, "w") as f:
            json.dump(output, f, separators=(",", ":"))
        os.replace(tmp_path, data_file)
        log.info("Wrote scores to %s (%d bytes)", data_file, os.path.getsize(data_file))
    except OSError as e:
        log.error("Failed to write scores: %s", e)
        # Clean up temp file on failure
        try:
            os.unlink(tmp_path)
        except OSError:
            pass


def main():
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    config = load_config()
    sports = config["sports"]
    interval = config["update_interval_seconds"]
    data_file = config["data_file"]

    log.info("Sports Score Fetcher starting")
    log.info("  Sports: %s", ", ".join(s.upper() for s in sports))
    log.info("  Update interval: %ds", interval)
    log.info("  Data file: %s", data_file)

    while running:
        try:
            scores = fetch_all_scores(sports)
            write_scores(scores, data_file)
        except Exception as e:
            log.error("Unexpected error in fetch cycle: %s", e, exc_info=True)

        # Sleep in small increments so we can respond to signals
        for _ in range(interval * 10):
            if not running:
                break
            time.sleep(0.1)

    log.info("Score fetcher stopped.")


if __name__ == "__main__":
    main()
