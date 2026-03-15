"""Data models for sports scores."""

from dataclasses import dataclass, field, asdict
from typing import Optional


@dataclass
class Team:
    abbreviation: str
    name: str
    score: Optional[int] = None
    record: str = ""
    logo_url: str = ""
    rank: Optional[int] = None  # for college teams


@dataclass
class Odds:
    spread: str = ""       # e.g. "LAL -3.5"
    over_under: str = ""   # e.g. "O/U 215.5"
    moneyline: str = ""    # e.g. "LAL -150"


@dataclass
class Game:
    game_id: str
    sport: str                    # nba, nfl, mlb, nhl, ncaaf, ncaam
    status: str                   # scheduled, in_progress, halftime, final, delayed, postponed
    home_team: Team = field(default_factory=Team)
    away_team: Team = field(default_factory=Team)
    clock: str = ""               # "5:32" or "End 3rd" or "7:00 PM ET"
    period: str = ""              # "Q3", "2nd", "T7", "P2", etc.
    detail: str = ""              # Full status text from ESPN
    venue: str = ""               # "Madison Square Garden"
    location: str = ""            # "New York, NY"
    broadcast: str = ""           # "ESPN", "TNT"
    odds: Optional[Odds] = None
    start_time: str = ""          # ISO 8601 time for scheduled games

    def to_dict(self):
        return asdict(self)


@dataclass
class ScoreBoard:
    sport: str
    games: list = field(default_factory=list)
    last_updated: str = ""

    def to_dict(self):
        return {
            "sport": self.sport,
            "games": [g.to_dict() for g in self.games],
            "last_updated": self.last_updated,
        }


@dataclass
class BracketMatchup:
    """A single matchup in a tournament bracket."""
    game_id: str = ""
    round_name: str = ""         # "First Round", "Second Round", "Sweet 16", etc.
    round_number: int = 0        # 1-6 (1=First Round, 6=Championship)
    region: str = ""             # "East", "West", "South", "Midwest"
    home_team: Optional[Team] = None
    away_team: Optional[Team] = None
    home_seed: int = 0
    away_seed: int = 0
    status: str = "scheduled"    # scheduled, in_progress, halftime, final
    clock: str = ""
    period: str = ""
    detail: str = ""
    start_time: str = ""

    def to_dict(self):
        return {
            "game_id": self.game_id,
            "round_name": self.round_name,
            "round_number": self.round_number,
            "region": self.region,
            "home_team": asdict(self.home_team) if self.home_team else None,
            "away_team": asdict(self.away_team) if self.away_team else None,
            "home_seed": self.home_seed,
            "away_seed": self.away_seed,
            "status": self.status,
            "clock": self.clock,
            "period": self.period,
            "detail": self.detail,
            "start_time": self.start_time,
        }


@dataclass
class BracketData:
    """Tournament bracket data organized by region and round."""
    tournament_name: str = ""
    current_round: str = ""
    regions: list = field(default_factory=list)  # list of region names
    matchups: list = field(default_factory=list)  # list of BracketMatchup
    last_updated: str = ""

    def to_dict(self):
        return {
            "tournament_name": self.tournament_name,
            "current_round": self.current_round,
            "regions": self.regions,
            "matchups": [m.to_dict() for m in self.matchups],
            "last_updated": self.last_updated,
        }
