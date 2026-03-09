/*
 * LED Score Ticker - Main Entry Point
 *
 * Reads sports scores from JSON and displays them as a scrolling ticker
 * on a 320x64 RGB LED matrix (10x P4 panels, 2 rows of 5).
 *
 * Requires root for GPIO access.
 */

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "led-matrix-c.h"
#include "score_reader.h"
#include "ticker.h"
#include "logo_cache.h"

static volatile int g_running = 1;

static void handle_signal(int sig) {
    (void)sig;
    g_running = 0;
}

/* Default configuration - matches our hardware setup */
#define DEFAULT_ROWS        32
#define DEFAULT_COLS        64
#define DEFAULT_CHAIN       5
#define DEFAULT_PARALLEL    2
#define DEFAULT_BRIGHTNESS  80
#define DEFAULT_SLOWDOWN    2
#define DEFAULT_HARDWARE    "regular"
#define DEFAULT_DATA_FILE   "/tmp/scores.json"
#define DEFAULT_LOGO_DIR    "logos/"
#define DEFAULT_FONT_PATH   "display/libs/rpi-rgb-led-matrix/fonts/6x10.bdf"
#define DEFAULT_LARGE_FONT  "display/libs/rpi-rgb-led-matrix/fonts/9x15B.bdf"

/* Scroll speed: microseconds per frame */
#define FRAME_DELAY_US      25000   /* 40 FPS */

typedef struct {
    const char *data_file;
    const char *logo_dir;
    const char *font_path;
    const char *large_font_path;
    int scroll_speed;
    int brightness;
} AppConfig;

static AppConfig load_app_config(void) {
    AppConfig cfg = {
        .data_file = DEFAULT_DATA_FILE,
        .logo_dir = DEFAULT_LOGO_DIR,
        .font_path = DEFAULT_FONT_PATH,
        .large_font_path = DEFAULT_LARGE_FONT,
        .scroll_speed = 1,
        .brightness = DEFAULT_BRIGHTNESS,
    };

    /* Environment overrides */
    const char *env;
    if ((env = getenv("TICKER_DATA_FILE")))
        cfg.data_file = env;
    if ((env = getenv("TICKER_BRIGHTNESS")))
        cfg.brightness = atoi(env);

    return cfg;
}

int main(int argc, char **argv) {
    struct RGBLedMatrixOptions options;
    memset(&options, 0, sizeof(options));

    /* Hardware configuration for Electrodragon board + 10x P4 panels */
    options.rows = DEFAULT_ROWS;
    options.cols = DEFAULT_COLS;
    options.chain_length = DEFAULT_CHAIN;
    options.parallel = DEFAULT_PARALLEL;
    options.hardware_mapping = DEFAULT_HARDWARE;
    options.brightness = DEFAULT_BRIGHTNESS;
    options.led_rgb_sequence = "RGB";
    options.pwm_lsb_nanoseconds = 130;
    options.pwm_bits = 11;

    /*
     * Create the matrix. Use the options+rt_options variant for full control.
     * Command-line overrides (--led-rows, --led-chain, etc.) can be passed
     * via led_matrix_create_from_options() if needed.
     */
    struct RGBLedMatrix *matrix = led_matrix_create_from_options(
        &options, &argc, &argv);

    if (matrix == NULL) {
        fprintf(stderr, "Error: Could not create LED matrix.\n");
        fprintf(stderr, "Make sure you are running as root (sudo).\n");
        return 1;
    }

    /* Set up signal handlers for clean shutdown */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    struct LedCanvas *offscreen = led_matrix_create_offscreen_canvas(matrix);
    int width, height;
    led_canvas_get_size(offscreen, &width, &height);
    printf("LED Matrix: %dx%d pixels\n", width, height);

    AppConfig cfg = load_app_config();

    /* Initialize subsystems */
    logo_cache_init(cfg.logo_dir);

    struct LedFont *font = load_font(cfg.font_path);
    struct LedFont *large_font = load_font(cfg.large_font_path);
    if (!font) {
        fprintf(stderr, "Error: Could not load font: %s\n", cfg.font_path);
        led_matrix_delete(matrix);
        return 1;
    }

    /* Initialize ticker state */
    TickerState ticker;
    ticker_init(&ticker, width, height, cfg.scroll_speed);

    /* Score data */
    ScoreData scores = {0};
    time_t last_load = 0;
    int load_interval = 5; /* Check for new data every 5 seconds */

    printf("Score Ticker running. Data file: %s\n", cfg.data_file);
    printf("Press Ctrl+C to stop.\n");

    while (g_running) {
        /* Periodically reload score data */
        time_t now = time(NULL);
        if (now - last_load >= load_interval) {
            ScoreData new_scores = {0};
            if (score_reader_load(cfg.data_file, &new_scores) == 0) {
                score_data_free(&scores);
                scores = new_scores;
                ticker_update_games(&ticker, &scores);
            }
            last_load = now;
        }

        /* Clear the offscreen buffer */
        led_canvas_clear(offscreen);

        /* Render the current ticker frame */
        ticker_render(&ticker, offscreen, font, large_font);

        /* Advance the scroll position */
        ticker_advance(&ticker);

        /* Swap buffers */
        offscreen = led_matrix_swap_on_vsync(matrix, offscreen);

        /* Frame delay */
        usleep(FRAME_DELAY_US);
    }

    printf("\nShutting down...\n");

    /* Cleanup */
    score_data_free(&scores);
    ticker_cleanup(&ticker);
    logo_cache_cleanup();
    delete_font(font);
    if (large_font) delete_font(large_font);

    led_canvas_clear(offscreen);
    led_matrix_swap_on_vsync(matrix, offscreen);
    led_matrix_delete(matrix);

    printf("Done.\n");
    return 0;
}
