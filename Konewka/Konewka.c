#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"

//Watering
#define MOISTURE_SENSOR_PIN 26
#define VALVE_PIN 2
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


void setup_adc(){
    adc_init();
    adc_gpio_init(MOISTURE_SENSOR_PIN);
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
    adc_select_input(0);
    return (uint16_t) adc_read();
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
    char msg[64];
    int len = snprintf(msg, sizeof(msg), "Moisture = %d, Valve = %d", moisture, valve_state);

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