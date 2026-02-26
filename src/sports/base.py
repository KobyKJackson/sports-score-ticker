"""Base class for sports data providers."""

from abc import ABC, abstractmethod
from typing import Any


class SportProvider(ABC):
    """Abstract base class for sport-specific data providers.

    To add a new league (e.g., NHL, MLB), create a subclass that implements
    get_games() and format_display_data(), then register it in __init__.py.
    """

    @abstractmethod
    def get_games(self) -> list[dict[str, Any]]:
        """Fetch current games from the ESPN API.

        Returns a list of game dicts with sport-specific fields like id, status,
        home/away team data, scores, etc.
        """
        ...

    @abstractmethod
    def format_display_data(self, game: dict[str, Any]) -> dict[str, Any]:
        """Transform a game dict into the display format expected by the LED app.

        Returns a dict with 'id' and 'data' keys, where 'data' is a list of
        display objects (text, image, multi) that the C++ app renders.
        """
        ...
