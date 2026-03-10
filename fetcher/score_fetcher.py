#!/usr/bin/env python3
"""Sports score fetcher with web API.

Polls ESPN APIs for live scores, writes JSON for the LED display,
and serves a Flask API for the web frontend (LED simulator + config editor).
"""

import json
import logging
import os
import signal
import sys
import tempfile
import threading
import time
from pathlib import Path

from flask import Flask, jsonify, request, send_from_directory
from flask_cors import CORS

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
    "scroll_speed": 1,
    "brightness": 80,
    "logo_dir": "logos/",
}

PROJECT_ROOT = Path(__file__).resolve().parent.parent
CONFIG_PATH = PROJECT_ROOT / "config" / "ticker.json"
WEB_DIR = PROJECT_ROOT / "web"

running = True
latest_scores = {"scoreboards": [], "timestamp": 0}
scores_lock = threading.Lock()
config_lock = threading.Lock()
current_config = dict(DEFAULT_CONFIG)


def signal_handler(sig, frame):
    global running
    log.info("Shutting down...")
    running = False


def load_config() -> dict:
    """Load configuration from ticker.json, falling back to defaults."""
    config_path = os.environ.get("TICKER_CONFIG", str(CONFIG_PATH))

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

    if data_file := os.environ.get("TICKER_DATA_FILE"):
        config["data_file"] = data_file

    return config


def save_config(config: dict):
    """Save configuration to ticker.json."""
    config_path = os.environ.get("TICKER_CONFIG", str(CONFIG_PATH))
    try:
        Path(config_path).parent.mkdir(parents=True, exist_ok=True)
        with open(config_path, "w") as f:
            json.dump(config, f, indent=2)
            f.write("\n")
        log.info("Saved config to %s", config_path)
    except OSError as e:
        log.error("Failed to save config: %s", e)
        raise


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
    """Atomically write scores JSON to the data file."""
    global latest_scores
    output = {"scoreboards": data, "timestamp": time.time()}

    with scores_lock:
        latest_scores = output

    dir_name = os.path.dirname(data_file) or "/tmp"
    try:
        fd, tmp_path = tempfile.mkstemp(dir=dir_name, suffix=".json")
        with os.fdopen(fd, "w") as f:
            json.dump(output, f, separators=(",", ":"))
        os.replace(tmp_path, data_file)
        log.info("Wrote scores to %s (%d bytes)", data_file, os.path.getsize(data_file))
    except OSError as e:
        log.error("Failed to write scores: %s", e)
        try:
            os.unlink(tmp_path)
        except OSError:
            pass


def fetcher_loop():
    """Background thread that polls ESPN APIs."""
    global current_config

    while running:
        with config_lock:
            sports = list(current_config["sports"])
            interval = current_config["update_interval_seconds"]
            data_file = current_config["data_file"]

        try:
            scores = fetch_all_scores(sports)
            write_scores(scores, data_file)
        except Exception as e:
            log.error("Unexpected error in fetch cycle: %s", e, exc_info=True)

        for _ in range(interval * 10):
            if not running:
                break
            time.sleep(0.1)


# --- Flask API ---

app = Flask(__name__, static_folder=None)
CORS(app)


@app.route("/")
def index():
    """Serve the web frontend."""
    return send_from_directory(str(WEB_DIR), "index.html")


@app.route("/static/<path:filename>")
def static_files(filename):
    """Serve static assets (JS, CSS)."""
    return send_from_directory(str(WEB_DIR / "static"), filename)


@app.route("/api/scores")
def api_scores():
    """Return current score data."""
    with scores_lock:
        return jsonify(latest_scores)


@app.route("/api/config", methods=["GET"])
def api_get_config():
    """Return current configuration."""
    with config_lock:
        return jsonify(current_config)


@app.route("/api/config", methods=["PUT"])
def api_put_config():
    """Update configuration. Expects JSON body with config fields to update."""
    global current_config

    data = request.get_json()
    if not data:
        return jsonify({"error": "Invalid JSON body"}), 400

    # Validate fields
    valid_keys = set(DEFAULT_CONFIG.keys())
    invalid = set(data.keys()) - valid_keys
    if invalid:
        return jsonify({"error": f"Unknown config keys: {', '.join(invalid)}"}), 400

    # Validate sports
    if "sports" in data:
        valid_sports = {"nba", "nfl", "mlb", "nhl", "ncaaf", "ncaam"}
        if not isinstance(data["sports"], list):
            return jsonify({"error": "sports must be an array"}), 400
        bad = set(data["sports"]) - valid_sports
        if bad:
            return jsonify({"error": f"Invalid sports: {', '.join(bad)}"}), 400

    # Validate numeric fields
    for field, lo, hi in [
        ("scroll_speed", 1, 10),
        ("update_interval_seconds", 5, 300),
        ("brightness", 1, 100),
    ]:
        if field in data:
            val = data[field]
            if not isinstance(val, int) or val < lo or val > hi:
                return jsonify({"error": f"{field} must be integer {lo}-{hi}"}), 400

    # Validate timezone
    if "timezone" in data:
        if not isinstance(data["timezone"], str) or "/" not in data["timezone"]:
            return jsonify({"error": "timezone must be IANA format (e.g. America/New_York)"}), 400

    with config_lock:
        current_config.update(data)
        try:
            save_config(current_config)
        except OSError as e:
            return jsonify({"error": str(e)}), 500
        return jsonify(current_config)


@app.route("/api/config/reset", methods=["POST"])
def api_reset_config():
    """Reset configuration to defaults."""
    global current_config
    with config_lock:
        current_config = dict(DEFAULT_CONFIG)
        try:
            save_config(current_config)
        except OSError as e:
            return jsonify({"error": str(e)}), 500
        return jsonify(current_config)


def main():
    global current_config

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    current_config = load_config()

    log.info("Sports Score Fetcher starting")
    log.info("  Sports: %s", ", ".join(s.upper() for s in current_config["sports"]))
    log.info("  Update interval: %ds", current_config["update_interval_seconds"])
    log.info("  Data file: %s", current_config["data_file"])

    # Start fetcher in background thread
    fetcher_thread = threading.Thread(target=fetcher_loop, daemon=True)
    fetcher_thread.start()

    # Start Flask API server
    port = int(os.environ.get("TICKER_WEB_PORT", "5001"))
    log.info("Web UI at http://0.0.0.0:%d", port)
    app.run(host="0.0.0.0", port=port, debug=False, use_reloader=False)


if __name__ == "__main__":
    main()
