// score_reader.hpp - Reads and parses the JSON scores file from the Python fetcher.

#pragma once

#include <optional>
#include <string>
#include <vector>

struct Team {
    std::string abbreviation;
    std::string name;
    int score = -1;
    std::string record;
    std::string logo_url;
    int rank = 0;

    bool has_score() const { return score >= 0; }
};

struct Odds {
    std::string spread;
    std::string over_under;
    std::string moneyline;
};

struct Game {
    std::string game_id;
    std::string sport;
    std::string status;
    Team home;
    Team away;
    std::string clock;
    std::string period;
    std::string detail;
    std::string venue;
    std::string location;
    std::string broadcast;
    std::optional<Odds> odds;
    std::string start_time;

    bool is_live() const { return status == "in_progress" || status == "halftime"; }
    bool is_final() const { return status == "final"; }
    bool is_scheduled() const { return status == "scheduled"; }
};

struct SportBoard {
    std::string sport;
    std::vector<Game> games;
};

struct ScoreData {
    std::vector<SportBoard> boards;
    double timestamp = 0;

    std::vector<const Game *> all_games() const;
    int total_games() const;
};

std::optional<ScoreData> load_scores(const std::string &filepath);
