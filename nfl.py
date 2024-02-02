import requests
from datetime import datetime, timedelta
from utilities import *


def format_nfl_game_info(game):
    competition = game["competitions"][0]
    competitors = competition["competitors"]
    teams = [team["team"]["abbreviation"] for team in competitors]
    logos = [team["team"]["logo"] for team in competitors]
    game_date_utc = datetime.strptime(game["date"], "%Y-%m-%dT%H:%MZ")
    game_date_local = convert_to_local_time(game_date_utc)
    game_status = competition["status"]["type"]["name"]

    if game_status == "STATUS_IN_PROGRESS":
        period = competition["status"]["period"]
        clock = competition["status"]["displayClock"]
        score = " - ".join([team["score"] for team in competitors])
        return {
            "string": f"{teams[0]} {score} {teams[1]} Q{period} {clock}",
            "logo_h": logos[0],
            "logo_a": logos[1],
        }
    elif game_status == "STATUS_FINAL":
        score = " - ".join([team["score"] for team in competitors])
        return {
            "string": f"{teams[0]} {score} {teams[1]} Final",
            "logo_h": logos[0],
            "logo_a": logos[1],
        }
    else:
        # For scheduled games
        return {
            "string": f"{teams[0]} vs {teams[1]} at {game_date_local.strftime('%m/%d %I:%M %p')}",
            "logo_h": logos[0],
            "logo_a": logos[1],
        }


def get_nfl_games():
    today = datetime.now()
    start_date = today - timedelta(days=2)
    end_date = today + timedelta(days=3)
    start_str = start_date.strftime("%Y%m%d")
    end_str = end_date.strftime("%Y%m%d")

    url = f"http://site.api.espn.com/apis/site/v2/sports/football/nfl/scoreboard?dates={start_str}-{end_str}"

    response = requests.get(url)
    if response.status_code != 200:
        return ["Failed to retrieve data"]

    data = response.json()
    games_info = [format_nfl_game_info(game) for game in data.get("events", [])]
    return games_info
