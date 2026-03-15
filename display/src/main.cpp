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
#include <fstream>
#include <string>
#include <unistd.h>

#include "led-matrix.h"
#include "graphics.h"

#include "score_reader.hpp"
#include "ticker.hpp"
#include "logo_cache.hpp"

using rgb_matrix::Font;
using rgb_matrix::FrameCanvas;
using rgb_matrix::RGBMatrix;
using rgb_matrix::RuntimeOptions;

static volatile bool g_running = true;

static void handle_signal(int)
{
    g_running = false;
}

struct AppConfig
{
    std::string data_file = "/tmp/scores.json";
    std::string logo_dir = "logos/";
    std::string font_path = "display/libs/rpi-rgb-led-matrix/fonts/7x13B.bdf";
    std::string large_font = "display/libs/rpi-rgb-led-matrix/fonts/10x20.bdf";
    std::string tiny_font = "display/libs/rpi-rgb-led-matrix/fonts/5x8.bdf";
    int scroll_speed = 1;
    int brightness = 80;
    bool notify_on_final = true;
    int notify_flash_count = 3;
    int notify_display_seconds = 5;

    void load_config_file()
    {
        const char *path = std::getenv("TICKER_CONFIG");
        std::string config_path = path ? path : "config/ticker.json";
        std::ifstream f(config_path);
        if (!f)
            return;
        std::string json((std::istreambuf_iterator<char>(f)), {});

        auto extract_int = [&](const char *key, int &out)
        {
            std::string search = std::string("\"") + key + "\"";
            auto pos = json.find(search);
            if (pos == std::string::npos)
                return;
            pos = json.find(':', pos + search.size());
            if (pos == std::string::npos)
                return;
            while (++pos < json.size() && std::isspace((unsigned char)json[pos]))
            {
            }
            if (pos < json.size() && (std::isdigit((unsigned char)json[pos]) || json[pos] == '-'))
                out = std::atoi(json.c_str() + pos);
        };

        auto extract_bool = [&](const char *key, bool &out)
        {
            std::string search = std::string("\"") + key + "\"";
            auto pos = json.find(search);
            if (pos == std::string::npos)
                return;
            pos = json.find(':', pos + search.size());
            if (pos == std::string::npos)
                return;
            while (++pos < json.size() && std::isspace((unsigned char)json[pos]))
            {
            }
            if (pos < json.size())
            {
                if (json[pos] == 't')
                    out = true;
                else if (json[pos] == 'f')
                    out = false;
            }
        };

        extract_int("scroll_speed", scroll_speed);
        extract_int("brightness", brightness);
        extract_bool("notify_on_final", notify_on_final);
        extract_int("notify_flash_count", notify_flash_count);
        extract_int("notify_display_seconds", notify_display_seconds);
    }

    void load_env()
    {
        if (const char *v = std::getenv("TICKER_DATA_FILE"))
            data_file = v;
        if (const char *v = std::getenv("TICKER_BRIGHTNESS"))
            brightness = std::atoi(v);
        if (const char *v = std::getenv("TICKER_SCROLL_SPEED"))
            scroll_speed = std::atoi(v);
    }
};

static constexpr int FRAME_DELAY_US = 25000; // 40 FPS

