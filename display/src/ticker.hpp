// ticker.hpp - Scrolling score ticker renderer.
//
// Renders a continuous scrolling strip of game cards across the 320x64 display.
// Each game card shows: [Logo] AWAY score - score HOME | period clock | odds | venue

#pragma once

#include "graphics.h"
#include "score_reader.hpp"
#include "logo_cache.hpp"

#include <string>
#include <vector>

class Ticker {
public:
    Ticker(int display_width, int display_height, int scroll_speed, LogoCache &logos);

    // Update the ticker's game list from new score data
    void update_games(const ScoreData &data);

    // Render one frame of the ticker to the canvas
    void render(rgb_matrix::Canvas *canvas, const rgb_matrix::Font &font,
                const rgb_matrix::Font &large_font) const;

    // Advance the scroll position by one frame
    void advance();

private:
    static constexpr int GAME_CARD_GAP = 10;
    static constexpr int SPORT_DIVIDER_W = 20;
    static constexpr int LOGO_SIZE = 28;

    // Color palette
    static const rgb_matrix::Color WHITE;
    static const rgb_matrix::Color YELLOW;
    static const rgb_matrix::Color GREEN;
    static const rgb_matrix::Color RED;
    static const rgb_matrix::Color CYAN;
    static const rgb_matrix::Color GRAY;
    static const rgb_matrix::Color ORANGE;
    static const rgb_matrix::Color DIM;

    struct TickerCard {
        const Game *game = nullptr;
        int total_width = 0;
    };

    int calc_card_width(const Game &game) const;

    static rgb_matrix::Color sport_color(const std::string &sport);
    static const char *sport_label(const std::string &sport);
    static rgb_matrix::Color status_color(const std::string &status);

    void draw_sport_divider(rgb_matrix::Canvas *canvas, const rgb_matrix::Font &font,
                            int x, const std::string &sport) const;
    int render_game_card(const TickerCard &card, rgb_matrix::Canvas *canvas,
                         const rgb_matrix::Font &font, const rgb_matrix::Font &large_font,
                         int x_start) const;

    int scroll_x_ = 0;
    int scroll_speed_;
    int display_width_;
    int display_height_;

    std::vector<TickerCard> cards_;
    int total_strip_width_ = 0;

    LogoCache &logos_;

    // Store games so pointers in cards_ remain valid
    std::vector<const Game *> game_ptrs_;
};
