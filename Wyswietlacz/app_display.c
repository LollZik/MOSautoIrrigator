#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "bsp_display.h"
#include "ssd1680.h"
#include "text_renderer.h"

#define BUZZ_TIME_MS 10000

static bool buzzer_active = false;
static uint32_t buzzer_end_ms = 0;

void update_display(udp_data_t data) {
    text_renderer_init();

    if (data.data_error) {
        draw_string(0, 0, "Data error");
    } else {
        char line1[32];
        char line2[32];
        char line3[32];
        
        snprintf(line1, sizeof(line1), "Moisture: %d", data.moisture);
        snprintf(line2, sizeof(line2), "Valve: %s", data.valve_state ? "ON" : "OFF");
        snprintf(line3, sizeof(line3), "Battery: %s", data.is_charging ? "Charging" : "Full");
        
        draw_string(10, 40, line1);
        draw_string(10, 60, line2);
        draw_string(10, 80, line3);
        draw_logo();
    }
    
    epd_display_image(framebuf);
}