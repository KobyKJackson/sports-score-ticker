#include <iostream>
#include <string>
#include <signal.h>
#include <curl/curl.h>
#include "json.hpp"

#include "led-matrix.h"
#include "graphics.h"

// Use namespace for convenience
using json = nlohmann::json;
using namespace rgb_matrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo)
{
    interrupt_received = true;
}

// Callback function writes received data to a std::string
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *s)
{
    size_t newLength = size * nmemb;
    try
    {
        s->append((char *)contents, newLength);
        return newLength;
    }
    catch (std::bad_alloc &e)
    {
        // Handle memory problem
        return 0;
    }
}

static std::string ReadCurlData()
{
    std::string retVal = "DEFAULT";

    CURL *curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.4.27:5001/api/data");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK)
        {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }
        else
        {
            // Parse JSON and save it in a json object
            json jsonResponse;
            try
            {
                jsonResponse = json::parse(readBuffer);
                // Now you can do something with jsonResponse
                // Example: Accessing an element (adjust according to your actual JSON structure)
                std::cout << "Example element: " << jsonResponse << std::endl;
                retVal = jsonResponse[12]["data"][4]["data"][0]["data"];
            }
            catch (json::parse_error &e)
            {
                std::cerr << "JSON parse error: " << e.what() << std::endl;
            }
        }
    }
    return retVal;
}

static bool FullSaturation(const Color &c)
{
    return (c.r == 0 || c.r == 255) && (c.g == 0 || c.g == 255) && (c.b == 0 || c.b == 255);
}

static void add_micros(struct timespec *accumulator, long micros)
{
    const long billion = 1000000000;
    const int64_t nanos = (int64_t)micros * 1000;
    accumulator->tv_sec += nanos / billion;
    accumulator->tv_nsec += nanos % billion;
    while (accumulator->tv_nsec > billion)
    {
        accumulator->tv_nsec -= billion;
        accumulator->tv_sec += 1;
    }
}

int main()
{
    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    std::string TEXT = "Koby";

    Color lColor(255, 255, 255);
    Color lBGColor(255, 0, 0);
    Color lOutlineColor(0, 0, 0);
    bool lIsOutline = true;
    float lSpeed = 7.0f;
    bool lXOriginConfigured = false;
    int lXOrigin = 0;
    int lYOrigin = 0;
    int lLetterSpacing = 0;
    int lLoops = 25;

    /*
     * Load font. This needs to be a filename with a bdf bitmap font.
     */

    const char *lpFontFile = "../libs/rpi-rgb-led-matrix/fonts/4x6.bdf";
    rgb_matrix::Font lFont;
    if (!lFont.LoadFont(lpFontFile))
    {
        fprintf(stderr, "Couldn't load font '%s'\n", lpFontFile);
        return 1;
    }

    /*
     * If we want an outline around the font, we create a new font with
     * the original font as a template that is just an outline font.
     */
    rgb_matrix::Font *lpOutlineFont = NULL;
    if (lIsOutline)
    {
        lpOutlineFont = lFont.CreateOutlineFont();
    }

    // Setup Matrix
    RGBMatrix::Options lMatrixOptions;
    rgb_matrix::RuntimeOptions lRuntimeOptions;

    lMatrixOptions.rows = 32;
    lMatrixOptions.cols = 64;
    lMatrixOptions.chain_length = 1;
    lMatrixOptions.parallel = 1;
    lMatrixOptions.hardware_mapping = "adafruit-hat"; // or "adafruit-hat" depending on your setup
    lMatrixOptions.disable_hardware_pulsing = true;
    lRuntimeOptions.gpio_slowdown = 4;

    // Run Validate?

    RGBMatrix *lMatrix = CreateMatrixFromOptions(lMatrixOptions, lRuntimeOptions);
    if (lMatrix == nullptr)
    {
        std::cerr << "Could not create matrix object." << std::endl;
        return 1;
    }

    // Create a new canvas to be used with led_matrix_swap_on_vsync
    FrameCanvas *lOffscreenCanvas = lMatrix->CreateFrameCanvas();

    const bool lAllExtremeColors = (lMatrixOptions.brightness == 100) && FullSaturation(lColor) && FullSaturation(lBGColor) && FullSaturation(lOutlineColor);
    if (lAllExtremeColors)
    {
        lMatrix->SetPWMBits(1);
    }

    const int lScrollDirection = (lSpeed >= 0) ? -1 : 1;
    lSpeed = fabs(lSpeed);
    int lDelaySpeed_usec = 1000000;
    if (lSpeed > 0)
    {
        lDelaySpeed_usec = 1000000 / lSpeed / lFont.CharacterWidth('W');
    }

    if (!lXOriginConfigured)
    {
        if (lSpeed == 0)
        {
            // There would be no scrolling, so text would never appear. Move to front.
            lXOrigin = lIsOutline ? 1 : 0;
        }
        else
        {
            lXOrigin = lScrollDirection < 0 ? lMatrix->width() : 0;
        }
    }

    int lX = lXOrigin;
    int lY = lYOrigin;
    int lLength = 0;
    struct timespec lNextFrame = {0, 0};
    uint64_t lFrameCounter = 0;

    while (!interrupt_received && lLoops != 0)
    {
        ++lFrameCounter;
        lOffscreenCanvas->Fill(lBGColor.r, lBGColor.g, lBGColor.b);

        if (lIsOutline)
        {
            // The outline font, we need to write with a negative (-2) text-spacing,
            // as we want to have the same letter pitch as the regular text that
            // we then write on top.
            rgb_matrix::DrawText(lOffscreenCanvas, *lpOutlineFont,
                                 lX - 1, lY + lFont.baseline(),
                                 lOutlineColor, NULL,
                                 TEXT.c_str(), lLetterSpacing - 2);
        }

        // length = holds how many pixels our text takes up
        lLength = rgb_matrix::DrawText(lOffscreenCanvas, lFont,
                                       lX, lY + lFont.baseline(),
                                       lColor, NULL,
                                       TEXT.c_str(), lLetterSpacing);

        lX += lScrollDirection;
        if ((lScrollDirection < 0 && lX + lLength < 0) ||
            (lScrollDirection > 0 && lX > lMatrix->width()))
        {
            lX = lXOrigin + ((lScrollDirection > 0) ? -lLength : 0);
            if (lLoops > 0)
            {
                --lLoops;
                TEXT = ReadCurlData();
            }
        }

        // Make sure render-time delays are not influencing scroll-time
        if (lSpeed > 0)
        {
            if (lNextFrame.tv_sec == 0 && lNextFrame.tv_nsec == 0)
            {
                // First time. Start timer, but don't wait.
                clock_gettime(CLOCK_MONOTONIC, &lNextFrame);
            }
            else
            {
                add_micros(&lNextFrame, lDelaySpeed_usec);
                clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &lNextFrame, NULL);
            }
        }
        // Swap the offscreen_canvas with canvas on vsync, avoids flickering
        lOffscreenCanvas = lMatrix->SwapOnVSync(lOffscreenCanvas);
        if (lSpeed <= 0)
            pause(); // Nothing to scroll.
    }

    // Finished. Shut down the RGB matrix.
    lMatrix->Clear();
    delete lMatrix;

    return 0;
}
