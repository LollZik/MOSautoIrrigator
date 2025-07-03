#ifndef SSD1680_H
#define SSD1680_H

#include "pico/stdlib.h"

#define EPD_CS 17
#define EPD_DC 16
#define EPD_RST 15
#define EPD_BUSY 14

void epd_init(void);
void epd_reset(void);
void epd_wait_until_idle(void);
void eps_send_command(uint8_t command);
void eps_send_data(uint8_t data);
void epd_display_image(const uint8_t *image, uint16_t width, uint16_t height);

#endif
