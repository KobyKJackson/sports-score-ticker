// logo_cache.cpp - PPM logo loading, caching, and rendering.

#include "logo_cache.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>

LogoCache::LogoCache(const std::string &logo_dir) : logo_dir_(logo_dir) {}

bool LogoCache::load_ppm(const std::string &path, Image &out) {
    FILE *f = std::fopen(path.c_str(), "rb");
    if (!f) return false;

    char magic[4]{};
    if (std::fscanf(f, "%3s", magic) != 1 || std::strcmp(magic, "P6") != 0) {
        std::fclose(f);
        return false;
    }

    // Skip comments
    int c;
    while ((c = std::fgetc(f)) != EOF) {
        if (c == '#') {
            while ((c = std::fgetc(f)) != EOF && c != '\n') {}
        } else if (!std::isspace(c)) {
            std::ungetc(c, f);
            break;
        }
    }

    int w, h, maxval;
    if (std::fscanf(f, "%d %d %d", &w, &h, &maxval) != 3) {
        std::fclose(f);
        return false;
    }
    std::fgetc(f); // skip whitespace after maxval

    if (w <= 0 || h <= 0 || w > MAX_LOGO_DIM || h > MAX_LOGO_DIM || maxval > 255) {
        std::fclose(f);
        return false;
    }

    int data_size = w * h * 3;
    out.pixels.resize(data_size);
    if (static_cast<int>(std::fread(out.pixels.data(), 1, data_size, f)) != data_size) {
        out.pixels.clear();
        std::fclose(f);
        return false;
    }

    out.width = w;
    out.height = h;
    std::fclose(f);
    return true;
}

const LogoCache::Image *LogoCache::get(const std::string &team_abbrev, const std::string &sport) {
    std::string key = sport + "_" + team_abbrev;

    auto it = cache_.find(key);
    if (it != cache_.end()) {
        return it->second.pixels.empty() ? nullptr : &it->second;
    }

    // Lowercase the abbreviation for filename lookup
    std::string abbr_lower = team_abbrev;
    std::transform(abbr_lower.begin(), abbr_lower.end(), abbr_lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    Image img;

    // Try sport-specific logo first
    if (!load_ppm(logo_dir_ + "/" + sport + "_" + abbr_lower + ".ppm", img)) {
        // Try generic
        if (!load_ppm(logo_dir_ + "/" + abbr_lower + ".ppm", img)) {
            // Cache empty image so we don't retry
            cache_[key] = Image{};
            return nullptr;
        }
    }

    auto [inserted, _] = cache_.emplace(key, std::move(img));
    return &inserted->second;
}

void LogoCache::draw(rgb_matrix::Canvas *canvas, const std::string &team_abbrev,
                     const std::string &sport, int x, int y, int max_size) {
    if (!canvas || team_abbrev.empty()) return;

    const Image *img = get(team_abbrev, sport);
    if (!img) return;

    // Scale to fit within max_size x max_size
    int scale_w = max_size * 256 / img->width;
    int scale_h = max_size * 256 / img->height;
    int scale = std::min(scale_w, scale_h);

    int out_w = img->width * scale / 256;
    int out_h = img->height * scale / 256;

    // Center within the box
    int off_x = (max_size - out_w) / 2;
    int off_y = (max_size - out_h) / 2;

    // Nearest-neighbor scaling render
    for (int dy = 0; dy < out_h; ++dy) {
        int src_y = dy * img->height / out_h;
        for (int dx = 0; dx < out_w; ++dx) {
            int src_x = dx * img->width / out_w;
            int idx = (src_y * img->width + src_x) * 3;

            uint8_t r = img->pixels[idx];
            uint8_t g = img->pixels[idx + 1];
            uint8_t b = img->pixels[idx + 2];

            // Skip black pixels (transparent)
            if (r == 0 && g == 0 && b == 0) continue;

            canvas->SetPixel(x + off_x + dx, y + off_y + dy, r, g, b);
        }
    }
}
