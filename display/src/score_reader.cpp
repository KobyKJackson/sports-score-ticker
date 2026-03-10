// score_reader.cpp - JSON score file parser.
//
// Hand-written JSON parser. The JSON from the Python fetcher has a well-defined
// structure, so we keep it dependency-free rather than pulling in a JSON library.

#include "score_reader.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace {

enum class Token {
    LBrace, RBrace, LBracket, RBracket,
    Colon, Comma, String, Number,
    True, False, Null, Eof, Error
};

struct Parser {
    const char *src;
    int pos;
    int len;
    Token type = Token::Eof;
    std::string str_val;
    double num_val = 0;

    void skip_ws() {
        while (pos < len) {
            char c = src[pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                ++pos;
            else
                break;
        }
    }

    void next() {
        skip_ws();
        if (pos >= len) { type = Token::Eof; return; }

        char c = src[pos];
        switch (c) {
        case '{': type = Token::LBrace;   ++pos; return;
        case '}': type = Token::RBrace;   ++pos; return;
        case '[': type = Token::LBracket; ++pos; return;
        case ']': type = Token::RBracket; ++pos; return;
        case ':': type = Token::Colon;    ++pos; return;
        case ',': type = Token::Comma;    ++pos; return;
        case '"': {
            ++pos;
            str_val.clear();
            while (pos < len && src[pos] != '"') {
                if (src[pos] == '\\' && pos + 1 < len) {
                    ++pos;
                    switch (src[pos]) {
                    case '"': case '\\': case '/': str_val += src[pos]; break;
                    case 'n': str_val += '\n'; break;
                    case 't': str_val += '\t'; break;
                    default:  str_val += src[pos]; break;
                    }
                } else {
                    str_val += src[pos];
                }
                ++pos;
            }
            if (pos < len) ++pos;
            type = Token::String;
            return;
        }
        case 't':
            if (pos + 3 < len && std::strncmp(src + pos, "true", 4) == 0) {
                pos += 4; type = Token::True; return;
            }
            break;
        case 'f':
            if (pos + 4 < len && std::strncmp(src + pos, "false", 5) == 0) {
                pos += 5; type = Token::False; return;
            }
            break;
        case 'n':
            if (pos + 3 < len && std::strncmp(src + pos, "null", 4) == 0) {
                pos += 4; type = Token::Null; return;
            }
            break;
        default:
            break;
        }

        if (c == '-' || (c >= '0' && c <= '9')) {
            int start = pos;
            while (pos < len) {
                char ch = src[pos];
                if ((ch >= '0' && ch <= '9') || ch == '-' || ch == '.' ||
                    ch == 'e' || ch == 'E' || ch == '+')
                    ++pos;
                else
                    break;
            }
            num_val = std::atof(std::string(src + start, pos - start).c_str());
            type = Token::Number;
            return;
        }

        type = Token::Error;
        ++pos;
    }

    void skip_value() {
        if (type == Token::LBrace) {
            next();
            while (type != Token::RBrace && type != Token::Eof) {
                if (type == Token::String) next();
                if (type == Token::Colon) next();
                skip_value();
                if (type == Token::Comma) next();
            }
            if (type == Token::RBrace) next();
        } else if (type == Token::LBracket) {
            next();
            while (type != Token::RBracket && type != Token::Eof) {
                skip_value();
                if (type == Token::Comma) next();
            }
            if (type == Token::RBracket) next();
        } else {
            next();
        }
    }

    // Consume the current key string and advance past the colon
    std::string consume_key() {
        std::string key = str_val;
        next();
        if (type == Token::Colon) next();
        return key;
    }

    std::string consume_string() {
        std::string val = str_val;
        next();
        return val;
    }
};

Team parse_team(Parser &p) {
    Team t;
    if (p.type != Token::LBrace) { p.skip_value(); return t; }
    p.next();

    while (p.type == Token::String) {
        auto key = p.consume_key();

        if (key == "abbreviation" && p.type == Token::String)
            t.abbreviation = p.consume_string();
        else if (key == "name" && p.type == Token::String)
            t.name = p.consume_string();
        else if (key == "score") {
            if (p.type == Token::Number) { t.score = static_cast<int>(p.num_val); p.next(); }
            else if (p.type == Token::Null) p.next();
            else p.skip_value();
        }
        else if (key == "record" && p.type == Token::String)
            t.record = p.consume_string();
        else if (key == "logo_url" && p.type == Token::String)
            t.logo_url = p.consume_string();
        else if (key == "rank") {
            if (p.type == Token::Number) { t.rank = static_cast<int>(p.num_val); p.next(); }
            else if (p.type == Token::Null) p.next();
            else p.skip_value();
        }
        else { p.skip_value(); continue; }

        if (p.type == Token::Comma) p.next();
    }
    if (p.type == Token::RBrace) p.next();
    return t;
}

std::optional<Odds> parse_odds(Parser &p) {
    if (p.type == Token::Null) { p.next(); return std::nullopt; }
    if (p.type != Token::LBrace) { p.skip_value(); return std::nullopt; }
    p.next();

    Odds o;
    while (p.type == Token::String) {
        auto key = p.consume_key();

        if (key == "spread" && p.type == Token::String)
            o.spread = p.consume_string();
        else if (key == "over_under" && p.type == Token::String)
            o.over_under = p.consume_string();
        else if (key == "moneyline" && p.type == Token::String)
            o.moneyline = p.consume_string();
        else { p.skip_value(); continue; }

        if (p.type == Token::Comma) p.next();
    }
    if (p.type == Token::RBrace) p.next();
    return o;
}

Game parse_game(Parser &p) {
    Game g;
    if (p.type != Token::LBrace) { p.skip_value(); return g; }
    p.next();

    while (p.type == Token::String) {
        auto key = p.consume_key();

        if (key == "game_id" && p.type == Token::String)
            g.game_id = p.consume_string();
        else if (key == "sport" && p.type == Token::String)
            g.sport = p.consume_string();
        else if (key == "status" && p.type == Token::String)
            g.status = p.consume_string();
        else if (key == "home_team")
            { g.home = parse_team(p); continue; }
        else if (key == "away_team")
            { g.away = parse_team(p); continue; }
        else if (key == "clock" && p.type == Token::String)
            g.clock = p.consume_string();
        else if (key == "period" && p.type == Token::String)
            g.period = p.consume_string();
        else if (key == "detail" && p.type == Token::String)
            g.detail = p.consume_string();
        else if (key == "venue" && p.type == Token::String)
            g.venue = p.consume_string();
        else if (key == "location" && p.type == Token::String)
            g.location = p.consume_string();
        else if (key == "broadcast" && p.type == Token::String)
            g.broadcast = p.consume_string();
        else if (key == "odds")
            { g.odds = parse_odds(p); continue; }
        else if (key == "start_time" && p.type == Token::String)
            g.start_time = p.consume_string();
        else { p.skip_value(); continue; }

        if (p.type == Token::Comma) p.next();
    }
    if (p.type == Token::RBrace) p.next();
    return g;
}

SportBoard parse_sport_board(Parser &p) {
    SportBoard sb;
    if (p.type != Token::LBrace) { p.skip_value(); return sb; }
    p.next();

    while (p.type == Token::String) {
        auto key = p.consume_key();

        if (key == "sport" && p.type == Token::String) {
            sb.sport = p.consume_string();
        } else if (key == "games" && p.type == Token::LBracket) {
            p.next();
            while (p.type != Token::RBracket && p.type != Token::Eof) {
                sb.games.push_back(parse_game(p));
                if (p.type == Token::Comma) p.next();
            }
            if (p.type == Token::RBracket) p.next();
        } else {
            p.skip_value();
            continue;
        }
        if (p.type == Token::Comma) p.next();
    }
    if (p.type == Token::RBrace) p.next();
    return sb;
}

} // anonymous namespace

