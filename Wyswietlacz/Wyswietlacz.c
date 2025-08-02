#include <stdio.h>
#include <string.h>
#include <ssd1680.h>
#include <text_renderer.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/spi.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"

#define EINK_CS 17
#define EINK_DC 16
#define EINK_RST 15
#define EINK_BUSY 14 

#define WIFI_SSID "PicoNet"
#define WIFI_PASSWORD "MosKonewka"
#define LISTEN_PORT 12345
#define MAX_MSG_LEN 32

static struct udp_pcb *udp_server;

void eink_init(){
    spi_init(spi0, 2 * 1000 * 1000);
    gpio_set_function(18, GPIO_FUNC_SPI); // SCK
    gpio_set_function(19, GPIO_FUNC_SPI); // MOSI
    gpio_set_function(EINK_DC, GPIO_FUNC_SIO);
    gpio_set_function(EINK_CS, GPIO_FUNC_SIO);
    gpio_set_function(EINK_RST, GPIO_FUNC_SIO);
    gpio_set_function(EINK_BUSY, GPIO_FUNC_SIO);

    gpio_set_dir(EINK_DC, GPIO_OUT);
    gpio_set_dir(EINK_CS, GPIO_OUT);
    gpio_set_dir(EINK_RST, GPIO_OUT);
    gpio_set_dir(EINK_BUSY, GPIO_IN);
    epd_init();
}

bool setup_wifi(){
    if(cyw43_arch_init_with_country(CYW43_COUNTRY_POLAND)){
        printf("Wifi initialization error\n");
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

bool setup_udp(){
    udp_server = udp_new();
    if(!udp_server){
        return false;
    }
    if(udp_bind(udp_server, IP_ADDR_ANY, LISTEN_PORT) != ERR_OK){
        return false;
    }
    udp_recv(udp_server, udp_receive_callback, NULL);
    return true;
}

static void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port){
    char buf[MAX_MSG_LEN+1];
    int len = p->len < MAX_MSG_LEN ? p->len : MAX_MSG_LEN;
    memcpy(buf, p->payload, len);
    buf[len]='\0';

    int moisture = 0;
    int valve_state = 0;
    if(sscanf(buf, "%d,%d", &moisture, &valve_state) == 2){
        generate_text_bitmap(moisture, valve_state != 0);
    }
    else{
        text_renderer_init();
        draw_string(0,0,"Error");
    }

    pbuf_free(p);
}

void generate_text_bitmap(int moisture, bool valve){
    text_renderer_init(); // Clear framebuff first

    char line1[32];
    char line2[32];
    snprintf(line1, sizeof(line1), "Moisture: %d", moisture);
    snprintf(line2, sizeof(line2),"Valve: %s", valve ? "ON" : "OFF");
    draw_string(0,0, line1);
    draw_string(0, 10, line2);
    epd_display_image(framebuf);
}

int main(){
    stdio_init_all();

    if(!setup_wifi){
        return -1;
    }
    if(!setup_udp){
        return -1;
    }
    eink_init();

    while (true){
       // Messages received via UDP are handled automatically because of 
       // threadsafe_background mode, and e-ink is automatically updated
       // inside the udp_receive_callback function, therefore while loop is empty
    }
    return 0;
}