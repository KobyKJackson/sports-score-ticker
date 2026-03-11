// ticker.cpp - Scrolling score ticker renderer.
//
// Layout per game card (three text rows, two logos):
//   Row 1 (y≈24): [Away logo] AWAY_ABBREV score @ score HOME_ABBREV [Home logo]
//   Row 2 (y≈43): period + clock  |  "FINAL"  |  scheduled date
//   Row 3 (y≈56): betting odds (spread + over/under)
//
// The strip is rendered twice per frame so the seam is invisible when looping.

#include "ticker.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

using rgb_matrix::Canvas;
using rgb_matrix::Color;
using rgb_matrix::DrawText;
using rgb_matrix::Font;

// ── Color palette ────────────────────────────────────────────────────────────
const Color Ticker::WHITE{255, 255, 255};
const Color Ticker::YELLOW{255, 255, 0};
const Color Ticker::GREEN{0, 255, 0};
const Color Ticker::RED{255, 0, 0};
const Color Ticker::CYAN{0, 255, 255};
const Color Ticker::GRAY{128, 128, 128};
const Color Ticker::ORANGE{255, 165, 0};
const Color Ticker::DIM{80, 80, 80};

// ── Constructor ──────────────────────────────────────────────────────────────

Ticker::Ticker(int display_width, int display_height, int scroll_speed, LogoCache &logos)
    : scroll_speed_(std::max(1, scroll_speed)) // enforce at least 1 px/4-frames
      ,
      display_width_(display_width), display_height_(display_height), logo_size_(display_height - 4) // logos fill the full panel height with 2px margin
      ,
      logo_gap_(logo_size_) // text starts 4px after logo right edge
      ,
      logos_(logos)
{
}

// ── Text measurement ─────────────────────────────────────────────────────────

// Sum the pixel-advance width of each character in str using the given font.
int Ticker::measure_text(const Font &font, const std::string &str)
{
    int w = 0;
    for (unsigned char c : str)
        w += font.CharacterWidth(c);
    return w;
}

// ── Card width calculation ───────────────────────────────────────────────────

int Ticker::calc_card_width(const Game &g, const Font &large_font, int &row1_w_out) const
{
    // Row 1: exact pixel width using font character metrics (no estimates needed).
    int row1_w = 0;
    if (g.away.rank > 0)
    {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "#%d", g.away.rank);
        row1_w += static_cast<int>(std::strlen(buf)) * SMALL_FONT_W + 2;
    }
    // measure_row1: exact width including extra inter-character advance
    auto measure_row1 = [&](const std::string &s)
    {
        return measure_text(large_font, s) + LARGE_EXTRA_ADVANCE * static_cast<int>(s.size());
    };
    row1_w += measure_row1(g.away.abbreviation) + 4;
    if (!g.is_scheduled() && g.home.has_score() && g.away.has_score())
    {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d", g.away.score);
        row1_w += measure_row1(buf) + 3;
        row1_w += large_font.CharacterWidth('@') + LARGE_EXTRA_ADVANCE + 6;
        std::snprintf(buf, sizeof(buf), "%d", g.home.score);
        row1_w += measure_row1(buf) + 4;
    }
    else
    {
        row1_w += large_font.CharacterWidth('@') + LARGE_EXTRA_ADVANCE + 6;
    }
    if (g.home.rank > 0)
    {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "#%d", g.home.rank);
        row1_w += static_cast<int>(std::strlen(buf)) * SMALL_FONT_W + 2;
    }
    row1_w += measure_row1(g.home.abbreviation) + 8;

    // Row 2: status / scheduled time — may be wider than row 1 (fixed-width font, estimates ok)
    int row2_w = 0;
    if (g.is_live())
    {
        int live_w = 0;
        if (!g.period.empty())
            live_w += static_cast<int>(g.period.size()) * SMALL_FONT_W + 4;
        if (!g.clock.empty())
            live_w += static_cast<int>(g.clock.size()) * SMALL_FONT_W;
        if (g.status == "halftime")
            live_w += 4 + 4 * SMALL_FONT_W;
        row2_w = live_w;
    }
    else if (g.is_final())
    {
        row2_w = 5 * SMALL_FONT_W; // "FINAL"
    }
    else if (g.is_scheduled() && !g.detail.empty())
    {
        row2_w = static_cast<int>(g.detail.size()) * TINY_FONT_W;
    }
    else if (!g.detail.empty())
    {
        row2_w = static_cast<int>(g.detail.size()) * SMALL_FONT_W;
    }

    // Row 3: odds (fixed-width font)
    int row3_w = 0;
    if (g.odds)
    {
        std::string odds_str;
        if (!g.odds->spread.empty())
            odds_str += g.odds->spread;
        if (!g.odds->over_under.empty())
            odds_str += (odds_str.empty() ? "" : "  ") + g.odds->over_under;
        if (!odds_str.empty())
            row3_w = static_cast<int>(odds_str.size()) * SMALL_FONT_W;
    }

    // Card text area is wide enough for the widest row; logos sit outside this area.
    row1_w_out = row1_w;
    int text_w = std::max({row1_w, row2_w, row3_w});
    return logo_gap_ + text_w + logo_gap_ + GAME_CARD_GAP;
}

