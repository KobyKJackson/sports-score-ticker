"""NFL data provider using the ESPN API."""

import logging
from datetime import datetime, timedelta
from typing import Any

import requests

from sports.base import SportProvider
from utilities import convert_to_local_time

logger = logging.getLogger(__name__)

ESPN_NFL_URL = "http://site.api.espn.com/apis/site/v2/sports/football/nfl/scoreboard"


def _get_team_data(team: dict) -> dict[str, Any]:
    return {
        "shortName": team["team"]["abbreviation"],
        "name": team["team"].get("name", team["team"]["abbreviation"]),
        "logo": team["team"]["logo"],
        "score": team["score"],
        "color": team["team"].get("color", "000000"),
        "altColor": team["team"].get("alternateColor", "ffffff"),
        "leaders": {},
    }


def _parse_game(game: dict) -> dict[str, Any]:
    comp = game["competitions"][0]
    output: dict[str, Any] = {
        "id": game["id"],
        "shortName": game["shortName"],
        "status": comp["status"]["type"]["name"],
        "start": convert_to_local_time(
            datetime.strptime(game["date"], "%Y-%m-%dT%H:%MZ")
        ),
        "period": comp["status"]["period"],
        "clock": comp["status"]["displayClock"],
    }

    for team in comp["competitors"]:
        if team["homeAway"] == "home":
            output["home"] = _get_team_data(team)
        else:
            output["away"] = _get_team_data(team)

    return output


class NFLProvider(SportProvider):
    """Fetches and formats NFL game data from ESPN."""

    def __init__(self, previous_hours: int = 36, upcoming_hours: int = 300):
        self.previous_hours = previous_hours
        self.upcoming_hours = upcoming_hours

    def get_games(self) -> list[dict[str, Any]]:
        today = datetime.now()
        start_str = (today - timedelta(hours=self.previous_hours)).strftime("%Y%m%d")
        end_str = (today + timedelta(hours=self.upcoming_hours)).strftime("%Y%m%d")

        url = f"{ESPN_NFL_URL}?dates={start_str}-{end_str}"
        try:
            response = requests.get(url, timeout=10)
        except requests.RequestException as e:
            logger.error("NFL API request failed: %s", e)
            return []
        if response.status_code != 200:
            logger.warning("NFL API returned status %d", response.status_code)
            return []

        data = response.json()
        return [_parse_game(game) for game in data.get("events", [])]

    def format_display_data(self, game: dict[str, Any]) -> dict[str, Any]:
        game_data: dict[str, Any] = {
            "id": game["id"],
            "data": [
                {"type": "image", "location": [1, 2, 3, 4], "data": game["away"]["logo"]},
                {"type": "text", "location": [2, 3], "data": "@"},
                {"type": "image", "location": [1, 2, 3, 4], "data": game["home"]["logo"]},
                {
                    "type": "multi",
                    "data": [
                        {
                            "type": "text", "location": [1, 2],
                            "data": game["away"]["name"],
                            "color": game["away"]["color"],
                            "altColor": game["away"]["altColor"],
                        },
                        {
                            "type": "text", "location": [3, 4],
                            "data": game["home"]["name"],
                            "color": game["home"]["color"],
                            "altColor": game["home"]["altColor"],
                        },
                    ],
                },
            ],
        }

        status = game["status"]

        if status in ("STATUS_IN_PROGRESS", "STATUS_HALFTIME", "STATUS_END_PERIOD"):
            game_data["data"].append({
                "type": "multi",
                "data": [
                    {"type": "text", "location": [1, 2], "data": game["away"]["score"]},
                    {"type": "text", "location": [3, 4], "data": game["home"]["score"]},
                ],
            })
            if status == "STATUS_IN_PROGRESS":
                period = game["period"]
                period_str = f"OT{period - 4}" if period > 4 else f"Q{period}"
                status_text = f"{period_str} {game['clock']}"
            elif status == "STATUS_HALFTIME":
                status_text = "HALF"
            else:
                status_text = f"END Q{game['period']}"
            game_data["data"].append(
                {"type": "text", "location": [2, 3], "data": status_text}
            )

        elif status == "STATUS_FINAL":
            game_data["data"].append({
                "type": "multi",
                "data": [
                    {"type": "text", "location": [1, 2], "data": game["away"]["score"]},
                    {"type": "text", "location": [3, 4], "data": game["home"]["score"]},
                ],
            })
            game_data["data"].append(
                {"type": "text", "location": [2, 3], "data": "FINAL"}
            )

        else:  # Scheduled
            game_data["data"].append({
                "type": "text", "location": [2, 3],
                "data": game["start"].strftime("%-m/%-d %-I:%M %p"),
            })

        return game_data
