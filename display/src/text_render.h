/*
 * text_render.h - Text rendering helpers for the LED matrix.
 *
 * Thin wrapper around rpi-rgb-led-matrix C API text functions,
 * providing a simpler interface with our Color type.
 *
 * NOTE: The library exports draw_text(), load_font(), delete_font() etc.
 * We use different names to avoid linker conflicts.
 */

#ifndef TEXT_RENDER_H
#define TEXT_RENDER_H

#include "led-matrix-c.h"
#include <stdint.h>

/* Color type used throughout the ticker */
typedef struct { uint8_t r, g, b; } TickerColor;

/*
 * Draw text on the canvas. Returns the x position after the last character.
 */
static inline int ticker_draw_text(struct LedCanvas *canvas, struct LedFont *font,
                                    int x, int y, TickerColor color, const char *text) {
    if (!canvas || !font || !text) return x;
    return draw_text(canvas, font, x, y, color.r, color.g, color.b, text, 0);
}

#endif /* TEXT_RENDER_H */
