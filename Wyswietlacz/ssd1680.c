#include "ssd1680.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

#define HEIGHT 122
#define WIDTH 250

void epd_reset(){
    gpio_put(EPD_RST,0);
    sleep_ms(10);
    gpio_put(EPD_RST,1);
    sleep_ms(10);
}

void epd_wait_until_idle(){
    while (gpio_get(EPD_BUSY) == 1){
        sleep_ms(10);
    }
}

void epd_send_command(uint8_t command){
    gpio_put(EPD_DC,0);
    gpio_put(EPD_CS,0);
    spi_write_blocking(spi0, &command, 1);
    gpio_put(EPD_CS,1);
}

void epd_send_data(uint8_t data){
    gpio_put(EPD_DC, 1);
    gpio_put(EPD_CS, 0);
    spi_write_blocking(spi0, &data, 1);
    gpio_put(EPD_CS, 1);
}

void epd_init(){
    epd_reset();
    epd_wait_until_idle();

    epd_send_command(0x12); // Software reset
    sleep_ms(10);

    epd_send_command(0x01);  // Driver output control
    // Display's height
    uint16_t m = HEIGHT - 1;
    uint8_t mux_lsb =  m & 0xFF;
    uint8_t mux_msb = (m >> 8) & 0x01;
    epd_send_data(mux_lsb);
    epd_send_data(mux_msb);
    epd_send_data(0x00);     // Default input direction

    epd_send_command(0x11);  // Data entry mode
    epd_send_data(0x03);     // Y increment, X increment

//  Resolution (for 250x122 e-ink)
    epd_send_command(0x44);  // Set RAM X adress
    epd_send_data(0x00);     // XStart = 0
    epd_send_data(0x1F);     // XEnd = 31 (width - 1 / 8)

    epd_send_command(0x45);  // Set RAM Y adress
    epd_send_data(0x00);     // YStart = 0
    epd_send_data(0x00);     // 8th byte = 0
    epd_send_data(0x79);     // YEnd = 121
    epd_send_data(0x00);     // 8th byte = 0

    epd_send_command(0x3C);  // Border Waveform Control (Panel border)
    epd_send_data(0x01);     // simplest waveform
    
//  Load waveform LUT
    epd_send_command(0x18);  // Temperature sensor control
    epd_send_data(0x80);     // Use internal sensor
    sleep_ms(2);             // Let the measurement stabilise

    epd_send_command(0x22);  // Display Update Control
    epd_send_data(0xB1);     // Load LUT from OTP

    epd_send_command(0x20);  // Execute Display Update Sequence
    epd_wait_until_idle();   // Wait until finished
}


void epd_display_image(const uint8_t *framebuf){
    uint16_t bytes_per_line = (WIDTH+7)/8;

    // Set RAM pointers to 0

    epd_send_command(0x4E); // Set RAM X adress
    epd_send_data(0x00);

    epd_send_command(0x4F); // Set RAM Y adress
    epd_send_data(0x00);
    epd_send_data(0x00);    // 8th byte

    epd_send_command(0x24);  // Write to RAM (Black & White only)    

    for(uint16_t y = 0; y < HEIGHT ; y++){
        for(uint16_t x = 0; x < bytes_per_line; x++){
            epd_send_data(framebuf[y * bytes_per_line + x]);
        }
    }  
    sleep_ms(10);
  
    epd_send_command(0x0C); //Softstart
    epd_send_data(0xD7);    // Gate driving voltage
    epd_send_data(0xD6);    // Source driving voltage
    epd_send_data(0x9D);    // VCOM voltage
    epd_send_data(0x00);    // Optionally: softstart timing control (zależnie od modułu)


    //Update the display
    epd_send_command(0x22); // Display Update Control
    epd_send_data(0xC7);    // Full update
    epd_send_command(0x20); // Master activation
    epd_wait_until_idle();
}

