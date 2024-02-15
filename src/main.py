import time
import requests
import json
import threading
from threading import Lock
from collections import OrderedDict
from flask import Flask, jsonify
from flask_cors import CORS, cross_origin  # Import CORS


from nba import *
from nfl import *

app = Flask(__name__)
CORS(app)
app.config["CORS_HEADERS"] = "Content-Type"


def printAPI():
    today = datetime.now()
    start_date = today - timedelta(hours=12)
    end_date = today + timedelta(hours=24)
    start_str = start_date.strftime("%Y%m%d")
    end_str = end_date.strftime("%Y%m%d")

    url = f"http://site.api.espn.com/apis/site/v2/sports/basketball/nba/scoreboard?dates={start_str}-{end_str}"

    response = requests.get(url)
    if response.status_code != 200:
        print("Failed to retrieve data")

    data = response.json()

    # Save the entire API response in a separate file
    with open("nbaOut.json", "w") as api_file:
        json.dump(data, api_file, indent=4)

    today = datetime.now()
    start_date = today - timedelta(hours=1)
    end_date = today + timedelta(hours=200)
    start_str = start_date.strftime("%Y%m%d")
    end_str = end_date.strftime("%Y%m%d")

    url = f"http://site.api.espn.com/apis/site/v2/sports/football/nfl/scoreboard?dates={start_str}-{end_str}"

    response = requests.get(url)
    if response.status_code != 200:
        print("Failed to retrieve data")

    data = response.json()

    # Save the entire API response in a separate file
    with open("nflOut.json", "w") as api_file:
        json.dump(data, api_file, indent=4)

    exit()


class ThreadSafeData:
    def __init__(self):
        self.data = OrderedDict()
        self.lock = Lock()

    def updateData(self, id, data):
        with self.lock:
            self.data[id] = data

    def getData(self):
        return self.data


data = ThreadSafeData()


