"""ESPN API client for fetching live sports scores."""

from __future__ import annotations

import logging
from datetime import datetime, timedelta, timezone

import requests

from models import Game, Odds, ScoreBoard, Team

log = logging.getLogger(__name__)

# ESPN scoreboard API endpoints by sport
ESPN_ENDPOINTS = {
    "nba": "https://site.api.espn.com/apis/site/v2/sports/basketball/nba/scoreboard",
    "nfl": "https://site.api.espn.com/apis/site/v2/sports/football/nfl/scoreboard",
    "mlb": "https://site.api.espn.com/apis/site/v2/sports/baseball/mlb/scoreboard",
    "nhl": "https://site.api.espn.com/apis/site/v2/sports/hockey/nhl/scoreboard",
    "ncaaf": "https://site.api.espn.com/apis/site/v2/sports/football/college-football/scoreboard",
    "ncaam": "https://site.api.espn.com/apis/site/v2/sports/basketball/mens-college-basketball/scoreboard",
}

REQUEST_TIMEOUT = 10


def _safe_get(data: dict, *keys, default=None):
    """Safely navigate nested dicts."""
    current = data
    for key in keys:
        if isinstance(current, dict):
            current = current.get(key, default)
        elif isinstance(current, list) and isinstance(key, int) and key < len(current):
            current = current[key]
        else:
            return default
    return current


def _parse_team(team_data: dict, competitor: dict) -> Team:
    """Parse an ESPN competitor into a Team."""
    abbrev = _safe_get(competitor, "team", "abbreviation", default="???")
    name = _safe_get(competitor, "team", "displayName", default=abbrev)
    score_str = _safe_get(competitor, "score", default=None)
    score = int(score_str) if score_str and score_str.isdigit() else None
    record_items = _safe_get(competitor, "records", default=[])
    record = record_items[0]["summary"] if record_items else ""
    logo = _safe_get(competitor, "team", "logo", default="")
    rank = _safe_get(competitor, "curatedRank", "current", default=None)
    if rank and rank > 25:
        rank = None

    return Team(
        abbreviation=abbrev,
        name=name,
        score=score,
        record=record,
        logo_url=logo,
        rank=rank,
    )


def _parse_odds(competition: dict) -> Odds | None:
    """Parse betting odds from a competition."""
    odds_list = _safe_get(competition, "odds", default=[])
    if not odds_list:
        return None

    primary = odds_list[0]
    spread_str = _safe_get(primary, "details", default="")
    ou_str = _safe_get(primary, "overUnder", default=None)
    over_under = f"O/U {ou_str}" if ou_str else ""

    # Try to extract moneyline from away/home odds
    away_ml = _safe_get(primary, "awayTeamOdds", "moneyLine", default=None)
    home_ml = _safe_get(primary, "homeTeamOdds", "moneyLine", default=None)
    moneyline = ""
    if away_ml and home_ml:
        moneyline = f"{away_ml}/{home_ml}"

    return Odds(spread=spread_str, over_under=over_under, moneyline=moneyline)


