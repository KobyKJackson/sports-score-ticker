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
from datetime import datetime
from pathlib import Path
from zoneinfo import ZoneInfo

import requests

from flask import Flask, Response, jsonify, request, send_from_directory
from flask_cors import CORS

from espn import fetch_bracket, fetch_scoreboard

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
    "hours_back": 12,
    "hours_ahead": 24,
    "show_bracket": False,
    "notify_on_final": True,
    "notify_flash_count": 3,
    "notify_display_seconds": 5,
}

NOTIFICATIONS_FILE = "/tmp/score_notifications.json"
previous_game_states = {}

PROJECT_ROOT = Path(__file__).resolve().parent.parent
CONFIG_PATH = PROJECT_ROOT / "config" / "ticker.json"
WEB_DIR = PROJECT_ROOT / "web"

running = True
latest_scores = {"scoreboards": [], "timestamp": 0}
latest_bracket = {"bracket": None, "timestamp": 0}
scores_lock = threading.Lock()
bracket_lock = threading.Lock()
config_lock = threading.Lock()
current_config = dict(DEFAULT_CONFIG)


def signal_handler(sig, frame):
    global running
    log.info("Shutting down...")
    running = False
    sys.exit(0)


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


def format_scheduled_detail(start_time: str, tz: ZoneInfo) -> str:
    """Format a UTC ISO start_time as 'Mar 12 @ 7:30 PM' in the given timezone."""
    try:
        utc_dt = datetime.fromisoformat(start_time.replace("Z", "+00:00"))
        local_dt = utc_dt.astimezone(tz)
        # strftime with manual zero-stripping for cross-platform compatibility
        month = local_dt.strftime("%b")
        day = str(local_dt.day)
        hour = str(local_dt.hour % 12 or 12)
        minute = local_dt.strftime("%M")
        ampm = local_dt.strftime("%p")
        return f"{month} {day} @ {hour}:{minute} {ampm}"
    except (ValueError, TypeError):
        return ""


def fetch_all_scores(sports: list, hours_back: int, hours_ahead: int, tz_name: str = "UTC") -> list:
    """Fetch scores for all configured sports and return combined list."""
    try:
        tz = ZoneInfo(tz_name)
    except Exception:
        tz = ZoneInfo("UTC")

    all_scoreboards = []
    for sport in sports:
        log.info("Fetching %s scores...", sport.upper())
        board = fetch_scoreboard(sport, hours_back=hours_back, hours_ahead=hours_ahead)
        for game in board.games:
            if game.status == "scheduled" and game.start_time:
                formatted = format_scheduled_detail(game.start_time, tz)
                if formatted:
                    game.detail = formatted
        if board.games:
            board_dict = board.to_dict()
            # Compute bet results for final games so the display can show them
            for game in board_dict["games"]:
                if game.get("status") == "final" and game.get("odds"):
                    game["bet_results"] = compute_bet_results(game, game["odds"])
            all_scoreboards.append(board_dict)
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
        os.chmod(tmp_path, 0o644)
        os.replace(tmp_path, data_file)
        log.info("Wrote scores to %s (%d bytes)", data_file, os.path.getsize(data_file))
    except OSError as e:
        log.error("Failed to write scores: %s", e)
        try:
            os.unlink(tmp_path)
        except OSError:
            pass


