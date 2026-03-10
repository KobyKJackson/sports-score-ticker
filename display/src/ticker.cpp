// ticker.cpp - Scrolling score ticker renderer.
//
// Layout on 320x64 display (2 rows of 5 P4 panels):
//   Line 1 (top half):    [Logo] AWAY score - score HOME
//   Line 2 (bottom half): period clock | odds | venue

#include "ticker.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

using rgb_matrix::Canvas;
using rgb_matrix::Color;
using rgb_matrix::DrawText;
using rgb_matrix::Font;

// Color constants
const Color Ticker::WHITE  {255, 255, 255};
const Color Ticker::YELLOW {255, 255, 0};
const Color Ticker::GREEN  {0,   255, 0};
const Color Ticker::RED    {255, 0,   0};
const Color Ticker::CYAN   {0,   255, 255};
const Color Ticker::GRAY   {128, 128, 128};
const Color Ticker::ORANGE {255, 165, 0};
const Color Ticker::DIM    {80,  80,  80};

Ticker::Ticker(int display_width, int display_height, int scroll_speed, LogoCache &logos)
    : scroll_speed_(std::max(1, scroll_speed))
    , display_width_(display_width)
    , display_height_(display_height)
    , logos_(logos) {}

int Ticker::calc_card_width(const Game &g) const {
    int w = LOGO_SIZE + 4; // logo + gap

    w += static_cast<int>(g.away.abbreviation.size()) * 6 + 4;

    if (g.home.has_score()) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d", g.away.score);
        w += static_cast<int>(std::strlen(buf)) * 6 + 4;
        w += 7; // " - "
        std::snprintf(buf, sizeof(buf), "%d", g.home.score);
        w += static_cast<int>(std::strlen(buf)) * 6 + 4;
    }

    w += static_cast<int>(g.home.abbreviation.size()) * 6 + 8;

    if (!g.detail.empty())
        w += static_cast<int>(g.detail.size()) * 6 + 8;

    if (g.odds && !g.odds->spread.empty()) {
        w += static_cast<int>(g.odds->spread.size()) * 6 + 4;
        if (!g.odds->over_under.empty())
            w += static_cast<int>(g.odds->over_under.size()) * 6 + 8;
    }

    if (!g.venue.empty()) {
        int vlen = std::min(static_cast<int>(g.venue.size()), 24);
        w += vlen * 6 + 8;
    }

    w += GAME_CARD_GAP;
    return w;
}

void Ticker::update_games(const ScoreData &data) {
    game_ptrs_ = data.all_games();
    cards_.clear();
    total_strip_width_ = 0;

    std::string prev_sport;
    for (auto *game : game_ptrs_) {
        if (game->sport != prev_sport) {
            total_strip_width_ += SPORT_DIVIDER_W;
            prev_sport = game->sport;
        }

        TickerCard card;
        card.game = game;
        card.total_width = calc_card_width(*game);
        total_strip_width_ += card.total_width;
        cards_.push_back(card);
    }

    if (total_strip_width_ < display_width_)
        total_strip_width_ = display_width_;
}

Color Ticker::sport_color(const std::string &sport) {
    if (sport == "nba")   return {29, 66, 138};
    if (sport == "nfl")   return {1, 51, 105};
    if (sport == "mlb")   return {0, 45, 98};
    if (sport == "nhl")   return {0, 0, 0};
    if (sport == "ncaaf") return {128, 0, 0};
    if (sport == "ncaam") return {0, 100, 0};
    return DIM;
}

const char *Ticker::sport_label(const std::string &sport) {
    if (sport == "nba")   return "NBA";
    if (sport == "nfl")   return "NFL";
    if (sport == "mlb")   return "MLB";
    if (sport == "nhl")   return "NHL";
    if (sport == "ncaaf") return "CFB";
    if (sport == "ncaam") return "CBB";
    return "???";
}

Color Ticker::status_color(const std::string &status) {
    if (status == "in_progress") return GREEN;
    if (status == "halftime")    return YELLOW;
    if (status == "final")       return RED;
    if (status == "delayed")     return ORANGE;
    return GRAY;
}

void Ticker::draw_sport_divider(Canvas *canvas, const Font &font,
                                int x, const std::string &sport) const {
    Color bg = sport_color(sport);

    // Draw colored background bar
    for (int dy = 0; dy < 64; ++dy)
        for (int dx = 0; dx < SPORT_DIVIDER_W; ++dx)
            canvas->SetPixel(x + dx, dy, bg.r, bg.g, bg.b);

    // Draw sport label vertically centered
    DrawText(canvas, font, x + 2, 36, WHITE, sport_label(sport));
}

