// score_reader.cpp - JSON score file parser.
//
// Hand-written recursive-descent JSON parser. The JSON from the Python fetcher
// has a well-defined structure, so we keep it dependency-free rather than
// pulling in a JSON library.

#include "score_reader.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace
{

    // Token types emitted by the lexer.
    enum class Token
    {
        LBrace,
        RBrace,
        LBracket,
        RBracket,
        Colon,
        Comma,
        String,
        Number,
        True,
        False,
        Null,
        Eof,
        Error
    };

    // Recursive-descent parser state. Holds the source buffer and current token.
    struct Parser
    {
        const char *src;         // full JSON text (not null-terminated past len)
        int pos;                 // current read cursor
        int len;                 // total source length
        Token type = Token::Eof; // type of the most-recently lexed token
        std::string str_val;     // string payload when type == String
        double num_val = 0;      // numeric payload when type == Number

        // Advance pos past any whitespace characters.
        void skip_ws()
        {
            while (pos < len)
            {
                char c = src[pos];
                if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                    ++pos;
                else
                    break;
            }
        }

        // Lex the next token and store it in type/str_val/num_val.
        void next()
        {
            skip_ws();
            if (pos >= len)
            {
                type = Token::Eof;
                return;
            }

            char c = src[pos];
            switch (c)
            {
            case '{':
                type = Token::LBrace;
                ++pos;
                return;
            case '}':
                type = Token::RBrace;
                ++pos;
                return;
            case '[':
                type = Token::LBracket;
                ++pos;
                return;
            case ']':
                type = Token::RBracket;
                ++pos;
                return;
            case ':':
                type = Token::Colon;
                ++pos;
                return;
            case ',':
                type = Token::Comma;
                ++pos;
                return;
            case '"':
            {
                // Collect string contents, handling backslash escapes.
                ++pos;
                str_val.clear();
                while (pos < len && src[pos] != '"')
                {
                    if (src[pos] == '\\' && pos + 1 < len)
                    {
                        ++pos; // skip the backslash
                        switch (src[pos])
                        {
                        case '"':
                        case '\\':
                        case '/':
                            str_val += src[pos];
                            break;
                        case 'n':
                            str_val += '\n';
                            break;
                        case 't':
                            str_val += '\t';
                            break;
                        default:
                            str_val += src[pos];
                            break; // pass through unknown escapes
                        }
                    }
                    else
                    {
                        str_val += src[pos];
                    }
                    ++pos;
                }
                if (pos < len)
                    ++pos; // consume closing '"'
                type = Token::String;
                return;
            }
            case 't': // "true"
                if (pos + 3 < len && std::strncmp(src + pos, "true", 4) == 0)
                {
                    pos += 4;
                    type = Token::True;
                    return;
                }
                break;
            case 'f': // "false"
                if (pos + 4 < len && std::strncmp(src + pos, "false", 5) == 0)
                {
                    pos += 5;
                    type = Token::False;
                    return;
                }
                break;
            case 'n': // "null"
                if (pos + 3 < len && std::strncmp(src + pos, "null", 4) == 0)
                {
                    pos += 4;
                    type = Token::Null;
                    return;
                }
                break;
            default:
                break;
            }

            // Number: optional leading '-', digits, optional decimal/exponent.
            if (c == '-' || (c >= '0' && c <= '9'))
            {
                int start = pos;
                while (pos < len)
                {
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

            // Unrecognised character — flag as error and advance to avoid infinite loops.
            type = Token::Error;
            ++pos;
        }

        // Skip over a complete JSON value (any type), consuming its tokens.
        void skip_value()
        {
            if (type == Token::LBrace)
            {
                // Skip object: consume key-value pairs until '}'
                next();
                while (type != Token::RBrace && type != Token::Eof)
                {
                    if (type == Token::String)
                        next(); // key
                    if (type == Token::Colon)
                        next();   // ':'
                    skip_value(); // value
                    if (type == Token::Comma)
                        next();
                }
                if (type == Token::RBrace)
                    next();
            }
            else if (type == Token::LBracket)
            {
                // Skip array: consume elements until ']'
                next();
                while (type != Token::RBracket && type != Token::Eof)
                {
                    skip_value();
                    if (type == Token::Comma)
                        next();
                }
                if (type == Token::RBracket)
                    next();
            }
            else
            {
                // Scalar: just advance past it
                next();
            }
        }

        // Return the current string token value, then advance to the next token.
        // Caller must ensure type == String before calling.
        std::string consume_key()
        {
            std::string key = str_val;
            next(); // advance past the key string
            if (type == Token::Colon)
                next(); // consume ':'
            return key;
        }

        // Return the current string token value, then advance.
        std::string consume_string()
        {
            std::string val = str_val;
            next();
            return val;
        }
    };

    // Parse a Team object from the current '{' token.
    Team parse_team(Parser &p)
    {
        Team t;
        if (p.type != Token::LBrace)
        {
            p.skip_value();
            return t;
        }
        p.next(); // consume '{'

        while (p.type == Token::String)
        {
            auto key = p.consume_key(); // reads key + ':'

            if (key == "abbreviation" && p.type == Token::String)
                t.abbreviation = p.consume_string();
            else if (key == "name" && p.type == Token::String)
                t.name = p.consume_string();
            else if (key == "score")
            {
                // Score may be a number or null (pre-game)
                if (p.type == Token::Number)
                {
                    t.score = static_cast<int>(p.num_val);
                    p.next();
                }
                else if (p.type == Token::Null)
                    p.next();
                else
                    p.skip_value();
            }
            else if (key == "record" && p.type == Token::String)
                t.record = p.consume_string();
            else if (key == "logo_url" && p.type == Token::String)
                t.logo_url = p.consume_string();
            else if (key == "rank")
            {
                // Rank may be a number or null (unranked)
                if (p.type == Token::Number)
                {
                    t.rank = static_cast<int>(p.num_val);
                    p.next();
                }
                else if (p.type == Token::Null)
                    p.next();
                else
                    p.skip_value();
            }
            else
            {
                p.skip_value();
                continue;
            } // unknown key — skip value, no comma advance

            if (p.type == Token::Comma)
                p.next();
        }
        if (p.type == Token::RBrace)
            p.next(); // consume '}'
        return t;
    }

    // Parse an Odds object, or return nullopt for JSON null.
    std::optional<Odds> parse_odds(Parser &p)
    {
        if (p.type == Token::Null)
        {
            p.next();
            return std::nullopt;
        }
        if (p.type != Token::LBrace)
        {
            p.skip_value();
            return std::nullopt;
        }
        p.next(); // consume '{'

        Odds o;
        while (p.type == Token::String)
        {
            auto key = p.consume_key();

            if (key == "spread" && p.type == Token::String)
                o.spread = p.consume_string();
            else if (key == "over_under" && p.type == Token::String)
                o.over_under = p.consume_string();
            else if (key == "moneyline" && p.type == Token::String)
                o.moneyline = p.consume_string();
            else
            {
                p.skip_value();
                continue;
            }

            if (p.type == Token::Comma)
                p.next();
        }
        if (p.type == Token::RBrace)
            p.next(); // consume '}'
        return o;
    }

    // Parse a Game object from the current '{' token.
    Game parse_game(Parser &p)
    {
        Game g;
        if (p.type != Token::LBrace)
        {
            p.skip_value();
            return g;
        }
        p.next(); // consume '{'

        while (p.type == Token::String)
        {
            auto key = p.consume_key();

            if (key == "game_id" && p.type == Token::String)
                g.game_id = p.consume_string();
            else if (key == "sport" && p.type == Token::String)
                g.sport = p.consume_string();
            else if (key == "status" && p.type == Token::String)
                g.status = p.consume_string();
            else if (key == "home_team")
                g.home = parse_team(p);
            else if (key == "away_team")
                g.away = parse_team(p);
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
                g.odds = parse_odds(p);
            else if (key == "start_time" && p.type == Token::String)
                g.start_time = p.consume_string();
            else
            {
                p.skip_value();
                continue;
            }

            if (p.type == Token::Comma)
                p.next();
        }
        if (p.type == Token::RBrace)
            p.next(); // consume '}'
        return g;
    }

    // Parse one sport scoreboard block { "sport": "...", "games": [...] }.
    SportBoard parse_sport_board(Parser &p)
    {
        SportBoard sb;
        if (p.type != Token::LBrace)
        {
            p.skip_value();
            return sb;
        }
        p.next(); // consume '{'

        while (p.type == Token::String)
        {
            auto key = p.consume_key();

            if (key == "sport" && p.type == Token::String)
            {
                sb.sport = p.consume_string();
            }
            else if (key == "games" && p.type == Token::LBracket)
            {
                // Parse the games array
                p.next(); // consume '['
                while (p.type != Token::RBracket && p.type != Token::Eof)
                {
                    sb.games.push_back(parse_game(p));
                    if (p.type == Token::Comma)
                        p.next();
                }
                if (p.type == Token::RBracket)
                    p.next(); // consume ']'
            }
            else
            {
                p.skip_value();
                continue;
            }
            if (p.type == Token::Comma)
                p.next();
        }
        if (p.type == Token::RBrace)
            p.next(); // consume '}'
        return sb;
    }

    // Parse a Notification object from the current '{' token.
    Notification parse_notification(Parser &p)
    {
        Notification n;
        if (p.type != Token::LBrace)
        {
            p.skip_value();
            return n;
        }
        p.next(); // consume '{'

        while (p.type == Token::String)
        {
            auto key = p.consume_key();

            if (key == "type" && p.type == Token::String)
                n.type = p.consume_string();
            else if (key == "game")
                n.game = parse_game(p);
            else if (key == "timestamp" && p.type == Token::Number)
            {
                n.timestamp = p.num_val;
                p.next();
            }
            else
            {
                p.skip_value();
                continue;
            }

            if (p.type == Token::Comma)
                p.next();
        }
        if (p.type == Token::RBrace)
            p.next(); // consume '}'
        return n;
    }

} // anonymous namespace

// Return flat list of all games across all sport boards (pointers into boards vector).
std::vector<const Game *> ScoreData::all_games() const
{
    std::vector<const Game *> result;
    for (auto &board : boards)
        for (auto &game : board.games)
            result.push_back(&game);
    return result;
}

// Return total game count across all sports.
int ScoreData::total_games() const
{
    int n = 0;
    for (auto &board : boards)
        n += static_cast<int>(board.games.size());
    return n;
}

// Read and parse the scores JSON file. Returns nullopt on any error.
std::optional<ScoreData> load_scores(const std::string &filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
    {
        std::fprintf(stderr, "load_scores: cannot open '%s': ", filepath.c_str());
        std::perror(nullptr);
        return std::nullopt;
    }

    // Read entire file into memory
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    if (content.empty())
    {
        std::fprintf(stderr, "load_scores: file is empty\n");
        return std::nullopt;
    }
    // Sanity cap — a valid scores file should never exceed 1 MB
    if (content.size() > 1024 * 1024)
    {
        std::fprintf(stderr, "load_scores: file too large (%zu bytes)\n", content.size());
        return std::nullopt;
    }

    // Start recursive-descent parse at the top-level object
    Parser p{content.c_str(), 0, static_cast<int>(content.size())};
    p.next(); // lex first token

    if (p.type != Token::LBrace)
    {
        std::fprintf(stderr, "load_scores: JSON does not start with '{' (got char '%c')\n", content[0]);
        return std::nullopt;
    }
    p.next(); // consume '{'

    ScoreData data;

    // Top-level keys: "scoreboards" (array) and "timestamp" (number)
    while (p.type == Token::String)
    {
        auto key = p.consume_key();

        if (key == "scoreboards" && p.type == Token::LBracket)
        {
            p.next(); // consume '['
            while (p.type != Token::RBracket && p.type != Token::Eof)
            {
                data.boards.push_back(parse_sport_board(p));
                if (p.type == Token::Comma)
                    p.next();
            }
            if (p.type == Token::RBracket)
                p.next(); // consume ']'
        }
        else if (key == "timestamp" && p.type == Token::Number)
        {
            data.timestamp = p.num_val;
            p.next();
        }
        else
        {
            p.skip_value(); // ignore unknown top-level keys
            continue;
        }
        if (p.type == Token::Comma)
            p.next();
    }

    return data;
}

// Read and parse the notifications JSON file. Returns nullopt on any error.
std::optional<NotificationData> load_notifications(const std::string &filepath)
{
    std::ifstream file(filepath, std::ios::binary);
    if (!file)
        return std::nullopt;

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();

    if (content.empty())
        return std::nullopt;
    if (content.size() > 1024 * 1024)
        return std::nullopt;

    Parser p{content.c_str(), 0, static_cast<int>(content.size())};
    p.next();

    if (p.type != Token::LBrace)
        return std::nullopt;
    p.next();

    NotificationData data;

    while (p.type == Token::String)
    {
        auto key = p.consume_key();

        if (key == "notifications" && p.type == Token::LBracket)
        {
            p.next(); // consume '['
            while (p.type != Token::RBracket && p.type != Token::Eof)
            {
                data.notifications.push_back(parse_notification(p));
                if (p.type == Token::Comma)
                    p.next();
            }
            if (p.type == Token::RBracket)
                p.next();
        }
        else
        {
            p.skip_value();
            continue;
        }
        if (p.type == Token::Comma)
            p.next();
    }

    return data;
}

// Clear notifications file by writing an empty array.
void clear_notifications(const std::string &filepath)
{
    std::string tmp = filepath + ".tmp";
    std::ofstream f(tmp);
    if (f)
    {
        f << R"({"notifications":[]})";
        f.close();
        std::rename(tmp.c_str(), filepath.c_str());
    }
}
