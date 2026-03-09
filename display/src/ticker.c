/*
 * ticker.c - Scrolling score ticker renderer.
 *
 * Layout on 320x64 display (2 rows of 5 P4 panels):
 *
 * Each game card uses the full 64px height with two lines of text:
 *   Line 1 (top): [Logo] AWAY score - score HOME
 *   Line 2 (bottom): period clock | odds | venue
 *
 * Cards scroll continuously left-to-right with sport dividers between sections.
 */

#include "ticker.h"
#include "logo_cache.h"

#include <string.h>
#include <stdio.h>

/* Calculate the pixel width of a game card */
static int calc_card_width(const GameData *game) {
    int w = 0;
    w += 28;  /* logo space */
    w += 4;   /* gap */

    /* Team abbreviations (~6px per char) */
    w += strlen(game->away.abbreviation) * 6 + 4;

    /* Scores */
    if (game->home.score >= 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", game->away.score);
        w += strlen(buf) * 6 + 4;
        w += 7; /* " - " */
        snprintf(buf, sizeof(buf), "%d", game->home.score);
        w += strlen(buf) * 6 + 4;
    }

    w += strlen(game->home.abbreviation) * 6 + 8;

    /* Status/clock */
    if (game->detail[0]) {
        w += strlen(game->detail) * 6 + 8;
    }

    /* Odds */
    if (game->has_odds && game->odds.spread[0]) {
        w += strlen(game->odds.spread) * 6 + 4;
        if (game->odds.over_under[0])
            w += strlen(game->odds.over_under) * 6 + 8;
    }

    /* Venue */
    if (game->venue[0]) {
        int vlen = strlen(game->venue);
        if (vlen > 24) vlen = 24;
        w += vlen * 6 + 8;
    }

    w += GAME_CARD_GAP;
    return w;
}

void ticker_init(TickerState *t, int width, int height, int speed) {
    memset(t, 0, sizeof(*t));
    t->display_width = width;
    t->display_height = height;
    t->scroll_speed = speed > 0 ? speed : 1;
    t->scroll_x = 0;
}

void ticker_update_games(TickerState *t, const ScoreData *data) {
    const GameData *all_games[MAX_TICKER_GAMES];
    int count = score_data_all_games(data, all_games, MAX_TICKER_GAMES);

    t->card_count = 0;
    t->total_strip_width = 0;

    char prev_sport[16] = "";
    for (int i = 0; i < count && t->card_count < MAX_TICKER_GAMES; i++) {
        if (strcmp(all_games[i]->sport, prev_sport) != 0) {
            t->total_strip_width += SPORT_DIVIDER_W;
            strncpy(prev_sport, all_games[i]->sport, sizeof(prev_sport) - 1);
        }

        TickerCard *card = &t->cards[t->card_count];
        card->game = all_games[i];
        card->total_width = calc_card_width(all_games[i]);

        t->total_strip_width += card->total_width;
        t->card_count++;
    }

    if (t->total_strip_width < t->display_width) {
        t->total_strip_width = t->display_width;
    }
}

static TickerColor sport_color(const char *sport) {
    if (strcmp(sport, "nba") == 0)   return COLOR_NBA;
    if (strcmp(sport, "nfl") == 0)   return COLOR_NFL;
    if (strcmp(sport, "mlb") == 0)   return COLOR_MLB;
    if (strcmp(sport, "nhl") == 0)   return COLOR_NHL;
    if (strcmp(sport, "ncaaf") == 0) return COLOR_NCAAF;
    if (strcmp(sport, "ncaam") == 0) return COLOR_NCAAM;
    return COLOR_DIM;
}

static const char *sport_label(const char *sport) {
    if (strcmp(sport, "nba") == 0)   return "NBA";
    if (strcmp(sport, "nfl") == 0)   return "NFL";
    if (strcmp(sport, "mlb") == 0)   return "MLB";
    if (strcmp(sport, "nhl") == 0)   return "NHL";
    if (strcmp(sport, "ncaaf") == 0) return "CFB";
    if (strcmp(sport, "ncaam") == 0) return "CBB";
    return "???";
}

