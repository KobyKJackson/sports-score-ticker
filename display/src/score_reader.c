/*
 * score_reader.c - JSON score file parser.
 *
 * Uses a simple hand-written JSON parser to avoid external dependencies.
 * The JSON structure from the Python fetcher is well-defined and predictable,
 * so we don't need a full-featured JSON library.
 */

#include "score_reader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Minimal JSON token types */
typedef enum {
    TOK_LBRACE, TOK_RBRACE, TOK_LBRACKET, TOK_RBRACKET,
    TOK_COLON, TOK_COMMA, TOK_STRING, TOK_NUMBER,
    TOK_TRUE, TOK_FALSE, TOK_NULL, TOK_EOF, TOK_ERROR
} TokenType;

typedef struct {
    const char *src;
    int         pos;
    int         len;
    /* Current token */
    TokenType   type;
    char        str_val[512];
    double      num_val;
} JsonParser;

static void skip_ws(JsonParser *p) {
    while (p->pos < p->len) {
        char c = p->src[p->pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            p->pos++;
        else
            break;
    }
}

static void next_token(JsonParser *p) {
    skip_ws(p);
    if (p->pos >= p->len) { p->type = TOK_EOF; return; }

    char c = p->src[p->pos];
    switch (c) {
        case '{': p->type = TOK_LBRACE;   p->pos++; return;
        case '}': p->type = TOK_RBRACE;   p->pos++; return;
        case '[': p->type = TOK_LBRACKET;  p->pos++; return;
        case ']': p->type = TOK_RBRACKET;  p->pos++; return;
        case ':': p->type = TOK_COLON;     p->pos++; return;
        case ',': p->type = TOK_COMMA;     p->pos++; return;
        case '"': {
            p->pos++; /* skip opening quote */
            int out = 0;
            while (p->pos < p->len && p->src[p->pos] != '"') {
                if (p->src[p->pos] == '\\' && p->pos + 1 < p->len) {
                    p->pos++;
                    char esc = p->src[p->pos];
                    if (esc == '"' || esc == '\\' || esc == '/')
                        p->str_val[out++] = esc;
                    else if (esc == 'n') p->str_val[out++] = '\n';
                    else if (esc == 't') p->str_val[out++] = '\t';
                    else p->str_val[out++] = esc;
                } else {
                    if (out < (int)sizeof(p->str_val) - 1)
                        p->str_val[out++] = p->src[p->pos];
                }
                p->pos++;
            }
            p->str_val[out] = '\0';
            if (p->pos < p->len) p->pos++; /* skip closing quote */
            p->type = TOK_STRING;
            return;
        }
        case 't':
            if (p->pos + 3 < p->len && strncmp(p->src + p->pos, "true", 4) == 0) {
                p->pos += 4; p->type = TOK_TRUE; return;
            }
            break;
        case 'f':
            if (p->pos + 4 < p->len && strncmp(p->src + p->pos, "false", 5) == 0) {
                p->pos += 5; p->type = TOK_FALSE; return;
            }
            break;
        case 'n':
            if (p->pos + 3 < p->len && strncmp(p->src + p->pos, "null", 4) == 0) {
                p->pos += 4; p->type = TOK_NULL; return;
            }
            break;
        default:
            if (c == '-' || (c >= '0' && c <= '9')) {
                char buf[64];
                int i = 0;
                while (p->pos < p->len && i < 63) {
                    char ch = p->src[p->pos];
                    if ((ch >= '0' && ch <= '9') || ch == '-' || ch == '.' ||
                        ch == 'e' || ch == 'E' || ch == '+') {
                        buf[i++] = ch;
                        p->pos++;
                    } else break;
                }
                buf[i] = '\0';
                p->num_val = atof(buf);
                p->type = TOK_NUMBER;
                return;
            }
            break;
    }
    p->type = TOK_ERROR;
    p->pos++;
}

/* Skip a complete JSON value (object, array, string, number, etc.) */
static void skip_value(JsonParser *p) {
    if (p->type == TOK_LBRACE) {
        next_token(p);
        while (p->type != TOK_RBRACE && p->type != TOK_EOF) {
            if (p->type == TOK_STRING) next_token(p); /* key */
            if (p->type == TOK_COLON)  next_token(p);
            skip_value(p); /* value */
            if (p->type == TOK_COMMA)  next_token(p);
        }
        if (p->type == TOK_RBRACE) next_token(p);
    } else if (p->type == TOK_LBRACKET) {
        next_token(p);
        while (p->type != TOK_RBRACKET && p->type != TOK_EOF) {
            skip_value(p);
            if (p->type == TOK_COMMA) next_token(p);
        }
        if (p->type == TOK_RBRACKET) next_token(p);
    } else {
        next_token(p);
    }
}

/* Helper to copy a string safely */
static void copy_str(char *dst, const char *src, int max) {
    strncpy(dst, src, max - 1);
    dst[max - 1] = '\0';
}

/* Parse a team object */
static void parse_team(JsonParser *p, TeamData *team) {
    if (p->type != TOK_LBRACE) { skip_value(p); return; }
    next_token(p);

    team->score = -1;
    team->rank = 0;

    while (p->type == TOK_STRING) {
        char key[64];
        copy_str(key, p->str_val, sizeof(key));
        next_token(p); /* colon */
        if (p->type == TOK_COLON) next_token(p);

        if (strcmp(key, "abbreviation") == 0 && p->type == TOK_STRING)
            copy_str(team->abbreviation, p->str_val, sizeof(team->abbreviation));
        else if (strcmp(key, "name") == 0 && p->type == TOK_STRING)
            copy_str(team->name, p->str_val, sizeof(team->name));
        else if (strcmp(key, "score") == 0) {
            if (p->type == TOK_NUMBER)
                team->score = (int)p->num_val;
            /* score can be null */
        }
        else if (strcmp(key, "record") == 0 && p->type == TOK_STRING)
            copy_str(team->record, p->str_val, sizeof(team->record));
        else if (strcmp(key, "logo_url") == 0 && p->type == TOK_STRING)
            copy_str(team->logo_url, p->str_val, sizeof(team->logo_url));
        else if (strcmp(key, "rank") == 0) {
            if (p->type == TOK_NUMBER)
                team->rank = (int)p->num_val;
        }

        if (p->type != TOK_COMMA && p->type != TOK_RBRACE)
            next_token(p);
        if (p->type == TOK_COMMA) next_token(p);
    }
    if (p->type == TOK_RBRACE) next_token(p);
}

/* Parse an odds object */
static void parse_odds(JsonParser *p, OddsData *odds, int *has_odds) {
    if (p->type == TOK_NULL) {
        *has_odds = 0;
        next_token(p);
        return;
    }
    if (p->type != TOK_LBRACE) { skip_value(p); return; }

    *has_odds = 1;
    next_token(p);

    while (p->type == TOK_STRING) {
        char key[64];
        copy_str(key, p->str_val, sizeof(key));
        next_token(p);
        if (p->type == TOK_COLON) next_token(p);

        if (strcmp(key, "spread") == 0 && p->type == TOK_STRING)
            copy_str(odds->spread, p->str_val, sizeof(odds->spread));
        else if (strcmp(key, "over_under") == 0 && p->type == TOK_STRING)
            copy_str(odds->over_under, p->str_val, sizeof(odds->over_under));
        else if (strcmp(key, "moneyline") == 0 && p->type == TOK_STRING)
            copy_str(odds->moneyline, p->str_val, sizeof(odds->moneyline));

        if (p->type != TOK_COMMA && p->type != TOK_RBRACE)
            next_token(p);
        if (p->type == TOK_COMMA) next_token(p);
    }
    if (p->type == TOK_RBRACE) next_token(p);
}

/* Parse a single game object */
static void parse_game(JsonParser *p, GameData *game) {
    if (p->type != TOK_LBRACE) { skip_value(p); return; }
    next_token(p);

    game->home.score = -1;
    game->away.score = -1;

    while (p->type == TOK_STRING) {
        char key[64];
        copy_str(key, p->str_val, sizeof(key));
        next_token(p);
        if (p->type == TOK_COLON) next_token(p);

        if (strcmp(key, "game_id") == 0 && p->type == TOK_STRING)
            copy_str(game->game_id, p->str_val, sizeof(game->game_id));
        else if (strcmp(key, "sport") == 0 && p->type == TOK_STRING)
            copy_str(game->sport, p->str_val, sizeof(game->sport));
        else if (strcmp(key, "status") == 0 && p->type == TOK_STRING)
            copy_str(game->status, p->str_val, sizeof(game->status));
        else if (strcmp(key, "home_team") == 0)
            { parse_team(p, &game->home); continue; }
        else if (strcmp(key, "away_team") == 0)
            { parse_team(p, &game->away); continue; }
        else if (strcmp(key, "clock") == 0 && p->type == TOK_STRING)
            copy_str(game->clock, p->str_val, sizeof(game->clock));
        else if (strcmp(key, "period") == 0 && p->type == TOK_STRING)
            copy_str(game->period, p->str_val, sizeof(game->period));
        else if (strcmp(key, "detail") == 0 && p->type == TOK_STRING)
            copy_str(game->detail, p->str_val, sizeof(game->detail));
        else if (strcmp(key, "venue") == 0 && p->type == TOK_STRING)
            copy_str(game->venue, p->str_val, sizeof(game->venue));
        else if (strcmp(key, "location") == 0 && p->type == TOK_STRING)
            copy_str(game->location, p->str_val, sizeof(game->location));
        else if (strcmp(key, "broadcast") == 0 && p->type == TOK_STRING)
            copy_str(game->broadcast, p->str_val, sizeof(game->broadcast));
        else if (strcmp(key, "odds") == 0)
            { parse_odds(p, &game->odds, &game->has_odds); continue; }
        else if (strcmp(key, "start_time") == 0 && p->type == TOK_STRING)
            copy_str(game->start_time, p->str_val, sizeof(game->start_time));
        else {
            skip_value(p);
            continue;
        }

        next_token(p);
        if (p->type == TOK_COMMA) next_token(p);
    }
    if (p->type == TOK_RBRACE) next_token(p);
}

/* Parse the games array */
static int parse_games_array(JsonParser *p, GameData *games, int max_games) {
    if (p->type != TOK_LBRACKET) { skip_value(p); return 0; }
    next_token(p);

    int count = 0;
    while (p->type != TOK_RBRACKET && p->type != TOK_EOF && count < max_games) {
        memset(&games[count], 0, sizeof(GameData));
        parse_game(p, &games[count]);
        count++;
        if (p->type == TOK_COMMA) next_token(p);
    }
    if (p->type == TOK_RBRACKET) next_token(p);
    return count;
}

/* Parse a single sport board */
static void parse_sport_board(JsonParser *p, SportBoard *board) {
    if (p->type != TOK_LBRACE) { skip_value(p); return; }
    next_token(p);

    while (p->type == TOK_STRING) {
        char key[64];
        copy_str(key, p->str_val, sizeof(key));
        next_token(p);
        if (p->type == TOK_COLON) next_token(p);

        if (strcmp(key, "sport") == 0 && p->type == TOK_STRING) {
            copy_str(board->sport, p->str_val, sizeof(board->sport));
            next_token(p);
        } else if (strcmp(key, "games") == 0) {
            board->game_count = parse_games_array(p, board->games, MAX_GAMES);
        } else {
            skip_value(p);
            continue;
        }
        if (p->type == TOK_COMMA) next_token(p);
    }
    if (p->type == TOK_RBRACE) next_token(p);
}

int score_reader_load(const char *filepath, ScoreData *out) {
    FILE *f = fopen(filepath, "r");
    if (!f) return -1;

    /* Read entire file */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (size <= 0 || size > 1024 * 1024) { /* Max 1MB */
        fclose(f);
        return -1;
    }

    char *buf = malloc(size + 1);
    if (!buf) { fclose(f); return -1; }

    size_t read_bytes = fread(buf, 1, size, f);
    fclose(f);
    buf[read_bytes] = '\0';

    /* Parse top-level object */
    JsonParser parser = { .src = buf, .pos = 0, .len = (int)read_bytes };
    next_token(&parser);

    memset(out, 0, sizeof(*out));

    if (parser.type != TOK_LBRACE) { free(buf); return -1; }
    next_token(&parser);

    while (parser.type == TOK_STRING) {
        char key[64];
        copy_str(key, parser.str_val, sizeof(key));
        next_token(&parser);
        if (parser.type == TOK_COLON) next_token(&parser);

        if (strcmp(key, "scoreboards") == 0) {
            if (parser.type != TOK_LBRACKET) { skip_value(&parser); continue; }
            next_token(&parser);

            while (parser.type != TOK_RBRACKET && parser.type != TOK_EOF
                   && out->board_count < MAX_SPORTS) {
                parse_sport_board(&parser, &out->boards[out->board_count]);
                out->board_count++;
                if (parser.type == TOK_COMMA) next_token(&parser);
            }
            if (parser.type == TOK_RBRACKET) next_token(&parser);
        } else if (strcmp(key, "timestamp") == 0 && parser.type == TOK_NUMBER) {
            out->timestamp = parser.num_val;
            next_token(&parser);
        } else {
            skip_value(&parser);
            continue;
        }
        if (parser.type == TOK_COMMA) next_token(&parser);
    }

    free(buf);
    return 0;
}

void score_data_free(ScoreData *data) {
    /* All data is stack/embedded - just zero it out */
    memset(data, 0, sizeof(*data));
}

int score_data_total_games(const ScoreData *data) {
    int total = 0;
    for (int i = 0; i < data->board_count; i++)
        total += data->boards[i].game_count;
    return total;
}

int score_data_all_games(const ScoreData *data, const GameData **out_games, int max) {
    int count = 0;
    for (int i = 0; i < data->board_count && count < max; i++) {
        for (int j = 0; j < data->boards[i].game_count && count < max; j++) {
            out_games[count++] = &data->boards[i].games[j];
        }
    }
    return count;
}
