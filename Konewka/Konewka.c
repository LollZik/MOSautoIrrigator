#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"


#define MOISTURE_SENSOR_PIN 26
#define VALVE_PIN 2


#define MOISTURE_THRESHOLD 500 //0-4095

//Network settings

#define WIFI_SSID "PicoNet"
#define WIFI_PASSWORD "12345678"
#define UDP_PORT 12345


struct udp_pcb *udp_conn;
ip_addr_t client_ip;
uint16_t client_port = UDP_PORT;

void setup_adc(){
    adc_init();
    adc_gpio_init(MOISTURE_SENSOR_PIN);
    adc_select_input(0);
}


int read_moisture(){
    return adc_read();
}

void watering_controls(int moisture){
    if(moisture < MOISTURE_THRESHOLD){
        gpio_put(VALVE_PIN,1);
        sleep_ms(5000);
        gpio_put(VALVE_PIN,0);
    }

}



void setup_wifi(){
    cyw43_arch_init();
    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
}

void setup_udp(){
    udp_conn = udp_new();
    ip_addr_set_any(IPADDR_TYPE_V4, &client_ip);
    udp_bind(udp_conn, IPADDR_ANY, UDP_PORT);
}

void send_udp_data(int moisture, int valve_state){
    char msg[32];
    snprintf(msg, sizeof(msg),"Moisture:%d Valve:%d",moisture,valve_state);

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, strlen(msg), PBUF_RAM);
    if (!p) return;

    memcpy(p -> payload, msg, strlen(msg));

    udp_sendto(udp_conn, p, &client_ip, client_port);
    pbuf_free(p);
}




int main(){
    stdio_init_all();
    setup_adc();
    setup_wifi();
    setup_udp();

    gpio_init(VALVE_PIN);
    gpio_set_dir(VALVE_PIN, GPIO_OUT);
    gpio_put(VALVE_PIN,0); //Closed by default

    while (true) {
        int moisture = read_moisture();
        int valve_state = gpio_get(VALVE_PIN);
        send_udp_data(moisture, valve_state);
        watering_controls(moisture);

        sleep_ms(1800000); // 30 min
    }

    return 0;
}
