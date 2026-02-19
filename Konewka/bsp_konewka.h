#ifndef BSP_KONEWKA
#define BSP_KONEWKA

#include <stdint.h>
#include <stdbool.h>

// ADC
#define SPI_PORT        spi0
#define PIN_SCK         2
#define PIN_MOSI        3
#define PIN_MISO        4
#define PIN_CS          5

// Decoder (74HC238) 
#define DEC_ADDR_A 6
#define DEC_ADDR_B 7
#define DEC_ADDR_C 8
#define DEC_ENABLE 9

// Power supply
#define PIN_12V_ENABLE  10
#define PIN_CHG_STATUS  11

// Watering
#define MOISTURE_THRESHOLD 512 //0-1023
#define WATERING_TIME 2000


// Wifi settings
#define WIFI_SSID "PicoNet"
#define WIFI_PASSWORD "MosKonewka"
#define DEST_IP "192.168.4.1"
#define DEST_PORT 12345

#define MAX_PUMPS 8


// Sensors

void bsp_init(void);

uint16_t bsp_read_moisture(uint8_t channel_id);

bool bsp_is_battery_charging(void);

void bsp_pump_on(uint8_t channel_id);

void bsp_pumps_off(void);

// Communication

bool bsp_network_connect(void);

void bsp_send_message(uint16_t moisture, bool valve_state, bool is_charging);

void bsp_sleep_ms(uint32_t ms);
#endif