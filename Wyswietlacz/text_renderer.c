#include "text_renderer.h"
#include <string.h>
#include <stdio.h>

// Framebuffer definition
static uint8_t framebuf[X_BYTES * Y_LINES];

// Ustaw piksel (x,y) na czarny (0)
static void set_pixel(int x, int y){
    if (x < 0 || x >= EPD_WIDTH || y < 0 || y >= EPD_HEIGHT) return;
    int idx = y * X_BYTES + (x >> 3);
    int bit = 7 - (x & 7);
    framebuf[idx] &= ~(1 << bit);
}

// Simple 5x7 font definition
typedef struct {char c; uint8_t data[5];} font_char_t;
static const font_char_t font5x7[] ={
    {' ', {0x00,0x00,0x00,0x00,0x00}},
    {'0', {0x3E,0x45,0x49,0x51,0x3E}},
    {'1', {0x00,0x21,0x7F,0x01,0x00}},
    {'2', {0x23,0x45,0x49,0x49,0x31}},
    {'3', {0x42,0x41,0x49,0x49,0x36}},
    {'4', {0x18,0x14,0x12,0x7F,0x10}},
    {'5', {0x72,0x51,0x51,0x51,0x4E}},
    {'6', {0x1E,0x29,0x49,0x49,0x06}},
    {'7', {0x40,0x47,0x48,0x50,0x60}},
    {'8', {0x36,0x49,0x49,0x49,0x36}},
    {'9', {0x30,0x49,0x49,0x4A,0x3C}},
    {':', {0x00,0x36,0x36,0x00,0x00}},
    {'M', {0x7F,0x02,0x0C,0x02,0x7F}},
    {'o', {0x30,0x48,0x48,0x48,0x30}},
    {'i', {0x00,0x00,0x7A,0x00,0x00}},
    {'s', {0x72,0x4A,0x4A,0x4A,0x4C}},
    {'t', {0x04,0x3E,0x44,0x40,0x20}},
    {'u', {0x3C,0x40,0x40,0x20,0x7C}},
    {'r', {0x7C,0x08,0x04,0x04,0x08}},
    {'e', {0x38,0x54,0x54,0x54,0x18}},
    {'V', {0x7E,0x20,0x10,0x20,0x7E}},
    {'a', {0x38,0x54,0x54,0x54,0x7C}},
    {'l', {0x00,0x41,0x7F,0x40,0x00}},
    {'v', {0x3C,0x40,0x20,0x40,0x3C}},
};
static const int FONT_COUNT = sizeof(font5x7) / sizeof(font5x7[0]);

// Initialize renderer: clear framebuff to white
void text_renderer_init(void){
    memset(framebuf, 0xFF, sizeof(framebuf));
}

// Set a single pixel in framebuff to black
static void set_pixel(int x, int y){
    if( x < 0 || x >= EPD_WIDTH  ||  y < 0 || y >= EPD_HEIGHT) return;
    int idx = y * X_BYTES + (x >> 3);
    int bit = 7 - (x & 7);
    framebuf[idx] &= ~(1 << bit);
}


static void draw_char(int x, int y, char c){
    for (int i = 0; i < FONT_COUNT; i++) {
        if (font5x7[i].c == c){
            for (int col = 0; col < 5; col++){
                uint8_t bits = font5x7[i].data[col];
                for (int row = 0; row < 7; row++){
                    if (bits & (1 << row)){
                        set_pixel(x + col, y + row);
                    }
                }
            }
            return;
        }
    }
}

static void draw_string(int x0, int y0, const char *s){
    int x = x0;
    int y = y0;
    while (*s){
        if (*s == '\n'){
            y += 8; // line height + spacing
            x = x0; // reset column
        } else{
            draw_char(x, y, *s);
            x += 6; // character width + 1 pixel
        }
        s++;
    }
}