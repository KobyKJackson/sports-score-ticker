/*
 * ticker.h - Scrolling score ticker renderer.
 *
 * Renders a continuous scrolling strip of game cards across the 320x64 display.
 * Each game card shows: [Logo] AWAY score - score HOME | period clock | odds | venue
 */

#ifndef TICKER_H
#define TICKER_H

#include "led-matrix-c.h"
#include "score_reader.h"
#include "text_render.h"

#define MAX_TICKER_GAMES  64
#define GAME_CARD_GAP     10   /* Pixels between game cards */
#define SPORT_DIVIDER_W   20   /* Width of sport label divider */

/* Color constants using TickerColor from text_render.h */
static const TickerColor COLOR_WHITE   = {255, 255, 255};
static const TickerColor COLOR_YELLOW  = {255, 255, 0};
static const TickerColor COLOR_GREEN   = {0, 255, 0};
static const TickerColor COLOR_RED     = {255, 0, 0};
static const TickerColor COLOR_CYAN    = {0, 255, 255};
static const TickerColor COLOR_GRAY    = {128, 128, 128};
static const TickerColor COLOR_ORANGE  = {255, 165, 0};
static const TickerColor COLOR_DIM     = {80, 80, 80};

/* Sport header colors */
static const TickerColor COLOR_NBA     = {29, 66, 138};
static const TickerColor COLOR_NFL     = {1, 51, 105};
static const TickerColor COLOR_MLB     = {0, 45, 98};
static const TickerColor COLOR_NHL     = {0, 0, 0};
static const TickerColor COLOR_NCAAF   = {128, 0, 0};
static const TickerColor COLOR_NCAAM   = {0, 100, 0};

/* A rendered game card in the ticker strip */
typedef struct {
    const GameData *game;
    int total_width;     /* Total pixel width of this card */
} TickerCard;

typedef struct {
    int scroll_x;        /* Current scroll offset (pixels) */
    int scroll_speed;    /* Pixels per frame */
    int display_width;   /* Canvas width */
    int display_height;  /* Canvas height */

    TickerCard cards[MAX_TICKER_GAMES];
    int card_count;
    int total_strip_width;  /* Total width of all cards + gaps */

    /* Sport separator tracking */
    char last_sport[16];
} TickerState;

/* Initialize the ticker */
void ticker_init(TickerState *t, int width, int height, int speed);

/* Update the ticker's game list from new score data */
void ticker_update_games(TickerState *t, const ScoreData *data);

/* Render one frame of the ticker to the canvas */
void ticker_render(const TickerState *t, struct LedCanvas *canvas,
                   struct LedFont *font, struct LedFont *large_font);

/* Advance the scroll position by one frame */
void ticker_advance(TickerState *t);

/* Cleanup */
void ticker_cleanup(TickerState *t);

#endif /* TICKER_H */
