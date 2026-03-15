// ticker.hpp - Scrolling score ticker renderer.
//
// Renders a continuous scrolling strip of game cards across the LED display.
// Layout per card: [Logo] AWAY score @ score HOME [Logo]
//                  [period clock]
//                  [odds spread / over-under]

#pragma once

#include "graphics.h"
#include "score_reader.hpp"
#include "logo_cache.hpp"

#include <algorithm>
#include <queue>
#include <string>
#include <vector>

class Ticker
{
public:
    Ticker(int display_width, int display_height, int scroll_speed, LogoCache &logos);

    // Replace the current game list from new score data. Resets card widths and strip length.
    // large_font is used to measure exact row-1 text widths so card sizing is pixel-accurate.
    void update_games(const ScoreData &data, const rgb_matrix::Font &large_font);

    // Draw one frame of the scrolling ticker onto the canvas.
    void render(rgb_matrix::Canvas *canvas, const rgb_matrix::Font &font,
                const rgb_matrix::Font &large_font,
                const rgb_matrix::Font &sched_font) const;

    // Advance the scroll position by one frame (call once per frame after render).
    void advance();

    // Update scroll speed at runtime (e.g. after a config reload).
    void set_scroll_speed(int speed) { scroll_speed_ = std::max(1, speed); }

    // Queue a game notification (flashes screen, then shows centered card).
    void queue_notification(const Game &g);

    // Update notification config at runtime.
    void set_notify_config(int flash_count, int display_seconds);

    // Returns true if currently showing a notification (flashing or holding).
    bool in_notification() const { return notify_phase_ != NotifyPhase::None; }

private:
    // Blank pixels between the end of one game card and the start of the next.
    static constexpr int GAME_CARD_GAP = 40;
    // Width of the colored sport-label bar drawn between sport groups.
    static constexpr int SPORT_DIVIDER_W = 25;

    // Extra pixel advance added between row-1 characters (makes 10x20 feel like ~12-wide).
    static constexpr int LARGE_EXTRA_ADVANCE = 0;

    // Approximate character widths for row-2/3 pixel-width estimation (fixed-width fonts).
    static constexpr int SMALL_FONT_W = 7; // 7x13B font
    static constexpr int TINY_FONT_W = 5;  // 5x8 font

    // Color palette used across all rendering functions.
    static const rgb_matrix::Color WHITE;
    static const rgb_matrix::Color YELLOW;
    static const rgb_matrix::Color GREEN;
    static const rgb_matrix::Color RED;
    static const rgb_matrix::Color CYAN;
    static const rgb_matrix::Color GRAY;
    static const rgb_matrix::Color ORANGE;
    static const rgb_matrix::Color DIM;

    // One rendered game entry: pointer to the game data and its pre-computed pixel widths.
    struct TickerCard
    {
        const Game *game = nullptr;
        int total_width = 0;
        int row1_width = 0; // pixel width of row-1 content, for centering
    };

    // Compute the total pixel width of a game card (logos + text + gap).
    // Also writes the row-1 content width into row1_w_out for use when centering.
    int calc_card_width(const Game &game, const rgb_matrix::Font &large_font, int &row1_w_out) const;

    // Sum the advance widths of each character in str using the given font.
    static int measure_text(const rgb_matrix::Font &font, const std::string &str);

    // Map sport key to its divider background color.
    static rgb_matrix::Color sport_color(const std::string &sport);
    // Map sport key to its short display label ("NBA", "CFB", etc.).
    static const char *sport_label(const std::string &sport);
    // Map game status to its display color (green = live, red = final, etc.).
    static rgb_matrix::Color status_color(const std::string &status);

    // Draw the colored sport-divider bar at pixel column x.
    void draw_sport_divider(rgb_matrix::Canvas *canvas, const rgb_matrix::Font &font,
                            int x, const std::string &sport) const;
    // Draw one game card starting at pixel column x_start. Returns the card's pixel width.
    int render_game_card(const TickerCard &card, rgb_matrix::Canvas *canvas,
                         const rgb_matrix::Font &font, const rgb_matrix::Font &large_font,
                         const rgb_matrix::Font &sched_font, int x_start) const;

    int scroll_x_ = 0;     // current scroll offset in pixels
    int scroll_accum_ = 0; // sub-pixel accumulator for fractional scroll speeds
    int scroll_speed_;     // pixels-per-4-frames scroll rate
    int display_width_;    // canvas width in pixels
    int display_height_;   // canvas height in pixels
    int logo_size_;        // logo bounding box side length (square), px
    int logo_gap_;         // logo_size_ + 4: text starts 4px after logo right edge

    std::vector<TickerCard> cards_; // one entry per game, in display order
    int total_strip_width_ = 0;     // total pixel width of one full scroll cycle

    LogoCache &logos_;

    // Owns the Game pointers referenced by cards_; kept alive across update_games() calls.
    std::vector<const Game *> game_ptrs_;

    // ── Notification state machine ────────────────────────────────────────────
    enum class NotifyPhase { None, Active };
    NotifyPhase notify_phase_ = NotifyPhase::None;
    Game notify_game_;                // current notification game (owned copy)
    std::queue<Game> notify_queue_;   // pending notifications

    int notify_flash_count_ = 3;     // config: number of border flash cycles
    int notify_display_frames_ = 200; // config: total display duration in frames (5s * 40fps)

    int notify_frames_remaining_ = 0; // total frames left for this notification
    int notify_flash_timer_ = 0;      // frame counter within one flash cycle
    int notify_flash_remaining_ = 0;  // flash cycles left
    bool notify_flash_on_ = false;    // true = border visible, false = border off

    static constexpr int NOTIFY_BORDER_W = 3; // border thickness in pixels

    // Start the next notification from the queue.
    void start_next_notification();

    // Render the centered card with optional flashing border.
    void render_notification(rgb_matrix::Canvas *canvas, const rgb_matrix::Font &font,
                             const rgb_matrix::Font &large_font,
                             const rgb_matrix::Font &sched_font) const;

    // Draw a border around the display edge.
    void draw_border(rgb_matrix::Canvas *canvas, const rgb_matrix::Color &color) const;

    // Advance the notification state machine by one frame.
    void advance_notification();
};
