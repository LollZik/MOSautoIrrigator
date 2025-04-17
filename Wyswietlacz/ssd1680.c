#include "ssd1680.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"


void epd_init(){
    epd_reset();
    epd_wait_until_idle();

    epd_send_command(0x12); // Software reset
    sleep_ms(10);

    epd_send_command(0x01);  // Driver output control
    epd_send_data(0x27);     // Input direction
    epd_send_data(0x01);     // display width
    epd_send_data(0x00);     // display height

    epd_send_command(0x11);  // Data entry mode
    epd_send_data(0x03);     // Y increment, X increment
    epd_send_data(0x00);     // AM = "0" X-Mode


//  Resolution
    epd_send_command(0x44);  // Set RAM X adress
    epd_send_data(0x00);     // XStart = 0
    epd_send_data(0x3F);     // XEnd = 63

    epd_send_command(0x45);  // Set RAM Y adress
    epd_send_data(0x00);     // YStart = 0
    epd_send_data(0xFF);     // YEnd = 255

    epd_send_command(0x3C);  // Border Waveform Control (Panel border)
    epd_send_data(0x51);     // Fix level voltage
    
//  Load waveform LUT
    epd_send_command(0x18);  // Read from temperature register
    epd_send_data(0x80);

    epd_send_command(0x22);  // Display Update Control
    epd_send_data(0xB1);     // Load LUT from OTP

    epd_send_command(0x20);  // Execute Display Update Sequence
    epd_wait_until_idle();

//  Initialization finished, ready for image display  
}

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


void epd_display_image(const uint8_t *image, uint16_t width, uint16_t height){
    uint16_t bytes_per_line = width/8;


    //EPD_CLEAR()

    epd_send_command(0x4E); // Set RAM X adress counter
    epd_send_data(0x00);

    epd_send_command(0x4F); // Set RAM Y adress counter
    epd_send_data(0x00);

    epd_send_command(0x24);  // Write RAM (Black & White only)    


    for(uint16_t y = 0; y < height ; y++){
        for(uint16_t x = 0; x < bytes_per_line; x++){
            epd_send_data(image[y * bytes_per_line + x]);
        }
    }  
    sleep_ms(10);
    
  
    epd_send_command(0x0C); //Softstart
    epd_send_data(0xD7);
    epd_send_data(0xD6);
    epd_send_data(0x9D);

    //Update
    epd_send_command(0x22); // Display Update Control
    epd_send_data(0xC7);    // Display Refresh
    epd_send_command(0x20); // Master activation
    epd_wait_until_idle();
}

void epd_clear(){
    
}