def compute_bet_results(game: dict, opening_odds) -> dict:
    """Compute bet results from a final game and its opening odds."""
    if not opening_odds:
        return None

    home_score = game.get("home_team", {}).get("score")
    away_score = game.get("away_team", {}).get("score")
    if home_score is None or away_score is None:
        return None

    home_abbr = game.get("home_team", {}).get("abbreviation", "")
    away_abbr = game.get("away_team", {}).get("abbreviation", "")
    results = {}

    # Spread: parse "LAL -3.5" → fav_abbr, spread_num
    spread_str = opening_odds.get("spread", "")
    if spread_str:
        parts = spread_str.rsplit(" ", 1)
        if len(parts) == 2:
            fav_abbr = parts[0]
            try:
                spread_num = float(parts[1])
            except ValueError:
                spread_num = None

            if spread_num is not None:
                # Determine the underdog
                dog_abbr = away_abbr if fav_abbr == home_abbr else home_abbr
                dog_spread = f"+{abs(spread_num):g}" if spread_num != 0 else "0"

                if fav_abbr == home_abbr:
                    margin = home_score - away_score
                elif fav_abbr == away_abbr:
                    margin = away_score - home_score
                else:
                    margin = None

                if margin is not None:
                    # spread_num is negative for favorites (-3.5)
                    # favorite covers if margin + spread_num > 0
                    diff = margin + spread_num
                    if diff > 0:
                        results["spread_result"] = "covered"
                        results["spread_text"] = spread_str  # fav covered
                    elif diff < 0:
                        results["spread_result"] = "not_covered"
                        results["spread_text"] = f"{dog_abbr} {dog_spread}"  # dog covered
                    else:
                        results["spread_result"] = "push"
                        results["spread_text"] = spread_str

    # Over/Under: parse "O/U 215.5"
    ou_str = opening_odds.get("over_under", "")
    if ou_str:
        try:
            ou_num = float(ou_str.replace("O/U ", "").strip())
            total = home_score + away_score
            if total > ou_num:
                results["ou_result"] = "OVER"
            elif total < ou_num:
                results["ou_result"] = "UNDER"
            else:
                results["ou_result"] = "PUSH"
            results["ou_line"] = ou_num
        except ValueError:
            pass

    # Moneyline: stored as "away_ml/home_ml" e.g. "-150/+130"
    ml_str = opening_odds.get("moneyline", "")
    if ml_str and "/" in ml_str:
        ml_parts = ml_str.split("/", 1)
        if len(ml_parts) == 2:
            away_ml, home_ml = ml_parts[0].strip(), ml_parts[1].strip()
            if home_score > away_score:
                results["winner_ml"] = home_ml
                results["winner_abbr"] = home_abbr
            elif away_score > home_score:
                results["winner_ml"] = away_ml
                results["winner_abbr"] = away_abbr

    return results if results else None


def detect_and_write_finals(scoreboards: list):
    """Detect games that just went final and write notifications."""
    global previous_game_states

    with config_lock:
        if not current_config.get("notify_on_final", True):
            return

    new_notifications = []
    current_states = {}

    for board in scoreboards:
        sport = board.get("sport", "")
        for game in board.get("games", []):
            game_id = game.get("game_id", "")
            status = game.get("status", "")
            if not game_id:
                continue

            prev = previous_game_states.get(game_id)

            # Preserve opening odds from when we first saw the game
            if prev is None:
                current_states[game_id] = {
                    "status": status,
                    "odds": game.get("odds"),
                }
            else:
                current_states[game_id] = {
                    "status": status,
                    "odds": prev.get("odds"),  # keep opening odds
                }

            if status == "final" and prev is not None and prev.get("status") != "final":
                opening_odds = prev.get("odds")
                bet_results = compute_bet_results(game, opening_odds)
                notification_game = dict(game)
                notification_game["odds"] = None  # clear odds; bet_results replaces them
                new_notifications.append({
                    "type": "final",
                    "game": notification_game,
                    "bet_results": bet_results,
                    "timestamp": time.time(),
                })

    previous_game_states = current_states

    if not new_notifications:
        return

    # Read existing notifications, append new ones, write atomically
    existing = []
    try:
        with open(NOTIFICATIONS_FILE) as f:
            data = json.load(f)
            existing = data.get("notifications", [])
    except (FileNotFoundError, json.JSONDecodeError):
        pass

    existing.extend(new_notifications)
    output = {"notifications": existing}

    try:
        fd, tmp_path = tempfile.mkstemp(dir="/tmp", suffix=".json")
        with os.fdopen(fd, "w") as f:
            json.dump(output, f, separators=(",", ":"))
        os.chmod(tmp_path, 0o644)
        os.replace(tmp_path, NOTIFICATIONS_FILE)
        log.info("Wrote %d new notification(s)", len(new_notifications))
    except OSError as e:
        log.error("Failed to write notifications: %s", e)
        try:
            os.unlink(tmp_path)
        except OSError:
            pass


