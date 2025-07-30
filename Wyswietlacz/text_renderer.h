#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include <stdint.h>
#include <stdbool.h>

// Dimensions of the e-ink display
#define EPD_WIDTH    250
#define EPD_HEIGHT   122
#define X_BYTES      ((EPD_WIDTH + 7) / 8)
#define Y_LINES      (EPD_HEIGHT)

extern uint8_t framebuf[X_BYTES * Y_LINES];


// Initialize text renderer
void text_renderer_init(void);

// Draw a single charatcer
void draw(int x, int y, char c);

// Draw a null-terminated string starting at (x0,y0) - supports '\n'
static void draw_string(int x0, int y0, const char *s);

#endif
