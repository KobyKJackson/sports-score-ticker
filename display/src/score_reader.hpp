// score_reader.hpp - Data structures and entry point for reading scores.json.
//
// The Python fetcher writes a JSON file with this shape:
//   { "scoreboards": [ { "sport": "nba", "games": [...] }, ... ], "timestamp": 1234 }
// These structs mirror that structure exactly.

#pragma once

#include <optional>
#include <string>
#include <vector>

// One team in a game — may be home or away.
struct Team
{
    std::string abbreviation; // short code used for logo lookup (e.g. "LAL")
    std::string name;         // display name (e.g. "Los Angeles Lakers")
    int score = -1;           // current score; -1 means not yet available
    std::string record;       // win-loss record (e.g. "32-14")
    std::string logo_url;     // ESPN CDN URL (used by the web frontend)
    int rank = 0;             // AP/Coaches poll ranking; 0 = unranked

    bool has_score() const { return score >= 0; }
};

// Betting line information attached to a game, if available.
struct Odds
{
    std::string spread;     // point spread (e.g. "-3.5")
    std::string over_under; // total points line (e.g. "224.5")
    std::string moneyline;  // moneyline (e.g. "-175")
};

// One game with all associated metadata.
struct Game
{
    std::string game_id;      // ESPN unique game identifier
    std::string sport;        // "nba", "nfl", "mlb", "nhl", "ncaaf", "ncaam"
    std::string status;       // "scheduled", "in_progress", "halftime", "final"
    Team home;                // home team data
    Team away;                // away team data
    std::string clock;        // game clock string (e.g. "4:32" or "")
    std::string period;       // period/quarter label (e.g. "Q3", "2nd Half")
    std::string detail;       // extra info: formatted start time for scheduled games
    std::string venue;        // arena/stadium name
    std::string location;     // city, state
    std::string broadcast;    // TV network (e.g. "ESPN")
    std::optional<Odds> odds; // betting line; nullopt if unavailable
    std::string start_time;   // UTC ISO-8601 start time for scheduled games

    bool is_live() const { return status == "in_progress" || status == "halftime"; }
    bool is_final() const { return status == "final"; }
    bool is_scheduled() const { return status == "scheduled"; }
};

// All games for a single sport in one fetch cycle.
struct SportBoard
{
    std::string sport;       // sport key matching Game::sport
    std::vector<Game> games; // games in ESPN scoreboard order
};

// Top-level container for everything in scores.json.
struct ScoreData
{
    std::vector<SportBoard> boards; // one entry per sport with active games
    double timestamp = 0;           // Unix epoch seconds when fetcher wrote the file

    // Flat list of all games across all sports (pointers into boards).
    std::vector<const Game *> all_games() const;
    // Total game count across all sports.
    int total_games() const;
};

// Parse scores.json at filepath. Returns nullopt on I/O or parse error.
std::optional<ScoreData> load_scores(const std::string &filepath);
