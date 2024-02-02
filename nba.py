import requests
from datetime import datetime, timedelta
from utilities import *


def getTeamData(team):
    retVal = {}
    retVal["shortName"] = team["team"]["abbreviation"]
    retVal["logo"] = team["team"]["logo"]
    retVal["score"] = team["score"]
    leaders = {}
    for stat in team["leaders"]:
        if stat["name"] == "points":
            pts = {}
            pts["name"] = stat["leaders"][0]["athlete"]["shortName"]
            pts["value"] = stat["leaders"][0]["value"]
            leaders["Pts"] = pts
        elif stat["name"] == "rebounds":
            reb = {}
            reb["name"] = stat["leaders"][0]["athlete"]["shortName"]
            reb["value"] = stat["leaders"][0]["value"]
            leaders["Reb"] = reb
        elif stat["name"] == "assists":
            ast = {}
            ast["name"] = stat["leaders"][0]["athlete"]["shortName"]
            ast["value"] = stat["leaders"][0]["value"]
            leaders["Ast"] = ast
    retVal["leaders"] = leaders
    return retVal


def format_nba_game_info(game):
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


def get_nba_games(previous_hours=12, upcoming_hours=12):
    today = datetime.now()
    start_date = today - timedelta(hours=previous_hours)
    end_date = today + timedelta(hours=upcoming_hours)
    start_str = start_date.strftime("%Y%m%d")
    end_str = end_date.strftime("%Y%m%d")

    url = f"http://site.api.espn.com/apis/site/v2/sports/basketball/nba/scoreboard??dates={start_str}-{end_str}"

    response = requests.get(url)
    if response.status_code != 200:
        return ["Failed to retrieve data"]

    data = response.json()

    games_info = [format_nba_game_info(game) for game in data.get("events", [])]
    return games_info
