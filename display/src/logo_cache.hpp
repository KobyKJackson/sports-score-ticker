// logo_cache.hpp - Team logo PPM file cache and renderer.

#pragma once

#include "graphics.h"

#include <string>
#include <unordered_map>
#include <vector>

class LogoCache {
public:
    explicit LogoCache(const std::string &logo_dir);
    ~LogoCache() = default;

    // Draw a team logo on the canvas at (x, y), scaled to fit in max_size x max_size.
    void draw(rgb_matrix::Canvas *canvas, const std::string &team_abbrev,
              const std::string &sport, int x, int y, int max_size);

private:
    static constexpr int MAX_LOGO_DIM = 64;

    struct Image {
        std::vector<uint8_t> pixels; // RGB, row-major
        int width = 0;
        int height = 0;
    };

    // Load a PPM P6 file. Returns nullopt on failure.
    static bool load_ppm(const std::string &path, Image &out);

    // Try to find or load a logo, returning a pointer to the cached image (or nullptr).
    const Image *get(const std::string &team_abbrev, const std::string &sport);

    std::string logo_dir_;
    std::unordered_map<std::string, Image> cache_;
};