int main(int argc, char **argv)
{
    // Load config first so brightness and other settings apply to matrix init
    AppConfig cfg;
    cfg.load_config_file();
    cfg.load_env(); // env vars override config file

    // Hardware configuration — 4x P4 panels in U-mapper (128×64 display)
    RGBMatrix::Options matrix_opts;
    matrix_opts.rows = 32;        // pixels per panel row
    matrix_opts.cols = 64;        // pixels per panel column
    matrix_opts.chain_length = 5; // panels daisy-chained
    matrix_opts.parallel = 2;     // single parallel chain
    matrix_opts.hardware_mapping = "regular";
    // matrix_opts.pixel_mapper_config = "U-mapper"; // folds chain into 2-row U shape
    matrix_opts.disable_hardware_pulsing = true; // required for Pi 4
    matrix_opts.brightness = cfg.brightness;     // from config/ticker.json
    matrix_opts.led_rgb_sequence = "RGB";
    matrix_opts.pwm_lsb_nanoseconds = 130;
    matrix_opts.pwm_bits = 11;

    RuntimeOptions runtime_opts;
    runtime_opts.gpio_slowdown = 2;   // Pi 4 needs slowdown=2 to avoid glitches
    runtime_opts.drop_privileges = 0; // keep root so scores.json remains readable

    // Parse command-line flags (--led-rows, --led-chain, etc.) — overrides defaults above
    if (!rgb_matrix::ParseOptionsFromFlags(&argc, &argv, &matrix_opts, &runtime_opts))
    {
        rgb_matrix::PrintMatrixFlags(stderr);
        return 1;
    }

    RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_opts, runtime_opts);
    if (!matrix)
    {
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

    // Scale scroll speed proportionally so narrow displays feel the same as 320px wide
    cfg.scroll_speed = std::max(cfg.scroll_speed, cfg.scroll_speed * 320 / width);
    std::printf("Scroll speed: %d (display %dx%d)\n", cfg.scroll_speed, width, height);

    // Load fonts
    Font font;
    if (!font.LoadFont(cfg.font_path.c_str()))
    {
        std::fprintf(stderr, "Error: Could not load font: %s\n", cfg.font_path.c_str());
        delete matrix;
        return 1;
    }

    Font large_font;
    if (!large_font.LoadFont(cfg.large_font.c_str()))
    {
        std::fprintf(stderr, "Warning: Could not load large font: %s\n", cfg.large_font.c_str());
    }
    const Font &score_font = (large_font.height() > 0) ? large_font : font;

    Font tiny_font;
    if (!tiny_font.LoadFont(cfg.tiny_font.c_str()))
    {
        std::fprintf(stderr, "Warning: Could not load tiny font: %s\n", cfg.tiny_font.c_str());
    }
    const Font &sched_font = (tiny_font.height() > 0) ? tiny_font : font;

    // Initialize subsystems
    LogoCache logos(cfg.logo_dir);
    Ticker ticker(width, height, cfg.scroll_speed, logos);

    // Score data
    ScoreData scores;
    std::time_t last_load = 0;
    std::time_t last_config = 0;
    std::time_t last_notify_check = 0;
    constexpr int load_interval = 5;
    constexpr int config_interval = 5;
    constexpr int notify_check_interval = 1;
    const std::string notify_file = "/tmp/score_notifications.json";

    // Track applied values so we only act when something actually changed.
    int applied_brightness = cfg.brightness;
    int applied_scroll_speed = cfg.scroll_speed;

    std::printf("Score Ticker running. Data file: %s\n", cfg.data_file.c_str());
    std::printf("Press Ctrl+C to stop.\n");

    while (g_running)
    {
        std::time_t now = std::time(nullptr);

        // Periodically reload config and apply any changed settings live.
        if (now - last_config >= config_interval)
        {
            cfg.load_config_file();
            // env vars always win over config file
            cfg.load_env();
            // scale scroll speed for display width (same as startup)
            int scaled = std::max(cfg.scroll_speed, cfg.scroll_speed * 320 / width);

            if (cfg.brightness != applied_brightness)
            {
                matrix->SetBrightness(cfg.brightness);
                std::fprintf(stderr, "Brightness changed to %d\n", cfg.brightness);
                applied_brightness = cfg.brightness;
            }
            if (scaled != applied_scroll_speed)
            {
                ticker.set_scroll_speed(scaled);
                std::fprintf(stderr, "Scroll speed changed to %d\n", scaled);
                applied_scroll_speed = scaled;
            }
            ticker.set_notify_config(cfg.notify_flash_count, cfg.notify_display_seconds);
            last_config = now;
        }

        // Periodically check for notifications
        if (cfg.notify_on_final && now - last_notify_check >= notify_check_interval)
        {
            auto notifs = load_notifications(notify_file);
            if (notifs && !notifs->notifications.empty())
            {
                for (auto &n : notifs->notifications)
                    ticker.queue_notification(n);
                clear_notifications(notify_file);
                std::fprintf(stderr, "Queued %zu notification(s)\n",
                             notifs->notifications.size());
            }
            last_notify_check = now;
        }

        // Periodically reload score data
        if (now - last_load >= load_interval)
        {
            auto new_scores = load_scores(cfg.data_file);
            if (new_scores)
            {
                scores = std::move(*new_scores);
                std::fprintf(stderr, "Loaded %d games from %s\n",
                             scores.total_games(), cfg.data_file.c_str());
                ticker.update_games(scores, score_font);
            }
            else
            {
                std::fprintf(stderr, "Failed to load scores from %s\n",
                             cfg.data_file.c_str());
            }
            last_load = now;
        }

        // Clear, render, swap
        offscreen->Clear();
        ticker.render(offscreen, font, score_font, sched_font);
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