// ── Game list update ─────────────────────────────────────────────────────────

void Ticker::update_games(const ScoreData &data, const Font &large_font)
{
    // Snapshot game pointers so cards_ stays valid until the next update.
    game_ptrs_ = data.all_games();
    cards_.clear();
    total_strip_width_ = 0;

    std::string prev_sport;
    for (auto *game : game_ptrs_)
    {
        // Insert a sport-divider bar each time the sport changes.
        if (game->sport != prev_sport)
        {
            total_strip_width_ += SPORT_DIVIDER_W;
            prev_sport = game->sport;
        }

        TickerCard card;
        card.game = game;
        card.total_width = calc_card_width(*game, large_font, card.row1_width); // exact width via font metrics
        total_strip_width_ += card.total_width;
        cards_.push_back(card);
    }

    // Ensure the strip is at least as wide as the display to prevent blank frames.
    if (total_strip_width_ < display_width_)
        total_strip_width_ = display_width_;
}

// ── Sport metadata ───────────────────────────────────────────────────────────

// Background color for each sport's divider bar.
Color Ticker::sport_color(const std::string &sport)
{
    if (sport == "nba")
        return {29, 66, 138}; // NBA blue
    if (sport == "nfl")
        return {1, 51, 105}; // NFL blue
    if (sport == "mlb")
        return {0, 45, 98}; // MLB navy
    if (sport == "nhl")
        return {0, 0, 0}; // NHL black
    if (sport == "ncaaf")
        return {128, 0, 0}; // CFB crimson
    if (sport == "ncaam")
        return {0, 100, 0}; // CBB green
    return DIM;
}

// Short label displayed on the divider bar.
const char *Ticker::sport_label(const std::string &sport)
{
    if (sport == "nba")
        return "NBA";
    if (sport == "nfl")
        return "NFL";
    if (sport == "mlb")
        return "MLB";
    if (sport == "nhl")
        return "NHL";
    if (sport == "ncaaf")
        return "CFB";
    if (sport == "ncaam")
        return "CBB";
    return "???";
}

// Color used for the game status text (period, clock, "FINAL", etc.).
Color Ticker::status_color(const std::string &status)
{
    if (status == "in_progress")
        return GREEN;
    if (status == "halftime")
        return YELLOW;
    if (status == "final")
        return RED;
    if (status == "delayed")
        return ORANGE;
    return GRAY; // scheduled, unknown
}

// ── Sport divider ────────────────────────────────────────────────────────────

void Ticker::draw_sport_divider(Canvas *canvas, const Font &font,
                                int x, const std::string &sport) const
{
    Color bg = sport_color(sport);

    // Flood-fill a SPORT_DIVIDER_W × display_height_ colored rectangle.
    for (int dy = 0; dy < display_height_; ++dy)
        for (int dx = 0; dx < SPORT_DIVIDER_W; ++dx)
            canvas->SetPixel(x + dx, dy, bg.r, bg.g, bg.b);

    // Sport abbreviation label, baseline at y=36 (vertically centered on 64px display).
    DrawText(canvas, font, x + 2, 36, WHITE, sport_label(sport));
}

// ── Game card rendering ──────────────────────────────────────────────────────

