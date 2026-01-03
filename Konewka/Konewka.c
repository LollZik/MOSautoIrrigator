#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"


#define SLEEP_TIME 1800000 // 30min

// ADC
#define SPI_PORT        spi0
#define PIN_SCK         2
#define PIN_MOSI        3
#define PIN_MISO        4
#define PIN_CS          5

// MUX
#define MUX_ADDR_A 6
#define MUX_ADDR_B 7
#define MUX_ADDR_C 8
#define MUX_INH    9

// Watering
#define MOISTURE_THRESHOLD 800 //0-4095
#define WATERING_TIME 2000


// Wifi settings
#define WIFI_SSID "PicoNet"
#define WIFI_PASSWORD "MosKonewka"
#define DEST_IP "192.168.4.1"
#define DEST_PORT 12345

static struct udp_pcb *udp_client;
static ip_addr_t dest_addr;


// ==================== ADC & MUX ====================

void setup_adc(){ // External ADC, communitacion via SPI
    spi_init(SPI_PORT, 1000 * 1000); // 1 MHz
    gpio_set_function(PIN_SCK,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
}

void setup_mux(){
    gpio_init(MUX_ADDR_A);
    gpio_set_dir(MUX_ADDR_A, GPIO_OUT);

    gpio_init(MUX_ADDR_B);
    gpio_set_dir(MUX_ADDR_B, GPIO_OUT);

    gpio_init(MUX_ADDR_C);
    gpio_set_dir(MUX_ADDR_C, GPIO_OUT);
    
    gpio_init(MUX_INH);    
    gpio_set_dir(MUX_INH, GPIO_OUT);
    gpio_put(MUX_INH, 1);
}

int read_adc(uint8_t channel){ // MCP3008
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

int read_moisture(){
    long sum = 0;
    const int samples = 10;

    for(int i=0; i<samples; i++){
        sum += read_adc(0);
        sleep_ms(2);
    }

    return (uint16_t)(sum / samples);
}

void activate_pump(uint8_t channel_id) {
    if (channel_id > 7) return; 

    gpio_put(MUX_ADDR_A, (channel_id & 0x01));
    gpio_put(MUX_ADDR_B, (channel_id & 0x02));
    gpio_put(MUX_ADDR_C, (channel_id & 0x04));

    gpio_put(MUX_INH, 0);
    return;
}

void stop_all_pumps() {
    gpio_put(MUX_INH, 1);
}

// ==================== WI-FI ====================

bool setup_wifi(){
    if(cyw43_arch_init_with_country(CYW43_COUNTRY_POLAND)){
        return false;
    }
    cyw43_arch_enable_sta_mode();
    
    struct netif *n = &cyw43_state.netif[CYW43_ITF_STA];
    ip4_addr_t ip, netmask, gw;
    
    IP4_ADDR(&ip, 192, 168, 4, 2);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 4, 1);

    netif_set_addr(n, &ip, &netmask, &gw);
    netif_set_up(n);

    if(cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID,WIFI_PASSWORD,CYW43_AUTH_WPA2_AES_PSK,30000)){
        return false;
    }
    
    return true;
}

void send_udp_data(int moisture, int valve_state){
    if (!udp_client) return;
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
    sleep_ms(2000);

    setup_adc();
    setup_mux();

    if(!setup_wifi()){
        while(true){
             cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1); sleep_ms(200);
             cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0); sleep_ms(200);
        }
        return -1;
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);


    udp_client = udp_new();
    ipaddr_aton(DEST_IP, &dest_addr);

    while (true) {
        uint16_t moisture = read_moisture();
        bool valve_active = false;

        if(moisture < MOISTURE_THRESHOLD) { 
            activate_pump(0); // Select pump channel (0-7) connected to MUX
            valve_active = true;
            sleep_ms(WATERING_TIME);
            stop_all_pumps();
        }
        else{
            stop_all_pumps();
            valve_active = false;
        }
        send_udp_data(moisture, valve_active ? 1 : 0);

        sleep_ms(SLEEP_TIME);
    }
    return 0;
}