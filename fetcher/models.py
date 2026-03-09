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
