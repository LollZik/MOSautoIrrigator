#include "bsp_konewka.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "pico/cyw43_arch.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"

#define SLEEP_TIME 1800000 // 30min

static struct udp_pcb *udp_client;
static ip_addr_t dest_addr;


void bsp_init(void) {
    stdio_init_all();

    // SPI
    spi_init(SPI_PORT, 1000 * 1000);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_init(PIN_CS); 
    gpio_set_dir(PIN_CS, GPIO_OUT); 
    gpio_put(PIN_CS, 1);

    // Decoder
    gpio_init(DEC_ADDR_A);
    gpio_set_dir(DEC_ADDR_A, GPIO_OUT);
    gpio_init(DEC_ADDR_B);
    gpio_set_dir(DEC_ADDR_B, GPIO_OUT);
    gpio_init(DEC_ADDR_C);
    gpio_set_dir(DEC_ADDR_C, GPIO_OUT);
    gpio_init(DEC_ENABLE);
    gpio_set_dir(DEC_ENABLE, GPIO_OUT); 
    gpio_put(DEC_ENABLE, 0);

    // Power management
    gpio_init(PIN_12V_ENABLE);
    gpio_set_dir(PIN_12V_ENABLE, GPIO_OUT);
    gpio_put(PIN_12V_ENABLE, 0); // Turned off

    gpio_init(PIN_CHG_STATUS);
    gpio_set_dir(PIN_CHG_STATUS, GPIO_IN);
}

// helper function
static int _read_adc_raw(uint8_t channel) {
    uint8_t tx[3] = {
        0x01,
        (uint8_t)(0x80 | ((channel & 0x07) << 4)),
        0x00
    };
    uint8_t rx[3] = {0};

    gpio_put(PIN_CS, 0);
    spi_write_read_blocking(SPI_PORT, tx, rx, 3);
    gpio_put(PIN_CS, 1);

    return ((rx[1] & 0x03) << 8) | rx[2];
}

uint16_t bsp_read_moisture(uint8_t channel_id) {
    long sum = 0;
    const int samples = 10;
    // Return the average of 10 samples
    for(int i=0; i<samples; i++){
        sum += _read_adc_raw(channel_id);
        sleep_ms(2);
    }
    return (uint16_t)(sum / samples);
}

bool bsp_is_battery_charging(void) {
    return gpio_get(PIN_CHG_STATUS); // Assuming charger is not open-drain
}

void bsp_pump_on(uint8_t channel_id) {
    if (channel_id >= MAX_PUMPS) return;

    gpio_put(DEC_ADDR_A, (channel_id & 0x01));
    gpio_put(DEC_ADDR_B, (channel_id & 0x02));
    gpio_put(DEC_ADDR_C, (channel_id & 0x04));

    gpio_put(PIN_12V_ENABLE, 1);
    sleep_ms(20);

    gpio_put(DEC_ENABLE, 1);
}

void bsp_pumps_off(void) {
    gpio_put(DEC_ENABLE, 0);
    gpio_put(PIN_12V_ENABLE, 0); // For energy efficiency
}


bool bsp_network_connect(void) {
    if (cyw43_arch_init_with_country(CYW43_COUNTRY_POLAND)) return false;
    cyw43_arch_enable_sta_mode();

    // Static IP Setup
    struct netif *n = &cyw43_state.netif[CYW43_ITF_STA];
    ip4_addr_t ip, netmask, gw;
    IP4_ADDR(&ip, 192, 168, 4, 2);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 4, 1);
    netif_set_addr(n, &ip, &netmask, &gw);
    netif_set_up(n);

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        return false;
    }

    // UDP init
    udp_client = udp_new();
    ipaddr_aton(DEST_IP, &dest_addr);
    return true;
}

void bsp_send_message(uint16_t moisture, bool valve_state, bool is_charging){
    if (!udp_client) return;
    char msg[64];
    int len = snprintf(msg, sizeof(msg), "%d,%d,%d", moisture, valve_state, is_charging);

    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (!p) return;
    memcpy(p->payload, msg, len);
    udp_sendto(udp_client, p, &dest_addr, DEST_PORT);
    pbuf_free(p);
}

void bsp_sleep_ms(uint32_t ms) {
    sleep_ms(ms);
}
