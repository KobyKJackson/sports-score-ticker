/*
 * score_reader.h - Reads and parses the JSON scores file written by the Python fetcher.
 */

#ifndef SCORE_READER_H
#define SCORE_READER_H

#define MAX_GAMES       64
#define MAX_SPORTS      8
#define MAX_STR_LEN     128
#define MAX_ABBREV_LEN  8

typedef struct {
    char abbreviation[MAX_ABBREV_LEN];
    char name[MAX_STR_LEN];
    int  score;          /* -1 if no score yet */
    char record[32];
    char logo_url[256];
    int  rank;           /* 0 = unranked */
} TeamData;

typedef struct {
    char spread[64];
    char over_under[64];
    char moneyline[64];
} OddsData;

typedef struct {
    char     game_id[32];
    char     sport[16];
    char     status[32];       /* scheduled, in_progress, halftime, final, delayed, postponed */
    TeamData home;
    TeamData away;
    char     clock[32];
    char     period[16];
    char     detail[MAX_STR_LEN];
    char     venue[MAX_STR_LEN];
    char     location[MAX_STR_LEN];
    char     broadcast[32];
    OddsData odds;
    int      has_odds;
    char     start_time[64];
} GameData;

typedef struct {
    char     sport[16];
    GameData games[MAX_GAMES];
    int      game_count;
} SportBoard;

typedef struct {
    SportBoard boards[MAX_SPORTS];
    int        board_count;
    double     timestamp;
} ScoreData;

/*
 * Load and parse the JSON score file.
 * Returns 0 on success, -1 on error.
 * Caller must call score_data_free() when done.
 */
int score_reader_load(const char *filepath, ScoreData *out);

/*
 * Free any allocated memory in a ScoreData struct.
 */
void score_data_free(ScoreData *data);

/*
 * Get total number of games across all sports.
 */
int score_data_total_games(const ScoreData *data);

/*
 * Get a flat list of all games (pointers into the ScoreData).
 * Returns count. out_games must have space for MAX_GAMES pointers.
 */
int score_data_all_games(const ScoreData *data, const GameData **out_games, int max);

#endif /* SCORE_READER_H */
