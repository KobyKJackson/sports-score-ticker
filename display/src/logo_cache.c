/*
 * logo_cache.c - PPM logo loading, caching, and rendering.
 */

#include "logo_cache.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static LogoImage g_cache[MAX_CACHED_LOGOS];
static int g_cache_count = 0;
static char g_logo_dir[256] = "";

void logo_cache_init(const char *logo_dir) {
    if (logo_dir) {
        strncpy(g_logo_dir, logo_dir, sizeof(g_logo_dir) - 1);
        g_logo_dir[sizeof(g_logo_dir) - 1] = '\0';
    }
    g_cache_count = 0;
}

/* Find a cached logo by key */
static LogoImage *cache_find(const char *key) {
    for (int i = 0; i < g_cache_count; i++) {
        if (strcmp(g_cache[i].key, key) == 0)
            return &g_cache[i];
    }
    return NULL;
}

/* Load a PPM P6 file into a LogoImage */
static int load_ppm(const char *path, LogoImage *img) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    /* Read PPM header */
    char magic[4];
    if (fscanf(f, "%3s", magic) != 1 || strcmp(magic, "P6") != 0) {
        fclose(f);
        return -1;
    }

    /* Skip comments */
    int c;
    while ((c = fgetc(f)) != EOF) {
        if (c == '#') {
            while ((c = fgetc(f)) != EOF && c != '\n')
                ;
        } else if (!isspace(c)) {
            ungetc(c, f);
            break;
        }
    }

    int w, h, maxval;
    if (fscanf(f, "%d %d %d", &w, &h, &maxval) != 3) {
        fclose(f);
        return -1;
    }

    /* Skip the single whitespace after maxval */
    fgetc(f);

    if (w <= 0 || h <= 0 || w > MAX_LOGO_SIZE || h > MAX_LOGO_SIZE || maxval > 255) {
        fclose(f);
        return -1;
    }

    int data_size = w * h * 3;
    img->pixels = malloc(data_size);
    if (!img->pixels) {
        fclose(f);
        return -1;
    }

    if ((int)fread(img->pixels, 1, data_size, f) != data_size) {
        free(img->pixels);
        img->pixels = NULL;
        fclose(f);
        return -1;
    }

    img->width = w;
    img->height = h;
    fclose(f);
    return 0;
}

/* Try to load a logo from disk and cache it */
static LogoImage *cache_load(const char *key, const char *team_abbrev, const char *sport) {
    if (g_cache_count >= MAX_CACHED_LOGOS)
        return NULL;

    /* Build file path: logo_dir/abbrev.ppm (lowercase) */
    char path[512];
    char abbrev_lower[MAX_LOGO_SIZE];
    int i;
    for (i = 0; team_abbrev[i] && i < (int)sizeof(abbrev_lower) - 1; i++)
        abbrev_lower[i] = tolower((unsigned char)team_abbrev[i]);
    abbrev_lower[i] = '\0';

    /* Try sport-specific logo first, then generic */
    snprintf(path, sizeof(path), "%s/%s_%s.ppm", g_logo_dir, sport, abbrev_lower);
    LogoImage *img = &g_cache[g_cache_count];
    memset(img, 0, sizeof(*img));

    if (load_ppm(path, img) != 0) {
        /* Try without sport prefix */
        snprintf(path, sizeof(path), "%s/%s.ppm", g_logo_dir, abbrev_lower);
        if (load_ppm(path, img) != 0) {
            return NULL;
        }
    }

    strncpy(img->key, key, sizeof(img->key) - 1);
    g_cache_count++;
    return img;
}

void logo_draw(struct LedCanvas *canvas, const char *team_abbrev,
               const char *sport, int x, int y, int max_size) {
    if (!canvas || !team_abbrev || !team_abbrev[0])
        return;

    /* Build cache key */
    char key[32];
    snprintf(key, sizeof(key), "%s_%s", sport ? sport : "x", team_abbrev);

    /* Look up in cache */
    LogoImage *img = cache_find(key);
    if (!img) {
        img = cache_load(key, team_abbrev, sport ? sport : "x");
        if (!img) return;
    }

    /* Calculate scaling to fit in max_size x max_size */
    int scale_w = max_size * 256 / img->width;
    int scale_h = max_size * 256 / img->height;
    int scale = scale_w < scale_h ? scale_w : scale_h;

    int out_w = img->width * scale / 256;
    int out_h = img->height * scale / 256;

    /* Center within the max_size box */
    int off_x = (max_size - out_w) / 2;
    int off_y = (max_size - out_h) / 2;

    /* Nearest-neighbor scaling render */
    for (int dy = 0; dy < out_h; dy++) {
        int src_y = dy * img->height / out_h;
        for (int dx = 0; dx < out_w; dx++) {
            int src_x = dx * img->width / out_w;
            int src_idx = (src_y * img->width + src_x) * 3;

            unsigned char r = img->pixels[src_idx];
            unsigned char g = img->pixels[src_idx + 1];
            unsigned char b = img->pixels[src_idx + 2];

            /* Skip black pixels (treat as transparent) */
            if (r == 0 && g == 0 && b == 0)
                continue;

            led_canvas_set_pixel(canvas, x + off_x + dx, y + off_y + dy, r, g, b);
        }
    }
}

void logo_cache_cleanup(void) {
    for (int i = 0; i < g_cache_count; i++) {
        free(g_cache[i].pixels);
        g_cache[i].pixels = NULL;
    }
    g_cache_count = 0;
}