static TickerColor status_color(const char *status) {
    if (strcmp(status, "in_progress") == 0) return COLOR_GREEN;
    if (strcmp(status, "halftime") == 0)    return COLOR_YELLOW;
    if (strcmp(status, "final") == 0)       return COLOR_RED;
    if (strcmp(status, "delayed") == 0)     return COLOR_ORANGE;
    return COLOR_GRAY;
}

/* Draw a sport divider bar */
static void draw_sport_divider(struct LedCanvas *canvas, struct LedFont *font,
                               int x, const char *sport) {
    TickerColor bg = sport_color(sport);
    const char *label = sport_label(sport);

    /* Draw colored background bar */
    for (int dy = 0; dy < 64; dy++) {
        for (int dx = 0; dx < SPORT_DIVIDER_W; dx++) {
            led_canvas_set_pixel(canvas, x + dx, dy, bg.r, bg.g, bg.b);
        }
    }

    /* Draw sport label vertically centered */
    ticker_draw_text(canvas, font, x + 2, 36, COLOR_WHITE, label);
}

/* Render a single game card at the given x offset */
static int render_game_card(const TickerCard *card, struct LedCanvas *canvas,
                            struct LedFont *font, struct LedFont *large_font,
                            int x_start) {
    const GameData *g = card->game;
    int x = x_start;

    /* --- Logo area (28x28 at top-left of card) --- */
    logo_draw(canvas, g->away.abbreviation, g->sport, x, 2, 28);
    x += 32;

    /* --- Line 1: Teams and Scores (large font, y baseline ~20) --- */
    int line1_y = 20;
    struct LedFont *score_font = large_font ? large_font : font;

    /* Away team rank (college) */
    if (g->away.rank > 0) {
        char rank_str[8];
        snprintf(rank_str, sizeof(rank_str), "#%d", g->away.rank);
        x = ticker_draw_text(canvas, font, x, line1_y - 4, COLOR_YELLOW, rank_str);
        x += 2;
    }

    /* Away abbreviation */
    x = ticker_draw_text(canvas, score_font, x, line1_y, COLOR_WHITE,
                          g->away.abbreviation);
    x += 4;

    /* Scores */
    if (g->home.score >= 0 && g->away.score >= 0) {
        char score_buf[32];
        int away_winning = g->away.score > g->home.score;
        int home_winning = g->home.score > g->away.score;

        snprintf(score_buf, sizeof(score_buf), "%d", g->away.score);
        TickerColor sc = away_winning ? COLOR_WHITE : COLOR_GRAY;
        x = ticker_draw_text(canvas, score_font, x, line1_y, sc, score_buf);
        x += 3;

        x = ticker_draw_text(canvas, score_font, x, line1_y, COLOR_DIM, "-");
        x += 3;

        snprintf(score_buf, sizeof(score_buf), "%d", g->home.score);
        sc = home_winning ? COLOR_WHITE : COLOR_GRAY;
        x = ticker_draw_text(canvas, score_font, x, line1_y, sc, score_buf);
        x += 4;
    }

    /* Home team rank (college) */
    if (g->home.rank > 0) {
        char rank_str[8];
        snprintf(rank_str, sizeof(rank_str), "#%d", g->home.rank);
        x = ticker_draw_text(canvas, font, x, line1_y - 4, COLOR_YELLOW, rank_str);
        x += 2;
    }

    /* Home abbreviation */
    x = ticker_draw_text(canvas, score_font, x, line1_y, COLOR_WHITE,
                          g->home.abbreviation);
    x += 6;

    /* --- Line 2: Status, Odds, Venue (small font, y baseline ~48) --- */
    int line2_x = x_start + 32;
    int line2_y = 48;

    TickerColor stc = status_color(g->status);

    if (strcmp(g->status, "in_progress") == 0 || strcmp(g->status, "halftime") == 0) {
        if (g->period[0]) {
            line2_x = ticker_draw_text(canvas, font, line2_x, line2_y, stc, g->period);
            line2_x += 4;
        }
        if (g->clock[0]) {
            line2_x = ticker_draw_text(canvas, font, line2_x, line2_y, COLOR_WHITE, g->clock);
            line2_x += 8;
        }
        if (strcmp(g->status, "halftime") == 0) {
            line2_x = ticker_draw_text(canvas, font, line2_x, line2_y, COLOR_YELLOW, "HALF");
            line2_x += 8;
        }
    } else if (strcmp(g->status, "final") == 0) {
        line2_x = ticker_draw_text(canvas, font, line2_x, line2_y, COLOR_RED, "FINAL");
        line2_x += 8;
    } else if (strcmp(g->status, "scheduled") == 0) {
        if (g->detail[0]) {
            line2_x = ticker_draw_text(canvas, font, line2_x, line2_y, COLOR_CYAN, g->detail);
            line2_x += 8;
        }
    } else if (g->detail[0]) {
        line2_x = ticker_draw_text(canvas, font, line2_x, line2_y, stc, g->detail);
        line2_x += 8;
    }

    /* Betting odds */
    if (g->has_odds) {
        ticker_draw_text(canvas, font, line2_x - 4, line2_y, COLOR_DIM, "|");

        if (g->odds.spread[0]) {
            line2_x = ticker_draw_text(canvas, font, line2_x, line2_y, COLOR_ORANGE,
                                        g->odds.spread);
            line2_x += 4;
        }
        if (g->odds.over_under[0]) {
            line2_x = ticker_draw_text(canvas, font, line2_x, line2_y, COLOR_ORANGE,
                                        g->odds.over_under);
            line2_x += 8;
        }
    }

    /* Venue */
    if (g->venue[0]) {
        ticker_draw_text(canvas, font, line2_x - 4, line2_y, COLOR_DIM, "|");
        char venue_buf[64];
        strncpy(venue_buf, g->venue, 24);
        venue_buf[24] = '\0';
        line2_x = ticker_draw_text(canvas, font, line2_x, line2_y, COLOR_GRAY, venue_buf);
    }

    int max_x = x > line2_x ? x : line2_x;
    return max_x - x_start + GAME_CARD_GAP;
}