int Ticker::render_game_card(const TickerCard &card, Canvas *canvas,
                             const Font &font, const Font &large_font,
                             const Font &sched_font, int x_start) const
{
    const Game &g = *card.game;
    int x = x_start;

    // Away logo: right-aligned in its box so the logo's right edge is flush against the text.
    logos_.draw(canvas, g.away.abbreviation, g.sport, x, 2, logo_size_, /*right_align=*/true);
    x += logo_gap_;

    // Derive text area bounds from pre-computed card width and logo_gap_.
    const int text_start_x = x;
    const int text_w = card.total_width - 2 * logo_gap_ - GAME_CARD_GAP;
    const int text_end_x = text_start_x + text_w;
    const int text_mid = (text_start_x + text_end_x) / 2; // center point for row 2 & 3

    // ── Row 1: team abbreviations + scores ───────────────────────────────────
    constexpr int line1_y = 24; // baseline y for large font (10x20)

    // Center row 1 within the text area using the pre-computed row-1 pixel width.
    x = text_start_x + (text_w - card.row1_width) / 2;

    // Away rank badge (college), drawn smaller and raised above the abbreviation baseline.
    if (g.away.rank > 0)
    {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "#%d", g.away.rank);
        x += DrawText(canvas, font, x, line1_y - 5, YELLOW, buf);
        x += 2;
    }

    // Away team abbreviation (extra_advance widens each glyph for a ~12px-wide feel).
    x += DrawText(canvas, large_font, x, line1_y, WHITE, nullptr, g.away.abbreviation.c_str(), LARGE_EXTRA_ADVANCE);
    x += 4;

    if (!g.is_scheduled() && g.home.has_score() && g.away.has_score())
    {
        // Draw scores; winning team shown in white, losing team dimmed.
        char buf[16];
        bool away_winning = g.away.score > g.home.score;
        bool home_winning = g.home.score > g.away.score;

        std::snprintf(buf, sizeof(buf), "%d", g.away.score);
        x += DrawText(canvas, large_font, x, line1_y, away_winning ? WHITE : GRAY, nullptr, buf, LARGE_EXTRA_ADVANCE);
        x += 3;

        x += DrawText(canvas, large_font, x, line1_y, DIM, nullptr, "@", LARGE_EXTRA_ADVANCE);
        x += 3;

        std::snprintf(buf, sizeof(buf), "%d", g.home.score);
        x += DrawText(canvas, large_font, x, line1_y, home_winning ? WHITE : GRAY, nullptr, buf, LARGE_EXTRA_ADVANCE);
        x += 4;
    }
    else
    {
        // Scheduled game — no score, just show "@".
        x += DrawText(canvas, large_font, x, line1_y, DIM, nullptr, "@", LARGE_EXTRA_ADVANCE);
        x += 4;
    }

    // Home rank badge (college).
    if (g.home.rank > 0)
    {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "#%d", g.home.rank);
        x += DrawText(canvas, font, x, line1_y - 5, YELLOW, buf);
        x += 2;
    }

    // Home team abbreviation.
    x += DrawText(canvas, large_font, x, line1_y, WHITE, nullptr, g.home.abbreviation.c_str(), LARGE_EXTRA_ADVANCE);

    // ── Row 2: game status / scheduled time ──────────────────────────────────
    constexpr int line2_y = 43; // baseline y for small font (7x13B)
    Color stc = status_color(g.status);

    if (g.is_live())
    {
        // Build the full "Q3 4:32" or "HALF" string to measure it for centering.
        std::string live_str;
        if (!g.period.empty())
            live_str += g.period;
        if (!g.period.empty() && !g.clock.empty())
            live_str += " ";
        if (!g.clock.empty())
            live_str += g.clock;
        if (g.status == "halftime")
            live_str += live_str.empty() ? "HALF" : " HALF";

        int total_w = static_cast<int>(live_str.size()) * SMALL_FONT_W;
        int cx = text_mid - total_w / 2; // center the group

        if (!g.period.empty())
        {
            cx += DrawText(canvas, font, cx, line2_y, stc, g.period.c_str());
            if (!g.clock.empty())
                cx += 4; // gap between period and clock
        }
        if (!g.clock.empty())
        {
            cx += DrawText(canvas, font, cx, line2_y, WHITE, g.clock.c_str());
        }
        if (g.status == "halftime")
        {
            cx += 4;
            DrawText(canvas, font, cx, line2_y, YELLOW, "HALF");
        }
    }
    else if (g.is_final())
    {
        // "FINAL" centered under the scores.
        constexpr int fw = 5 * SMALL_FONT_W;
        DrawText(canvas, font, text_mid - fw / 2, line2_y, RED, "FINAL");
    }
    else if (g.is_scheduled() && !g.detail.empty())
    {
        // Scheduled game: show the formatted date/time in the tiny 5x8 font (text can be long).
        int dw = static_cast<int>(g.detail.size()) * SMALL_FONT_W;
        DrawText(canvas, font, text_mid - dw / 2, line2_y, CYAN, g.detail.c_str());
    }
    else if (!g.detail.empty())
    {
        // Any other status detail (e.g. "Delayed").
        int dw = static_cast<int>(g.detail.size()) * SMALL_FONT_W;
        DrawText(canvas, font, text_mid - dw / 2, line2_y, stc, g.detail.c_str());
    }

    // ── Row 3: betting odds ───────────────────────────────────────────────────
    if (g.odds)
    {
        // Combine spread and over/under into one string, separated by two spaces.
        std::string odds_str;
        if (!g.odds->spread.empty())
            odds_str += g.odds->spread;
        if (!g.odds->over_under.empty())
            odds_str += (odds_str.empty() ? "" : "  ") + g.odds->over_under;
        if (!odds_str.empty())
        {
            int ow = static_cast<int>(odds_str.size()) * SMALL_FONT_W;
            DrawText(canvas, font, text_mid - ow / 2, 56, ORANGE, odds_str.c_str());
        }
    }

    // Home logo: left-aligned in its box so the logo's left edge is flush against the text.
    logos_.draw(canvas, g.home.abbreviation, g.sport,
                text_end_x - 5, 2, logo_size_, /*right_align=*/false);

    return text_end_x + logo_gap_ - x_start + GAME_CARD_GAP; // total card pixel width
}

