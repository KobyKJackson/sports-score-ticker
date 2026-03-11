// logo_cache.cpp - PPM logo loading, caching, and rendering.
//
// Logos are stored as P6 PPM files in logos/{sport}_{abbrev}.ppm (e.g. nba_lal.ppm).
// They are loaded on first use and kept in memory for the lifetime of the app.

#include "logo_cache.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

LogoCache::LogoCache(const std::string &logo_dir) : logo_dir_(logo_dir) {}

// Load a binary PPM (P6) file into out. Returns false on any error.
bool LogoCache::load_ppm(const std::string &path, Image &out)
{
    FILE *f = std::fopen(path.c_str(), "rb");
    if (!f)
        return false;

    // Verify the magic "P6" header
    char magic[4]{};
    if (std::fscanf(f, "%3s", magic) != 1 || std::strcmp(magic, "P6") != 0)
    {
        std::fclose(f);
        return false;
    }

    // Skip optional comment lines (lines starting with '#')
    int c;
    while ((c = std::fgetc(f)) != EOF)
    {
        if (c == '#')
        {
            while ((c = std::fgetc(f)) != EOF && c != '\n')
            {
            } // skip to end of comment
        }
        else if (!std::isspace(c))
        {
            std::ungetc(c, f); // put back the first non-space non-comment character
            break;
        }
    }

    // Read width, height, and max channel value
    int w, h, maxval;
    if (std::fscanf(f, "%d %d %d", &w, &h, &maxval) != 3)
    {
        std::fclose(f);
        return false;
    }
    std::fgetc(f); // consume the single whitespace byte that separates header from pixel data

    // Reject malformed or oversized images
    if (w <= 0 || h <= 0 || w > MAX_LOGO_DIM || h > MAX_LOGO_DIM || maxval > 255)
    {
        std::fclose(f);
        return false;
    }

    // Read raw RGB pixel data (3 bytes per pixel, row-major)
    int data_size = w * h * 3;
    out.pixels.resize(data_size);
    if (static_cast<int>(std::fread(out.pixels.data(), 1, data_size, f)) != data_size)
    {
        out.pixels.clear();
        std::fclose(f);
        return false;
    }

    out.width = w;
    out.height = h;
    std::fclose(f);
    return true;
}

// Return a pointer to the cached image for the given team+sport, loading it if needed.
// Returns nullptr if the logo file is not found.
const LogoCache::Image *LogoCache::get(const std::string &team_abbrev, const std::string &sport)
{
    std::string key = sport + "_" + team_abbrev; // cache key is "nba_LAL" etc.

    auto it = cache_.find(key);
    if (it != cache_.end())
    {
        // Empty pixels means we already tried and failed — don't retry
        return it->second.pixels.empty() ? nullptr : &it->second;
    }

    // Filenames are all lowercase (e.g. "nba_lal.ppm")
    std::string abbr_lower = team_abbrev;
    std::transform(abbr_lower.begin(), abbr_lower.end(), abbr_lower.begin(),
                   [](unsigned char c)
                   { return std::tolower(c); });

    Image img;

    // Try sport-specific file first, fall back to generic abbreviation
    if (!load_ppm(logo_dir_ + "/" + sport + "_" + abbr_lower + ".ppm", img))
    {
        if (!load_ppm(logo_dir_ + "/" + abbr_lower + ".ppm", img))
        {
            // Cache a sentinel empty image so we don't hit the filesystem again
            cache_[key] = Image{};
            return nullptr;
        }
    }

    // Move image into the cache and return a pointer to it
    auto [inserted, _] = cache_.emplace(key, std::move(img));
    return &inserted->second;
}

// Draw a team logo scaled to fit within a max_size × max_size box at canvas position (x, y).
// Black pixels (0,0,0) in the source image are treated as transparent and skipped.
void LogoCache::draw(rgb_matrix::Canvas *canvas, const std::string &team_abbrev,
                     const std::string &sport, int x, int y, int max_size, bool right_align)
{
    if (!canvas || team_abbrev.empty())
        return;

    const Image *img = get(team_abbrev, sport);
    if (!img)
        return;

    // Compute uniform scale factor (fixed-point with 8-bit fractional part)
    int scale_w = max_size * 256 / img->width;
    int scale_h = max_size * 256 / img->height;
    int scale = std::min(scale_w, scale_h); // fit within the box, preserving aspect ratio

    int out_w = img->width * scale / 256;
    int out_h = img->height * scale / 256;

    // Align horizontally: right-align pushes logo flush to the right edge of the box
    // (so it sits right next to text on the left side), left-align does the opposite.
    int off_x = right_align ? (max_size - out_w) : 0;
    int off_y = (max_size - out_h) / 2; // always vertically centered

    // Nearest-neighbor scaling: map each output pixel to its closest source pixel
    for (int dy = 0; dy < out_h; ++dy)
    {
        int src_y = dy * img->height / out_h; // nearest source row
        for (int dx = 0; dx < out_w; ++dx)
        {
            int src_x = dx * img->width / out_w; // nearest source column
            int idx = (src_y * img->width + src_x) * 3;

            uint8_t r = img->pixels[idx];
            uint8_t g = img->pixels[idx + 1];
            uint8_t b = img->pixels[idx + 2];

            // Pure black is the transparency color used by download_logos.py
            if (r == 0 && g == 0 && b == 0)
                continue;

            canvas->SetPixel(x + off_x + dx, y + off_y + dy, r, g, b);
        }
    }
}
