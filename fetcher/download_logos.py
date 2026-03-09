#!/usr/bin/env python3
"""Download team logos from ESPN and convert to PPM format for the LED display.

Fetches team logos for all configured sports, resizes them to 28x28 pixels,
and saves as PPM (P6) files that the C display application can read directly.

Requires: Pillow (pip install Pillow)
"""

import json
import os
import struct
import sys
from io import BytesIO
from pathlib import Path

import requests

# ESPN team list endpoints
ESPN_TEAMS = {
    "nba": "https://site.api.espn.com/apis/site/v2/sports/basketball/nba/teams",
    "nfl": "https://site.api.espn.com/apis/site/v2/sports/football/nfl/teams",
    "mlb": "https://site.api.espn.com/apis/site/v2/sports/baseball/mlb/teams",
    "nhl": "https://site.api.espn.com/apis/site/v2/sports/hockey/nhl/teams",
}

LOGO_SIZE = 28  # pixels


def download_image(url: str) -> bytes | None:
    """Download an image from a URL."""
    try:
        resp = requests.get(url, timeout=10)
        resp.raise_for_status()
        return resp.content
    except requests.RequestException as e:
        print(f"  Failed to download {url}: {e}")
        return None


def image_to_ppm(image_data: bytes, size: int) -> bytes | None:
    """Convert image data to PPM P6 format, resized to size x size.

    Uses Pillow if available, otherwise returns None.
    """
    try:
        from PIL import Image
    except ImportError:
        print("Error: Pillow is required for logo conversion.")
        print("Install it with: pip3 install Pillow")
        sys.exit(1)

    try:
        img = Image.open(BytesIO(image_data))
        img = img.convert("RGBA")

        # Resize maintaining aspect ratio
        img.thumbnail((size, size), Image.Resampling.LANCZOS)

        # Create a black background and paste the logo centered
        bg = Image.new("RGB", (size, size), (0, 0, 0))
        offset_x = (size - img.width) // 2
        offset_y = (size - img.height) // 2
        bg.paste(img, (offset_x, offset_y), img)  # Use alpha as mask

        # Convert to PPM P6 bytes
        pixels = bg.tobytes()
        header = f"P6\n{size} {size}\n255\n".encode()
        return header + pixels

    except Exception as e:
        print(f"  Image conversion failed: {e}")
        return None


def fetch_teams(sport: str) -> list[dict]:
    """Fetch team list from ESPN API."""
    url = ESPN_TEAMS.get(sport)
    if not url:
        return []

    try:
        resp = requests.get(url, params={"limit": "100"}, timeout=10)
        resp.raise_for_status()
        data = resp.json()

        teams = []
        for item in data.get("sports", [{}])[0].get("leagues", [{}])[0].get("teams", []):
            team = item.get("team", {})
            abbrev = team.get("abbreviation", "").lower()
            name = team.get("displayName", "")
            logo = team.get("logos", [{}])[0].get("href", "") if team.get("logos") else ""

            if abbrev and logo:
                teams.append({"abbreviation": abbrev, "name": name, "logo_url": logo})

        return teams
    except Exception as e:
        print(f"Failed to fetch {sport} teams: {e}")
        return []


def main():
    project_root = Path(__file__).resolve().parent.parent
    logo_dir = project_root / "logos"
    logo_dir.mkdir(exist_ok=True)

    print(f"Downloading team logos to {logo_dir}")
    print(f"Logo size: {LOGO_SIZE}x{LOGO_SIZE} pixels")
    print()

    total = 0
    for sport, url in ESPN_TEAMS.items():
        print(f"=== {sport.upper()} ===")
        teams = fetch_teams(sport)
        print(f"  Found {len(teams)} teams")

        for team in teams:
            abbrev = team["abbreviation"]
            logo_url = team["logo_url"]

            # Output path: logos/abbrev.ppm (e.g., logos/lal.ppm)
            out_path = logo_dir / f"{abbrev}.ppm"

            if out_path.exists():
                print(f"  {abbrev}: already exists, skipping")
                continue

            print(f"  {abbrev}: downloading...", end=" ")
            img_data = download_image(logo_url)
            if not img_data:
                continue

            ppm_data = image_to_ppm(img_data, LOGO_SIZE)
            if not ppm_data:
                continue

            out_path.write_bytes(ppm_data)
            print(f"saved ({len(ppm_data)} bytes)")
            total += 1

    print()
    print(f"Done! Downloaded {total} new logos.")
    print(f"Logo directory: {logo_dir}")


if __name__ == "__main__":
    main()