def _parse_game(event: dict, sport: str) -> Game:
    """Parse a single ESPN event into a Game."""
    game_id = _safe_get(event, "id", default="")
    competition = _safe_get(event, "competitions", 0, default={})
    competitors = _safe_get(competition, "competitors", default=[])

    # ESPN lists home team first, away second (usually)
    home_data = None
    away_data = None
    for comp in competitors:
        if _safe_get(comp, "homeAway") == "home":
            home_data = comp
        else:
            away_data = comp

    if not home_data and len(competitors) > 0:
        home_data = competitors[0]
    if not away_data and len(competitors) > 1:
        away_data = competitors[1]

    home = _parse_team({}, home_data) if home_data else Team(abbreviation="???", name="Unknown")
    away = _parse_team({}, away_data) if away_data else Team(abbreviation="???", name="Unknown")

    # Game status
    status_obj = _safe_get(event, "status", default={})
    status_type = _safe_get(status_obj, "type", "name", default="STATUS_SCHEDULED")
    detail = _safe_get(status_obj, "type", "detail", default="")
    clock = _safe_get(status_obj, "displayClock", default="")
    period = _safe_get(status_obj, "period", default=0)

    # Map ESPN status to our status
    status_map = {
        "STATUS_SCHEDULED": "scheduled",
        "STATUS_IN_PROGRESS": "in_progress",
        "STATUS_HALFTIME": "halftime",
        "STATUS_END_PERIOD": "in_progress",
        "STATUS_FINAL": "final",
        "STATUS_FINAL_OT": "final",
        "STATUS_DELAYED": "delayed",
        "STATUS_POSTPONED": "postponed",
        "STATUS_CANCELED": "postponed",
        "STATUS_RAIN_DELAY": "delayed",
    }
    status = status_map.get(status_type, "scheduled")

    # Period display
    period_labels = {
        "nba": lambda p: f"Q{p}" if p <= 4 else f"OT{p-4}" if p > 5 else "OT",
        "nfl": lambda p: f"Q{p}" if p <= 4 else f"OT{p-4}" if p > 5 else "OT",
        "mlb": lambda p: f"{'T' if p % 2 == 1 else 'B'}{(p+1)//2}",
        "nhl": lambda p: f"P{p}" if p <= 3 else "OT",
        "ncaaf": lambda p: f"Q{p}" if p <= 4 else f"OT{p-4}" if p > 5 else "OT",
        "ncaam": lambda p: f"H{p}" if p <= 2 else f"OT{p-2}" if p > 3 else "OT",
    }
    period_str = ""
    if period and status in ("in_progress", "halftime"):
        labeler = period_labels.get(sport, lambda p: str(p))
        period_str = labeler(period)

    # Venue
    venue_data = _safe_get(competition, "venue", default={})
    venue_name = _safe_get(venue_data, "fullName", default="")
    venue_city = _safe_get(venue_data, "address", "city", default="")
    venue_state = _safe_get(venue_data, "address", "state", default="")
    location = f"{venue_city}, {venue_state}" if venue_city else ""

    # Broadcast
    broadcasts = _safe_get(competition, "broadcasts", default=[])
    broadcast = ""
    if broadcasts:
        names = _safe_get(broadcasts, 0, "names", default=[])
        broadcast = names[0] if names else ""

    # Odds
    odds = _parse_odds(competition)

    # Start time
    start_time = _safe_get(event, "date", default="")

    return Game(
        game_id=game_id,
        sport=sport,
        status=status,
        home_team=home,
        away_team=away,
        clock=clock,
        period=period_str,
        detail=detail,
        venue=venue_name,
        location=location,
        broadcast=broadcast,
        odds=odds,
        start_time=start_time,
    )


def fetch_scoreboard(sport: str, hours_back: int = 12, hours_ahead: int = 24) -> ScoreBoard:
    """Fetch the current scoreboard for a sport from ESPN.

    Returns a ScoreBoard with parsed games. On error, returns an empty ScoreBoard.
    hours_back: show games that started up to this many hours ago.
    hours_ahead: show games starting up to this many hours from now.
    """
    url = ESPN_ENDPOINTS.get(sport)
    if not url:
        log.error("Unknown sport: %s", sport)
        return ScoreBoard(sport=sport)

    now = datetime.now(timezone.utc)
    window = {"back": hours_back, "ahead": hours_ahead}

    # For most sports ESPN returns today's games by default
    # For college sports we might need a group param for top 25 / FBS
    params = {}
    if sport == "ncaaf":
        params["groups"] = "80"  # FBS
        params["limit"] = "50"
    elif sport == "ncaam":
        params["groups"] = "50"  # D1
        params["limit"] = "50"

    try:
        resp = requests.get(url, params=params, timeout=REQUEST_TIMEOUT)
        resp.raise_for_status()
        data = resp.json()
    except requests.RequestException as e:
        log.warning("Failed to fetch %s scores: %s", sport, e)
        return ScoreBoard(sport=sport)

    events = _safe_get(data, "events", default=[])
    games = []
    for event in events:
        try:
            game = _parse_game(event, sport)
            games.append(game)
        except Exception as e:
            log.warning("Failed to parse %s game: %s", sport, e)

    # Filter games within our window
    filtered = []
    for game in games:
        if game.status in ("in_progress", "halftime", "delayed"):
            filtered.append(game)
        elif game.status == "final":
            # Show finals for a while after the game
            try:
                start = datetime.fromisoformat(game.start_time.replace("Z", "+00:00"))
                if now - start < timedelta(hours=window["back"]):
                    filtered.append(game)
            except (ValueError, TypeError):
                filtered.append(game)
        elif game.status == "scheduled":
            try:
                start = datetime.fromisoformat(game.start_time.replace("Z", "+00:00"))
                if start - now < timedelta(hours=window["ahead"]):
                    filtered.append(game)
            except (ValueError, TypeError):
                filtered.append(game)
        else:
            filtered.append(game)

    return ScoreBoard(
        sport=sport,
        games=filtered,
        last_updated=now.isoformat(),
    )