def update_game_info():
    while True:
        for game in get_nba_games():
            gameData = {
                "id": game["id"],
                "data": [
                    {
                        "type": "image",
                        "location": [1, 2, 3, 4],
                        "data": game["away"]["logo"],
                    },
                    {"type": "text", "location": [2, 3], "data": "@"},
                    {
                        "type": "image",
                        "location": [1, 2, 3, 4],
                        "data": game["home"]["logo"],
                    },
                    {
                        "type": "multi",
                        "data": [
                            {
                                "type": "text",
                                "location": [1, 2],
                                "data": game["away"]["name"],
                                "color": game["away"]["color"],
                                "altColor": game["away"]["altColor"],
                            },
                            {
                                "type": "text",
                                "location": [3, 4],
                                "data": game["home"]["name"],
                                "color": game["home"]["color"],
                                "altColor": game["home"]["altColor"],
                            },
                        ],
                    },
                ],
            }
            if game["status"] == "STATUS_IN_PROGRESS":
                period = game["period"]
                if period > 4:
                    period = "OT" + str((period - 4))
                else:
                    period = "Q" + str(period)
                clock = game["clock"]
                gameData["data"].append(
                    {
                        "type": "multi",
                        "data": [
                            {
                                "type": "text",
                                "location": [1, 2],
                                "data": game["away"]["score"],
                            },
                            {
                                "type": "text",
                                "location": [3, 4],
                                "data": game["home"]["score"],
                            },
                        ],
                    }
                )
                gameData["data"].append(
                    {
                        "type": "text",
                        "location": [2, 3],
                        "data": f"{period} {clock}",
                    }
                )
                gameData["data"].append(
                    {
                        "type": "multi",
                        "data": [
                            {
                                "type": "text",
                                "location": [1],
                                "data": "PTS "
                                + str(game["away"]["leaders"]["Pts"]["value"])
                                + " "
                                + game["away"]["leaders"]["Pts"]["name"],
                            },
                            {
                                "type": "text",
                                "location": [2],
                                "data": "REB "
                                + str(game["away"]["leaders"]["Reb"]["value"])
                                + " "
                                + game["away"]["leaders"]["Reb"]["name"],
                            },
                            {
                                "type": "text",
                                "location": [3],
                                "data": "PTS "
                                + str(game["home"]["leaders"]["Pts"]["value"])
                                + " "
                                + game["home"]["leaders"]["Pts"]["name"],
                            },
                            {
                                "type": "text",
                                "location": [4],
                                "data": "REB "
                                + str(game["home"]["leaders"]["Reb"]["value"])
                                + " "
                                + game["home"]["leaders"]["Reb"]["name"],
                            },
                        ],
                    }
                )
                data.updateData(game["id"], gameData)
            elif game["status"] == "STATUS_HALFTIME":
                gameData["data"].append(
                    {
                        "type": "multi",
                        "data": [
                            {
                                "type": "text",
                                "location": [1, 2],
                                "data": game["away"]["score"],
                            },
                            {
                                "type": "text",
                                "location": [3, 4],
                                "data": game["home"]["score"],
                            },
                        ],
                    }
                )
                gameData["data"].append(
                    {
                        "type": "text",
                        "location": [2, 3],
                        "data": "HALF",
                    }
                )
                gameData["data"].append(
                    {
                        "type": "multi",
                        "data": [
                            {
                                "type": "text",
                                "location": [1],
                                "data": "PTS "
                                + str(game["away"]["leaders"]["Pts"]["value"])
                                + " "
                                + game["away"]["leaders"]["Pts"]["name"],
                            },
                            {
                                "type": "text",
                                "location": [2],
                                "data": "REB "
                                + str(game["away"]["leaders"]["Reb"]["value"])
                                + " "
                                + game["away"]["leaders"]["Reb"]["name"],
                            },
                            {
                                "type": "text",
                                "location": [3],
                                "data": "PTS "
                                + str(game["home"]["leaders"]["Pts"]["value"])
                                + " "
                                + game["home"]["leaders"]["Pts"]["name"],
                            },
                            {
                                "type": "text",
                                "location": [4],
                                "data": "REB "
                                + str(game["home"]["leaders"]["Reb"]["value"])
                                + " "
                                + game["home"]["leaders"]["Reb"]["name"],
                            },
                        ],
                    },
                )
                data.updateData(game["id"], gameData)
            elif game["status"] == "STATUS_END_PERIOD":
                gameData["data"].append(
                    {
                        "type": "multi",
                        "data": [
                            {
                                "type": "text",
                                "location": [1, 2],
                                "data": game["away"]["score"],
                            },
                            {
                                "type": "text",
                                "location": [3, 4],
                                "data": game["home"]["score"],
                            },
                        ],
                    }
                )
                gameData["data"].append(
                    {
                        "type": "text",
                        "location": [2, 3],
                        "data": f"END Q{game['period']}",
                    }
                )
                gameData["data"].append(
                    {
                        "type": "multi",
                        "data": [
                            {
                                "type": "text",
                                "location": [1],
                                "data": "PTS "
                                + str(game["away"]["leaders"]["Pts"]["value"])
                                + " "
                                + game["away"]["leaders"]["Pts"]["name"],
                            },
                            {
                                "type": "text",
                                "location": [2],
                                "data": "REB "
                                + str(game["away"]["leaders"]["Reb"]["value"])
                                + " "
                                + game["away"]["leaders"]["Reb"]["name"],
                            },
                            {
                                "type": "text",
                                "location": [3],
                                "data": "PTS "
                                + str(game["home"]["leaders"]["Pts"]["value"])
                                + " "
                                + game["home"]["leaders"]["Pts"]["name"],
                            },
                            {
                                "type": "text",
                                "location": [4],
                                "data": "REB "
                                + str(game["home"]["leaders"]["Reb"]["value"])
                                + " "
                                + game["home"]["leaders"]["Reb"]["name"],
                            },
                        ],
                    },
                )
                data.updateData(game["id"], gameData)
            elif game["status"] == "STATUS_FINAL":
                gameData["data"].append(
                    {
                        "type": "multi",
                        "data": [
                            {
                                "type": "text",
                                "location": [1, 2],
                                "data": game["away"]["score"],
                            },
                            {
                                "type": "text",
                                "location": [3, 4],
                                "data": game["home"]["score"],
                            },
                        ],
                    }
                )
                gameData["data"].append(
                    {
                        "type": "text",
                        "location": [2, 3],
                        "data": f"{game['statusDetail']}",
                    }
                )
                gameData["data"].append(
                    {
                        "type": "multi",
                        "data": [
                            {
                                "type": "text",
                                "location": [1],
                                "data": "PTS "
                                + str(game["away"]["leaders"]["Pts"]["value"])
                                + " "
                                + game["away"]["leaders"]["Pts"]["name"],
                            },
                            {
                                "type": "text",
                                "location": [2],
                                "data": "REB "
                                + str(game["away"]["leaders"]["Reb"]["value"])
                                + " "
                                + game["away"]["leaders"]["Reb"]["name"],
                            },
                            {
                                "type": "text",
                                "location": [3],
                                "data": "PTS "
                                + str(game["home"]["leaders"]["Pts"]["value"])
                                + " "
                                + game["home"]["leaders"]["Pts"]["name"],
                            },
                            {
                                "type": "text",
                                "location": [4],
                                "data": "REB "
                                + str(game["home"]["leaders"]["Reb"]["value"])
                                + " "
                                + game["home"]["leaders"]["Reb"]["name"],
                            },
                        ],
                    },
                )
                data.updateData(game["id"], gameData)
            else:  # For scheduled games
                homeOdds = ""
                awayOdds = ""
                if "odds" in game:
                    if game["odds"]["favorite"] == "home":
                        homeOdds = game["odds"]["spread"]
                        awayOdds = game["odds"]["overUnder"]
                    else:
                        awayOdds = game["odds"]["spread"]
                        homeOdds = game["odds"]["overUnder"]

                gameData["data"][3]["data"][0]["data"] += f" ({game['away']['record']})"
                gameData["data"][3]["data"][1]["data"] += f" ({game['home']['record']})"
                gameData["data"].append(
                    {
                        "type": "multi",
                        "data": [
                            {
                                "type": "text",
                                "location": [1, 2],
                                "data": awayOdds,
                            },
                            {
                                "type": "text",
                                "location": [3, 4],
                                "data": homeOdds,
                            },
                        ],
                    }
                )
                gameData["data"].append(
                    {
                        "type": "text",
                        "location": [2, 3],
                        "data": f"{game['start'].strftime('%-m/%-d %-I:%M %p')}",
                    },
                )
                data.updateData(game["id"], gameData)
        time.sleep(1)  # Adjust as needed


def activate_game_updates():
    """Start the background thread for game updates."""
    thread = threading.Thread(target=update_game_info)
    thread.daemon = True
    thread.start()


@app.route("/api/data", methods=["GET"])
@cross_origin()
def get_text():
    send = list(data.getData().values())

    return jsonify(send)


if __name__ == "__main__":
    # printAPI()
    activate_game_updates()
    app.run(debug=True, port=5001, host="0.0.0.0")
    exit()
