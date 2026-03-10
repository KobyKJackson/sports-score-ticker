// LED Score Ticker - Main Entry Point
//
// Reads sports scores from JSON and displays them as a scrolling ticker
// on a 320x64 RGB LED matrix (10x P4 panels, 2 rows of 5).
//
// Requires root for GPIO access.

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>

#include "led-matrix.h"
#include "graphics.h"

#include "score_reader.hpp"
#include "ticker.hpp"
#include "logo_cache.hpp"

using rgb_matrix::RGBMatrix;
using rgb_matrix::FrameCanvas;
using rgb_matrix::Font;
using rgb_matrix::RuntimeOptions;

static volatile bool g_running = true;

static void handle_signal(int) {
    g_running = false;
}

struct AppConfig {
    std::string data_file   = "/tmp/scores.json";
    std::string logo_dir    = "logos/";
    std::string font_path   = "display/libs/rpi-rgb-led-matrix/fonts/6x10.bdf";
    std::string large_font  = "display/libs/rpi-rgb-led-matrix/fonts/9x15B.bdf";
    int scroll_speed        = 1;
    int brightness          = 80;

    void load_env() {
        if (const char *v = std::getenv("TICKER_DATA_FILE"))
            data_file = v;
        if (const char *v = std::getenv("TICKER_BRIGHTNESS"))
            brightness = std::atoi(v);
    }
};

static constexpr int FRAME_DELAY_US = 25000; // 40 FPS

int main(int argc, char **argv) {
    // Hardware configuration for Electrodragon board + 10x P4 panels
    RGBMatrix::Options matrix_opts;
    matrix_opts.rows = 32;
    matrix_opts.cols = 64;
    matrix_opts.chain_length = 5;
    matrix_opts.parallel = 2;
    matrix_opts.hardware_mapping = "regular";
    matrix_opts.brightness = 80;
    matrix_opts.led_rgb_sequence = "RGB";
    matrix_opts.pwm_lsb_nanoseconds = 130;
    matrix_opts.pwm_bits = 11;

    RuntimeOptions runtime_opts;
    runtime_opts.gpio_slowdown = 2;

    // Parse command-line flags (--led-rows, --led-chain, etc.)
    if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv, &matrix_opts, &runtime_opts)) {
        rgb_matrix::PrintMatrixFlags(stderr);
        return 1;
    }

    RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_opts, runtime_opts);
    if (!matrix) {
        std::fprintf(stderr, "Error: Could not create LED matrix.\n");
        std::fprintf(stderr, "Make sure you are running as root (sudo).\n");
        return 1;
    }

    std::signal(SIGINT, handle_signal);
    std::signal(SIGTERM, handle_signal);

    FrameCanvas *offscreen = matrix->CreateFrameCanvas();
    int width = matrix->width();
    int height = matrix->height();
    std::printf("LED Matrix: %dx%d pixels\n", width, height);

    AppConfig cfg;
    cfg.load_env();

    // Load fonts
    Font font;
    if (!font.LoadFont(cfg.font_path.c_str())) {
        std::fprintf(stderr, "Error: Could not load font: %s\n", cfg.font_path.c_str());
        delete matrix;
        return 1;
    }

    Font large_font;
    if (!large_font.LoadFont(cfg.large_font.c_str())) {
        std::fprintf(stderr, "Warning: Could not load large font: %s\n", cfg.large_font.c_str());
        // Fall through - we'll use the regular font as fallback
    }
    const Font &score_font = (large_font.height() > 0) ? large_font : font;

    // Initialize subsystems
    LogoCache logos(cfg.logo_dir);
    Ticker ticker(width, height, cfg.scroll_speed, logos);

    // Score data
    ScoreData scores;
    std::time_t last_load = 0;
    constexpr int load_interval = 5;

    std::printf("Score Ticker running. Data file: %s\n", cfg.data_file.c_str());
    std::printf("Press Ctrl+C to stop.\n");

    while (g_running) {
        // Periodically reload score data
        std::time_t now = std::time(nullptr);
        if (now - last_load >= load_interval) {
            auto new_scores = load_scores(cfg.data_file);
            if (new_scores) {
                scores = std::move(*new_scores);
                ticker.update_games(scores);
            }
            last_load = now;
        }

        // Clear, render, swap
        offscreen->Clear();
        ticker.render(offscreen, font, score_font);
        ticker.advance();
        offscreen = matrix->SwapOnVSync(offscreen);

        usleep(FRAME_DELAY_US);
    }

    std::printf("\nShutting down...\n");

    offscreen->Clear();
    matrix->SwapOnVSync(offscreen);
    delete matrix;

    std::printf("Done.\n");
    return 0;
}
