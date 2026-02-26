"""Flask API server for the Sports Score Ticker.

Fetches live sports data from ESPN via provider classes and serves it
as JSON for both the React UI and the C++ LED application.
"""

import json
import logging
import time
import threading
from collections import OrderedDict
from threading import Lock

from flask import Flask, jsonify
from flask_cors import CORS, cross_origin

from sports import ALL_PROVIDERS

# Configuration
API_PORT = 5001
API_HOST = "0.0.0.0"
UPDATE_INTERVAL_SECONDS = 1
JSON_OUTPUT_PATH = "led-app/src/example.json"

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
logger = logging.getLogger(__name__)

app = Flask(__name__)
CORS(app)
app.config["CORS_HEADERS"] = "Content-Type"


class ThreadSafeData:
    def __init__(self):
        self.data: OrderedDict = OrderedDict()
        self.lock = Lock()

    def update(self, game_id: str, game_data: dict) -> None:
        with self.lock:
            self.data[game_id] = game_data

    def get_all(self) -> dict:
        with self.lock:
            return dict(self.data)


game_data = ThreadSafeData()

# Instantiate all providers
providers = [ProviderClass() for ProviderClass in ALL_PROVIDERS]


def update_game_info() -> None:
    """Background loop that fetches and formats game data from all providers."""
    while True:
        try:
            for provider in providers:
                for game in provider.get_games():
                    display = provider.format_display_data(game)
                    game_data.update(display["id"], display)

            # Write JSON for the C++ LED app
            _write_json_for_led_app()
        except Exception:
            logger.exception("Error updating game data")

        time.sleep(UPDATE_INTERVAL_SECONDS)


def _write_json_for_led_app() -> None:
    """Write current game data to JSON file consumed by the C++ app."""
    try:
        all_data = game_data.get_all()
        output = {"games": list(all_data.values())}
        with open(JSON_OUTPUT_PATH, "w") as f:
            json.dump(output, f, default=str)
    except Exception:
        logger.exception("Failed to write JSON for LED app")


def activate_game_updates() -> None:
    """Start the background thread for game updates."""
    thread = threading.Thread(target=update_game_info, daemon=True)
    thread.start()


@app.route("/api/data", methods=["GET"])
@cross_origin()
def get_data():
    return jsonify(list(game_data.get_all().values()))


if __name__ == "__main__":
    activate_game_updates()
    app.run(debug=False, port=API_PORT, host=API_HOST)
