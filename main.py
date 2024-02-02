import time
import requests
import json
import threading
from threading import Lock
from collections import OrderedDict


from nba import *
from nfl import *


def printAPI():
    url = f"http://site.api.espn.com/apis/site/v2/sports/basketball/nba/scoreboard"

    response = requests.get(url)
    if response.status_code != 200:
        print("Failed to retrieve data")

    data = response.json()

    # Save the entire API response in a separate file
    with open("out.json", "w") as api_file:
        json.dump(data, api_file, indent=4)

    exit()


class ThreadSafeTextSections:
    def __init__(self):
        self.sections = OrderedDict()  # Stores sections with unique IDs
        self.lock = Lock()
        self.current_text = ""  # Current compiled text to display

    def add_or_update_section(self, section_id, text):
        with self.lock:
            self.sections[section_id] = text + " | "  # Update or add the section text
            self._compile_text()

    def _compile_text(self):
        """Compile the sections into a single string up to the max width."""
        compiled_text = " ".join(self.sections.values())  # Combine texts with space
        self.current_text = compiled_text

    def get_scrolling_text(self):
        """Return the current text to display."""
        return self.current_text


def scroll_text(text_sections, screenSize=100):
    scroll_position = 0
    while True:
        full_text = text_sections.get_scrolling_text()
        # Ensure text is at least 100 characters by repeating it
        scrolling_text = (
            (full_text * (screenSize // len(full_text) + 1))
            if len(full_text) > 0
            else " " * screenSize
        )
        # Calculate display text considering wrapping
        display_text = (
            scrolling_text[scroll_position : scroll_position + screenSize]
            if scroll_position + screenSize <= len(scrolling_text)
            else scrolling_text[scroll_position:]
            + scrolling_text[: scroll_position + screenSize - len(scrolling_text)]
        )

        # Increment scroll position, wrapping around as needed
        scroll_position = (scroll_position + 1) % len(scrolling_text)

        print("\033c", end="")  # Clear the console
        print(display_text)
        time.sleep(0.2)  # Update every 0.2 seconds for smoother scrolling


if __name__ == "__main__":
    text_sections = ThreadSafeTextSections()

    # Thread for scrolling text
    scrolling_thread = threading.Thread(
        target=scroll_text,
        args=(
            text_sections,
            50,
        ),
    )
    scrolling_thread.daemon = True
    scrolling_thread.start()

    while True:
        for game in get_nba_games() + get_nfl_games():
            if game["status"] == "STATUS_IN_PROGRESS":
                period = game["period"]
                clock = game["clock"]
                text_sections.add_or_update_section(
                    game["id"],
                    f"{game['home']['shortName']} {game['home']['score']} - {game['away']['score']} {game['away']['shortName']}  Q{period} {clock}",
                )
            elif game["status"] == "STATUS_FINAL":
                text_sections.add_or_update_section(
                    game["id"],
                    f"{game['home']['shortName']} {game['home']['score']} - {game['away']['score']} {game['away']['shortName']}  Final",
                )
            else:  # For scheduled games
                text_sections.add_or_update_section(
                    game["id"],
                    f"{game['shortName']} at {game['start'].strftime('%m/%d %I:%M %p')}",
                )
        time.sleep(1)  # Wait more before stopping

    # printAPI()
