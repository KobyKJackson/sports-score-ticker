# Configuration Reference

## Config File

The ticker reads its configuration from `config/ticker.json` at startup. Both the Python fetcher and C display application read this file.

## Settings

### `sports`
**Type**: Array of strings
**Default**: `["nba", "nfl", "mlb", "nhl", "ncaaf", "ncaam"]`

Which sports to fetch and display. Valid values:
- `"nba"` - NBA games
- `"nfl"` - NFL games
- `"mlb"` - MLB games
- `"nhl"` - NHL games
- `"ncaaf"` - NCAA Football (FBS)
- `"ncaam"` - NCAA Men's Basketball

### `scroll_speed`
**Type**: Integer (1-5)
**Default**: `1`

Pixel scroll speed per frame. Higher = faster scrolling.
- `1` - Slow, easy to read
- `2` - Medium
- `3` - Fast
- `5` - Very fast

### `update_interval_seconds`
**Type**: Integer
**Default**: `30`

How often the Python fetcher polls the ESPN API (in seconds). Minimum recommended: 15 seconds.

### `brightness`
**Type**: Integer (1-100)
**Default**: `80`

LED panel brightness percentage. Lower values extend panel life and reduce power draw.

### `timezone`
**Type**: String (IANA timezone)
**Default**: `"America/New_York"`

Timezone for displaying scheduled game times. Examples:
- `"America/New_York"` - Eastern
- `"America/Chicago"` - Central
- `"America/Denver"` - Mountain
- `"America/Los_Angeles"` - Pacific

### `data_file`
**Type**: String (file path)
**Default**: `"/tmp/scores.json"`

Path to the shared JSON file used for communication between the fetcher and display.

### `logo_dir`
**Type**: String (directory path)
**Default**: `"logos/"`

Directory containing team logo PPM files.

### `show_betting`
**Type**: Boolean
**Default**: `true`

Whether to display betting odds/spread for upcoming games.

### `show_venue`
**Type**: Boolean
**Default**: `true`

Whether to display the venue/location for games.

## Example Configuration

```json
{
  "sports": ["nba", "nfl", "mlb", "nhl", "ncaaf", "ncaam"],
  "scroll_speed": 1,
  "update_interval_seconds": 30,
  "brightness": 80,
  "timezone": "America/New_York",
  "data_file": "/tmp/scores.json",
  "logo_dir": "logos/",
  "show_betting": true,
  "show_venue": true
}
```

## Web Frontend

The score fetcher serves a web UI at `http://<host>:5001` with two features:

### LED Panel Simulator
A pixel-accurate 320x64 canvas that replicates the physical LED display in the browser. Useful for testing layout without hardware. Supports adjustable zoom (2x-5x), optional pixel grid overlay, and shows an FPS counter.

### Config Editor
A form-based editor that reads and writes configuration in real time via the REST API. Changes take effect immediately without restarting services.

## REST API

The score fetcher exposes these endpoints on port 5001 (configurable via `TICKER_WEB_PORT`):

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/scores` | Current score data (same format as `/tmp/scores.json`) |
| GET | `/api/config` | Current configuration |
| PUT | `/api/config` | Update configuration (JSON body with fields to change) |
| POST | `/api/config/reset` | Reset configuration to defaults |

### Example: Update config via API

```bash
curl -X PUT http://localhost:5001/api/config \
  -H "Content-Type: application/json" \
  -d '{"scroll_speed": 2, "brightness": 60}'
```

## Environment Variables

These override config file settings:

| Variable | Overrides | Example |
|----------|-----------|---------|
| `TICKER_CONFIG` | Config file path | `/etc/ticker/ticker.json` |
| `TICKER_DATA_FILE` | `data_file` | `/tmp/scores.json` |
| `TICKER_BRIGHTNESS` | `brightness` | `60` |
| `TICKER_WEB_PORT` | Web UI port | `5001` |