def fetch_and_write_bracket(data_file: str):
    """Fetch bracket data and write it to a bracket JSON file."""
    global latest_bracket

    bracket_file = data_file.replace("scores.json", "bracket.json")
    try:
        bracket = fetch_bracket()
        bracket_dict = bracket.to_dict()
        output = {"bracket": bracket_dict, "timestamp": time.time()}

        with bracket_lock:
            latest_bracket = output

        dir_name = os.path.dirname(bracket_file) or "/tmp"
        fd, tmp_path = tempfile.mkstemp(dir=dir_name, suffix=".json")
        with os.fdopen(fd, "w") as f:
            json.dump(output, f, separators=(",", ":"))
        os.chmod(tmp_path, 0o644)
        os.replace(tmp_path, bracket_file)
        log.info("Wrote bracket to %s (%d matchups)",
                 bracket_file, len(bracket.matchups))
    except Exception as e:
        log.error("Failed to fetch/write bracket: %s", e, exc_info=True)


def fetcher_loop():
    """Background thread that polls ESPN APIs."""
    global current_config

    while running:
        with config_lock:
            sports = list(current_config["sports"])
            interval = current_config["update_interval_seconds"]
            data_file = current_config["data_file"]
            hours_back = current_config.get("hours_back", DEFAULT_CONFIG["hours_back"])
            hours_ahead = current_config.get("hours_ahead", DEFAULT_CONFIG["hours_ahead"])
            timezone = current_config.get("timezone", DEFAULT_CONFIG["timezone"])
            show_bracket = current_config.get("show_bracket", False)

        try:
            scores = fetch_all_scores(sports, hours_back, hours_ahead, timezone)
            write_scores(scores, data_file)
            detect_and_write_finals(scores)
            if show_bracket:
                fetch_and_write_bracket(data_file)
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


@app.route("/api/logo")
def proxy_logo():
    """Proxy ESPN team logo images to avoid browser CORS restrictions."""
    url = request.args.get("url", "")
    if not url or not url.startswith("https://a.espncdn.com/"):
        return "", 404
    try:
        resp = requests.get(url, timeout=5)
        resp.raise_for_status()
        return Response(
            resp.content,
            content_type=resp.headers.get("content-type", "image/png"),
            headers={"Cache-Control": "public, max-age=86400"},
        )
    except requests.RequestException:
        return "", 502


@app.route("/api/scores")
def api_scores():
    """Return current score data."""
    with scores_lock:
        return jsonify(latest_scores)


@app.route("/api/bracket")
def api_bracket():
    """Return current bracket data."""
    with bracket_lock:
        return jsonify(latest_bracket)


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
        ("hours_back", 0, 168),
        ("hours_ahead", 0, 168),
        ("notify_flash_count", 1, 10),
        ("notify_display_seconds", 1, 30),
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


@app.route("/api/test-notification", methods=["POST"])
def api_test_notification():
    """Write a test notification for a random game."""
    import random

    with scores_lock:
        all_games = []
        for board in latest_scores.get("scoreboards", []):
            for game in board.get("games", []):
                game["sport"] = board.get("sport", "")
                all_games.append(game)

    if not all_games:
        return jsonify({"error": "No games available"}), 404

    # Prefer a final game, otherwise pick any and fake it
    final_games = [g for g in all_games if g.get("status") == "final"]
    if final_games:
        game = random.choice(final_games)
    else:
        game = dict(random.choice(all_games))
        game["status"] = "final"

    # Compute bet results from current odds (best we can do for test)
    bet_results = compute_bet_results(game, game.get("odds"))
    notification_game = dict(game)
    notification_game["odds"] = None  # clear odds; bet_results replaces them

    notification = {
        "type": "final",
        "game": notification_game,
        "bet_results": bet_results,
        "timestamp": time.time(),
    }

    # Read existing, append, write atomically
    existing = []
    try:
        with open(NOTIFICATIONS_FILE) as f:
            data = json.load(f)
            existing = data.get("notifications", [])
    except (FileNotFoundError, json.JSONDecodeError):
        pass

    existing.append(notification)
    output = {"notifications": existing}

    try:
        fd, tmp_path = tempfile.mkstemp(dir="/tmp", suffix=".json")
        with os.fdopen(fd, "w") as f:
            json.dump(output, f, separators=(",", ":"))
        os.chmod(tmp_path, 0o644)
        os.replace(tmp_path, NOTIFICATIONS_FILE)
    except OSError as e:
        return jsonify({"error": str(e)}), 500

    return jsonify(notification)


@app.route("/api/notifications")
def api_notifications():
    """Return pending notifications."""
    try:
        with open(NOTIFICATIONS_FILE) as f:
            return jsonify(json.load(f))
    except (FileNotFoundError, json.JSONDecodeError):
        return jsonify({"notifications": []})


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
