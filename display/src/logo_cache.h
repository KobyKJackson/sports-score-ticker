/*
 * logo_cache.h - Team logo PPM file cache and renderer.
 *
 * Loads team logos from PPM files on disk and caches them in memory.
 * Logos are stored as PPM (P6 binary) files in the logo directory,
 * named by team abbreviation: e.g., "lal.ppm", "bos.ppm"
 */

#ifndef LOGO_CACHE_H
#define LOGO_CACHE_H

#include "led-matrix-c.h"

#define MAX_LOGO_SIZE   64   /* Max logo dimension (pixels) */
#define MAX_CACHED_LOGOS 200  /* Max number of cached logos */

typedef struct {
    unsigned char *pixels; /* RGB pixel data, row-major */
    int width;
    int height;
    char key[32];          /* Cache key: "sport_abbrev" e.g. "nba_lal" */
} LogoImage;

/*
 * Initialize the logo cache with the directory containing PPM files.
 */
void logo_cache_init(const char *logo_dir);

/*
 * Draw a team logo on the canvas at (x, y), scaled to fit in max_size x max_size.
 * Looks up the logo by team abbreviation and sport.
 * Does nothing if the logo is not found.
 */
void logo_draw(struct LedCanvas *canvas, const char *team_abbrev,
               const char *sport, int x, int y, int max_size);

/*
 * Free all cached logos.
 */
void logo_cache_cleanup(void);

#endif /* LOGO_CACHE_H */