int Ticker::render_game_card(const TickerCard &card, Canvas *canvas,
                             const Font &font, const Font &large_font,
                             int x_start) const {
    const Game &g = *card.game;
    int x = x_start;

    // --- Logo (28x28 at top-left) ---
    logos_.draw(canvas, g.away.abbreviation, g.sport, x, 2, LOGO_SIZE);
    x += LOGO_SIZE + 4;

    // --- Line 1: Teams and Scores (large font, baseline y=20) ---
    constexpr int line1_y = 20;

    // Away rank (college)
    if (g.away.rank > 0) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "#%d", g.away.rank);
        x = DrawText(canvas, font, x, line1_y - 4, YELLOW, buf);
        x += 2;
    }

    // Away abbreviation
    x = DrawText(canvas, large_font, x, line1_y, WHITE, g.away.abbreviation.c_str());
    x += 4;

    // Scores
    if (g.home.has_score() && g.away.has_score()) {
        char buf[16];
        bool away_winning = g.away.score > g.home.score;
        bool home_winning = g.home.score > g.away.score;

        std::snprintf(buf, sizeof(buf), "%d", g.away.score);
        x = DrawText(canvas, large_font, x, line1_y, away_winning ? WHITE : GRAY, buf);
        x += 3;

        x = DrawText(canvas, large_font, x, line1_y, DIM, "-");
        x += 3;

        std::snprintf(buf, sizeof(buf), "%d", g.home.score);
        x = DrawText(canvas, large_font, x, line1_y, home_winning ? WHITE : GRAY, buf);
        x += 4;
    }

    // Home rank
    if (g.home.rank > 0) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "#%d", g.home.rank);
        x = DrawText(canvas, font, x, line1_y - 4, YELLOW, buf);
        x += 2;
    }

    // Home abbreviation
    x = DrawText(canvas, large_font, x, line1_y, WHITE, g.home.abbreviation.c_str());
    x += 6;

    // --- Line 2: Status, Odds, Venue (small font, baseline y=48) ---
    int line2_x = x_start + LOGO_SIZE + 4;
    constexpr int line2_y = 48;

    Color stc = status_color(g.status);

    if (g.is_live()) {
        if (!g.period.empty()) {
            line2_x = DrawText(canvas, font, line2_x, line2_y, stc, g.period.c_str());
            line2_x += 4;
        }
        if (!g.clock.empty()) {
            line2_x = DrawText(canvas, font, line2_x, line2_y, WHITE, g.clock.c_str());
            line2_x += 8;
        }
        if (g.status == "halftime") {
            line2_x = DrawText(canvas, font, line2_x, line2_y, YELLOW, "HALF");
            line2_x += 8;
        }
    } else if (g.is_final()) {
        line2_x = DrawText(canvas, font, line2_x, line2_y, RED, "FINAL");
        line2_x += 8;
    } else if (g.is_scheduled()) {
        if (!g.detail.empty()) {
            line2_x = DrawText(canvas, font, line2_x, line2_y, CYAN, g.detail.c_str());
            line2_x += 8;
        }
    } else if (!g.detail.empty()) {
        line2_x = DrawText(canvas, font, line2_x, line2_y, stc, g.detail.c_str());
        line2_x += 8;
    }

    // Betting odds
    if (g.odds) {
        DrawText(canvas, font, line2_x - 4, line2_y, DIM, "|");
        if (!g.odds->spread.empty()) {
            line2_x = DrawText(canvas, font, line2_x, line2_y, ORANGE, g.odds->spread.c_str());
            line2_x += 4;
        }
        if (!g.odds->over_under.empty()) {
            line2_x = DrawText(canvas, font, line2_x, line2_y, ORANGE,
                               g.odds->over_under.c_str());
            line2_x += 8;
        }
    }

    // Venue
    if (!g.venue.empty()) {
        DrawText(canvas, font, line2_x - 4, line2_y, DIM, "|");
        std::string venue_short = g.venue.substr(0, 24);
        line2_x = DrawText(canvas, font, line2_x, line2_y, GRAY, venue_short.c_str());
    }

    return std::max(x, line2_x) - x_start + GAME_CARD_GAP;
}

void Ticker::render(Canvas *canvas, const Font &font, const Font &large_font) const {
    if (cards_.empty()) {
        DrawText(canvas, large_font, 40, 36, CYAN, "No games right now");
        return;
    }

    // Render the strip twice for seamless looping
    int strip_x = -scroll_x_;

    for (int pass = 0; pass < 2; ++pass) {
        int cx = strip_x;
        std::string prev_sport;

        for (auto &card : cards_) {
            // Sport divider
            if (card.game->sport != prev_sport) {
                if (cx + SPORT_DIVIDER_W > 0 && cx < display_width_)
                    draw_sport_divider(canvas, font, cx, card.game->sport);
                cx += SPORT_DIVIDER_W;
                prev_sport = card.game->sport;
            }

            // Only render if visible
            if (cx + card.total_width > 0 && cx < display_width_)
                render_game_card(card, canvas, font, large_font, cx);
            cx += card.total_width;
        }

        strip_x += total_strip_width_;
    }
}

void Ticker::advance() {
    scroll_x_ += scroll_speed_;
    if (scroll_x_ >= total_strip_width_)
        scroll_x_ -= total_strip_width_;
}
