#include <stdio.h>
#include <pico/stdlib.h>
#include <hardware/spi.h>
#include <pico/cyw43_arch.h>

#define EINK_CS 17
#define EINK_DC 16
#define EINK_RST 15
#define EINK_BUSY 14 

#define WIFI_SSID "PicoNet"
#define WIFI_PASS "12345678"
#define UDP_PORT 12345

struct udp_pcb *udp_conn;

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
}

void wifi_init(){
    if(cyw43_arch_init()){
        return;
    }
    cyw43_arch_enable_sta_mode();
}

void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t addr, u16_t port){
    if (p != NULL){
        char buf[32] = {0};
        pbuf_copy_partial(p, buf, sizeof(buf)-1, 0);
        pbuf_free(p);

        int moisture = 0, valve_state = 0;
        if (sscanf(buf, "Moisture:%d Valve:%d",&moisture, &valve_state) == 2){
            printf("Moisture:%d Valve:%d",moisture, valve_state);
            update_display(moisture, valve_state);
        }
    }
}

void setup_udp(){
    udp_conn = udp_new();
    if(!udp_conn){
        return;
    }
    udp_bind(udp_conn, IP_ADDR_ANY, UDP_PORT);
    udp_recv(udp_conn, udp_recv_callback, NULL);
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
    eink_init();
    wifi_init();
    setup_udp();

    while (true){
        receive_data();
        update_display(600, 1);
        sleep_ms(1800000);

    }
    return 0;
}

