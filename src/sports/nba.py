"""NBA data provider using the ESPN API."""

import logging
from datetime import datetime, timedelta
from typing import Any

import requests

from sports.base import SportProvider
from utilities import convert_to_local_time

logger = logging.getLogger(__name__)

ESPN_NBA_URL = "http://site.api.espn.com/apis/site/v2/sports/basketball/nba/scoreboard"


def _get_overall_record(records: list[dict]) -> str:
    for record in records:
        if record["name"] == "overall":
            return record["summary"]
    return ""


def _get_team_data(team: dict) -> dict[str, Any]:
    leaders = {
        "Pts": {"name": "", "value": ""},
        "Reb": {"name": "", "value": ""},
        "Ast": {"name": "", "value": ""},
    }
    if "leaders" in team:
        for stat in team["leaders"]:
            if stat["name"] == "points":
                leaders["Pts"] = {
                    "name": stat["leaders"][0]["athlete"]["shortName"],
                    "value": int(stat["leaders"][0]["value"]),
                }
            elif stat["name"] == "rebounds":
                leaders["Reb"] = {
                    "name": stat["leaders"][0]["athlete"]["shortName"],
                    "value": int(stat["leaders"][0]["value"]),
                }
            elif stat["name"] == "assists":
                leaders["Ast"] = {
                    "name": stat["leaders"][0]["athlete"]["shortName"],
                    "value": int(stat["leaders"][0]["value"]),
                }
    return {
        "shortName": team["team"]["abbreviation"],
        "name": team["team"]["name"],
        "logo": team["team"]["logo"],
        "score": team["score"],
        "color": team["team"]["color"],
        "altColor": team["team"]["alternateColor"],
        "record": _get_overall_record(team["records"]),
        "leaders": leaders,
    }


def _parse_game(game: dict) -> dict[str, Any]:
    comp = game["competitions"][0]
    output = {
        "id": game["id"],
        "shortName": game["shortName"],
        "status": comp["status"]["type"]["name"],
        "statusDetail": comp["status"]["type"]["shortDetail"],
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

    if "odds" in comp:
        odds: dict[str, Any] = {}
        raw_odds = comp["odds"][0]
        odds["overUnder"] = ("O/U " + str(raw_odds["overUnder"])) if "overUnder" in raw_odds else ""
        odds["spread"] = raw_odds.get("spread", "")
        if raw_odds.get("awayTeamOdds", {}).get("favorite") is True:
            odds["favorite"] = "away"
            if odds["spread"] != "":
                odds["spread"] = "-" + str(odds["spread"])
        else:
            odds["favorite"] = "home"
        output["odds"] = odds

    return output


class NBAProvider(SportProvider):
    """Fetches and formats NBA game data from ESPN."""

    def __init__(self, previous_hours: int = 18, upcoming_hours: int = 6):
        self.previous_hours = previous_hours
        self.upcoming_hours = upcoming_hours

    def get_games(self) -> list[dict[str, Any]]:
        today = datetime.now()
        start_str = (today - timedelta(hours=self.previous_hours)).strftime("%Y%m%d")
        end_str = (today + timedelta(hours=self.upcoming_hours)).strftime("%Y%m%d")

        url = f"{ESPN_NBA_URL}?dates={start_str}-{end_str}"
        try:
            response = requests.get(url, timeout=10)
        except requests.RequestException as e:
            logger.error("NBA API request failed: %s", e)
            return []
        if response.status_code != 200:
            logger.warning("NBA API returned status %d", response.status_code)
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
            # Score row
            game_data["data"].append({
                "type": "multi",
                "data": [
                    {"type": "text", "location": [1, 2], "data": game["away"]["score"]},
                    {"type": "text", "location": [3, 4], "data": game["home"]["score"]},
                ],
            })

            # Status text
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

            # Leaders
            game_data["data"].append({
                "type": "multi",
                "data": [
                    {"type": "text", "location": [1], "data": f"PTS {game['away']['leaders']['Pts']['value']} {game['away']['leaders']['Pts']['name']}"},
                    {"type": "text", "location": [2], "data": f"REB {game['away']['leaders']['Reb']['value']} {game['away']['leaders']['Reb']['name']}"},
                    {"type": "text", "location": [3], "data": f"PTS {game['home']['leaders']['Pts']['value']} {game['home']['leaders']['Pts']['name']}"},
                    {"type": "text", "location": [4], "data": f"REB {game['home']['leaders']['Reb']['value']} {game['home']['leaders']['Reb']['name']}"},
                ],
            })

        elif status == "STATUS_FINAL":
            game_data["data"].append({
                "type": "multi",
                "data": [
                    {"type": "text", "location": [1, 2], "data": game["away"]["score"]},
                    {"type": "text", "location": [3, 4], "data": game["home"]["score"]},
                ],
            })
            game_data["data"].append(
                {"type": "text", "location": [2, 3], "data": game["statusDetail"]}
            )
            game_data["data"].append({
                "type": "multi",
                "data": [
                    {"type": "text", "location": [1], "data": f"PTS {game['away']['leaders']['Pts']['value']} {game['away']['leaders']['Pts']['name']}"},
                    {"type": "text", "location": [2], "data": f"REB {game['away']['leaders']['Reb']['value']} {game['away']['leaders']['Reb']['name']}"},
                    {"type": "text", "location": [3], "data": f"PTS {game['home']['leaders']['Pts']['value']} {game['home']['leaders']['Pts']['name']}"},
                    {"type": "text", "location": [4], "data": f"REB {game['home']['leaders']['Reb']['value']} {game['home']['leaders']['Reb']['name']}"},
                ],
            })

        else:  # Scheduled
            home_odds = ""
            away_odds = ""
            if "odds" in game:
                if game["odds"]["favorite"] == "home":
                    home_odds = game["odds"]["spread"]
                    away_odds = game["odds"]["overUnder"]
                else:
                    away_odds = game["odds"]["spread"]
                    home_odds = game["odds"]["overUnder"]

            game_data["data"][3]["data"][0]["data"] += f" ({game['away']['record']})"
            game_data["data"][3]["data"][1]["data"] += f" ({game['home']['record']})"
            game_data["data"].append({
                "type": "multi",
                "data": [
                    {"type": "text", "location": [1, 2], "data": away_odds},
                    {"type": "text", "location": [3, 4], "data": home_odds},
                ],
            })
            game_data["data"].append({
                "type": "text", "location": [2, 3],
                "data": game["start"].strftime("%-m/%-d %-I:%M %p"),
            })

        return game_data
