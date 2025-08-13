#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"

// SPI
#define SPI_PORT        spi0
#define PIN_CS          4
#define PIN_MOSI        5
#define PIN_MISO        6
#define PIN_SCK         7


//Watering
#define VALVE_PIN 7
#define MOISTURE_THRESHOLD 500 //0-4095
#define WATERING_TIME 1000
#define SLEEP_TIME 1800000 // 30 min


//Wifi settings
#define WIFI_SSID "PicoNet"
#define WIFI_PASSWORD "MosKonewka"
#define DEST_IP "192.168.1.100"
#define DEST_PORT 12345

static struct udp_pcb *udp_client;
static ip_addr_t dest_addr;


void setup_adc(){ // Project uses external ADC, communitacion via SPI
    spi_init(SPI_PORT, 2 * 1000 * 1000); // 2MHz
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
}

int read_adc(uint8_t channel){
        uint8_t tx[3] = {
            0x01, // Start bit
            (uint8_t)(0x80 | ((channel & 0x07) << 4)), // Single-ended mode + channel selection
            0x00}; // Any value

        uint8_t rx[3] = {0};

        gpio_put(PIN_CS,0);
        spi_write_read_blocking(SPI_PORT, tx, rx, 3);
        gpio_put(PIN_CS,1);

        return ((rx[1] & 0x03) << 8) | rx[2];
}

bool setup_wifi(){
    if(cyw43_arch_init_with_country(CYW43_COUNTRY_POLAND)){
        printf("Initialization error\n");
        return false;
    }
    cyw43_arch_enable_sta_mode();
    if(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID,WIFI_PASSWORD,CYW43_AUTH_WPA2_AES_PSK,30000)){
        printf("Connection failed\n");
        return false;
    }
    printf("Connected\n");
    return true;
}

int read_moisture(){
    return (int)read_adc(0);
}

bool watering_controls(int moisture){
    if(moisture < MOISTURE_THRESHOLD){
        gpio_put(VALVE_PIN,1);
        sleep_ms(WATERING_TIME);
        gpio_put(VALVE_PIN,0);
        return true;
    }
    return false;
}

void send_udp_data(int moisture, int valve_state){
    char msg[32];
    int len = snprintf(msg, sizeof(msg),"%d,%d", moisture, valve_state);

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (!p) return;
    memcpy(p -> payload, msg, len);

    udp_sendto(udp_client, p, &dest_addr, DEST_PORT);
    pbuf_free(p);
}

int main(){
    stdio_init_all();
    setup_adc();

    gpio_init(VALVE_PIN);
    gpio_set_dir(VALVE_PIN, GPIO_OUT);
    gpio_put(VALVE_PIN,0); //Closed by default

    if(!setup_wifi()){
        return -1;
    }

    udp_client = udp_new();
    if(!udp_client){
        printf("UDP initialization error\n");
        return -1;
    }
    ipaddr_aton(DEST_IP, &dest_addr);

    while (true) {
        int moisture = read_moisture();
        int valve_state = watering_controls(moisture);
        send_udp_data(moisture, valve_state);
        sleep_ms(SLEEP_TIME);
    }
    return 0;
}