void ticker_render(const TickerState *t, struct LedCanvas *canvas,
                   struct LedFont *font, struct LedFont *large_font) {
    if (t->card_count == 0) {
        ticker_draw_text(canvas, large_font ? large_font : font, 40, 36,
                          COLOR_CYAN, "No games right now");
        return;
    }

    /*
     * Render the ticker strip with wrapping for seamless looping.
     * We render the strip twice: once at the normal position and once
     * shifted by total_strip_width so there's no gap.
     */
    int strip_x = -t->scroll_x;
    char prev_sport[16] = "";

    for (int pass = 0; pass < 2; pass++) {
        int cx = strip_x;
        memset((void *)prev_sport, 0, sizeof(prev_sport));

        for (int i = 0; i < t->card_count; i++) {
            const TickerCard *card = &t->cards[i];

            /* Sport divider */
            if (strcmp(card->game->sport, prev_sport) != 0) {
                if (cx + SPORT_DIVIDER_W > 0 && cx < t->display_width) {
                    draw_sport_divider(canvas, font, cx, card->game->sport);
                }
                cx += SPORT_DIVIDER_W;
                strncpy((char *)prev_sport, card->game->sport, sizeof(prev_sport) - 1);
            }

            /* Only render if visible on screen */
            if (cx + card->total_width > 0 && cx < t->display_width) {
                render_game_card(card, canvas, font, large_font, cx);
            }
            cx += card->total_width;
        }

        strip_x += t->total_strip_width;
    }
}

void ticker_advance(TickerState *t) {
    t->scroll_x += t->scroll_speed;
    if (t->scroll_x >= t->total_strip_width) {
        t->scroll_x -= t->total_strip_width;
    }
}

void ticker_cleanup(TickerState *t) {
    memset(t, 0, sizeof(*t));
}
