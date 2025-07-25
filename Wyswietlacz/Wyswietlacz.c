#include <stdio.h>
#include <string.h>
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
#define MAX_MSG_LEN 64

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

static void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port){
    char buf[MAX_MSG_LEN+1];
    int len = p->len < MAX_MSG_LEN ? p->len : MAX_MSG_LEN;
    memcpy(buf, p->payload, len);
    buf[len]='\0';

    // E-ink

    pbuf_free(p);
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


void update_display(int moisture, int valve_status){\
    char text[64];
    snprintf(text, sizeof(text), "Moisture:%d\nValve: %d",moisture,valve_status ? "OPEN" : "CLOSED");
     eink_clear();
     eink_draw_text(text);
     eink_refresh();
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