std::vector<const Game *> ScoreData::all_games() const {
    std::vector<const Game *> result;
    for (auto &board : boards)
        for (auto &game : board.games)
            result.push_back(&game);
    return result;
}

int ScoreData::total_games() const {
    int n = 0;
    for (auto &board : boards)
        n += static_cast<int>(board.games.size());
    return n;
}

std::optional<ScoreData> load_scores(const std::string &filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) return std::nullopt;

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    if (content.empty() || content.size() > 1024 * 1024)
        return std::nullopt;

    Parser p{content.c_str(), 0, static_cast<int>(content.size())};
    p.next();

    if (p.type != Token::LBrace) return std::nullopt;
    p.next();

    ScoreData data;

    while (p.type == Token::String) {
        auto key = p.consume_key();

        if (key == "scoreboards" && p.type == Token::LBracket) {
            p.next();
            while (p.type != Token::RBracket && p.type != Token::Eof) {
                data.boards.push_back(parse_sport_board(p));
                if (p.type == Token::Comma) p.next();
            }
            if (p.type == Token::RBracket) p.next();
        } else if (key == "timestamp" && p.type == Token::Number) {
            data.timestamp = p.num_val;
            p.next();
        } else {
            p.skip_value();
            continue;
        }
        if (p.type == Token::Comma) p.next();
    }

    return data;
}
