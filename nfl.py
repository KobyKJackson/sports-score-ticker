import requests
from datetime import datetime, timedelta
from utilities import *


def getTeamData(team):
    retVal = {}
    retVal["shortName"] = team["team"]["abbreviation"]
    retVal["logo"] = team["team"]["logo"]
    retVal["score"] = team["score"]
    leaders = {}
    retVal["leaders"] = leaders
    return retVal


def format_nfl_game_info(game):
    output = {}
    output["id"] = game["id"]
    output["shortName"] = game["shortName"]
    output["status"] = game["competitions"][0]["status"]["type"]["name"]
    output["start"] = convert_to_local_time(
        datetime.strptime(game["date"], "%Y-%m-%dT%H:%MZ")
    )
    output["period"] = game["competitions"][0]["status"]["period"]
    output["clock"] = game["competitions"][0]["status"]["displayClock"]

    for team in game["competitions"][0]["competitors"]:
        if team["homeAway"] == "home":
            output["home"] = getTeamData(team)
        else:
            output["away"] = getTeamData(team)
    return output


def get_nfl_games(previous_hours=36, upcoming_hours=72):
    today = datetime.now()
    start_date = today - timedelta(hours=previous_hours)
    end_date = today + timedelta(hours=upcoming_hours)
    start_str = start_date.strftime("%Y%m%d")
    end_str = end_date.strftime("%Y%m%d")

    url = f"http://site.api.espn.com/apis/site/v2/sports/football/nfl/scoreboard?dates={start_str}-{end_str}"

    response = requests.get(url)
    if response.status_code != 200:
        return ["Failed to retrieve data"]

    data = response.json()

    games_info = [format_nfl_game_info(game) for game in data.get("events", [])]
    return games_info