// ── Main render ──────────────────────────────────────────────────────────────

void Ticker::render(Canvas *canvas, const Font &font, const Font &large_font,
                    const Font &sched_font) const
{
    if (cards_.empty())
    {
        // Center "NO GAMES" on the display when there is nothing to show.
        const char *msg = "NO GAMES";
        int msg_w = static_cast<int>(std::strlen(msg)) * SMALL_FONT_W;
        DrawText(canvas, font, (display_width_ - msg_w) / 2, display_height_ / 2 + 4, CYAN, msg);
        return;
    }

    // Render the strip twice (back-to-back) so the wrap point is seamless.
    int strip_x = -scroll_x_; // offset of the first strip copy

    for (int pass = 0; pass < 2; ++pass)
    {
        int cx = strip_x;
        std::string prev_sport;

        for (auto &card : cards_)
        {
            // Draw a sport divider whenever the sport changes.
            if (card.game->sport != prev_sport)
            {
                if (cx + SPORT_DIVIDER_W > 0 && cx < display_width_)
                    draw_sport_divider(canvas, font, cx, card.game->sport);
                cx += SPORT_DIVIDER_W;
                prev_sport = card.game->sport;
            }

            // Skip cards fully outside the visible area to save CPU.
            if (cx + card.total_width > 0 && cx < display_width_)
                render_game_card(card, canvas, font, large_font, sched_font, cx);
            cx += card.total_width;
        }

        strip_x += total_strip_width_; // second copy starts one full strip to the right
    }
}

// ── Scroll advance ───────────────────────────────────────────────────────────

void Ticker::advance()
{
    // Accumulate scroll_speed_ (pixels × 4) and drain into scroll_x_ one pixel at a time.
    // This gives sub-pixel precision: speed=1 → 1px every 4 frames (~10 fps at 40 FPS).
    scroll_accum_ += scroll_speed_;
    scroll_x_ += scroll_accum_ / 4;
    scroll_accum_ %= 4;

    // Wrap when we've scrolled one full strip width (seamless loop).
    if (scroll_x_ >= total_strip_width_)
        scroll_x_ -= total_strip_width_;
}
