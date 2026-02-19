#include "bsp_display.h"
#include "ssd1680.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "lwip/udp.h"
#include "lwip/pbuf.h"
#include <string.h>
#include <stdio.h>

static struct udp_pcb *udp_server;

static volatile bool new_data_flag = false;
static volatile udp_data_t latest_data = {0, false, false, false};

static void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port){
    char buf[MAX_MSG_LEN+1];
    int len = p->len < MAX_MSG_LEN ? p->len : MAX_MSG_LEN;
    memcpy(buf, p->payload, len);
    buf[len]='\0';

    int temp_moisture = 0, temp_valve = 0, temp_charging = 0;
    
    if(sscanf(buf, "%d,%d,%d", &temp_moisture, &temp_valve, &temp_charging) == 3){
        latest_data.moisture = temp_moisture;
        latest_data.valve_state = temp_valve > 0;
        latest_data.is_charging = temp_charging > 0;
        latest_data.data_error = false;
    } else {
        latest_data.data_error = true;
    }
    
    new_data_flag = true;
    pbuf_free(p);
}

bool bsp_network_init(void){
    if(cyw43_arch_init_with_country(CYW43_COUNTRY_POLAND)){
        printf("Wifi initialization error\n");
        return false;
    }
    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK);
    struct netif *n = &cyw43_state.netif[CYW43_ITF_AP];
    
    ip4_addr_t ip, netmask, gw;
    IP4_ADDR(&ip, 192, 168, 4, 1);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw, 192, 168, 4, 1);

    netif_set_addr(n, &ip, &netmask, &gw);
    netif_set_up(n);
    
    udp_server = udp_new();
    if(!udp_server || udp_bind(udp_server, IP_ADDR_ANY, LISTEN_PORT) != ERR_OK) {
        return false;
    }
    udp_recv(udp_server, udp_receive_callback, NULL);
    
    return true;
}

void bsp_display_init(void) {
    spi_init(spi0, 10*1000*1000);
    gpio_init(EINK_CS_PIN); gpio_set_dir(EINK_CS_PIN, GPIO_OUT); gpio_put(EINK_CS_PIN, 1);
    gpio_init(DC_PIN); gpio_set_dir(DC_PIN, GPIO_OUT); gpio_put(DC_PIN, 0);
    gpio_init(EN_PIN); gpio_set_dir(EN_PIN, GPIO_OUT); gpio_put(EN_PIN, 1);
    sleep_ms(100);
    gpio_init(RST_PIN); gpio_set_dir(RST_PIN, GPIO_OUT); gpio_put(RST_PIN, 0);
    sleep_ms(20);
    gpio_init(RST_PIN); gpio_set_dir(RST_PIN, GPIO_OUT); gpio_put(RST_PIN, 1);
    sleep_ms(200);
    gpio_init(SRAM_CS_PIN); gpio_set_dir(SRAM_CS_PIN, GPIO_OUT); gpio_put(SRAM_CS_PIN, 1);
    gpio_init(BUSY_PIN); gpio_set_dir(BUSY_PIN, GPIO_IN);

    gpio_set_function(SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    epd_init();

    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_wrap(slice, PWM_WRAP);
    pwm_set_clkdiv(slice, 50.0f);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER_PIN), 0);
    pwm_set_enabled(slice, false);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);
}

bool bsp_get_new_message(udp_data_t *data) {
    if (new_data_flag && !latest_data.data_error) {
        *data = latest_data;
        new_data_flag = false;
        return true;
    }
    return false;
}

void bsp_buzzer_set(bool state) {
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);

    if (state) {
        uint32_t level = (PWM_WRAP+1) * BUZZ_DUTY_PC / 100;
        pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER_PIN), level);
        pwm_set_enabled(slice, true);
    }
    else {
        pwm_set_enabled(slice, false);
        pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER_PIN), 0);
    }
}

bool bsp_button_is_pressed(void){
    static int last_stable_state = 1;
    static uint32_t last_change_ms = 0;
    static bool reported = false;

    int raw = gpio_get(BUTTON_PIN);
    uint32_t now_ms = time_us_64() / 1000ULL;

    if(raw != last_stable_state){
       last_change_ms = now_ms;
       last_stable_state = raw;
       reported = false;
       return false; 
    }
    else {
        if(now_ms - last_change_ms >= BUTTON_DEBOUNCE_MS){
            if(raw == 0 && !reported){
                reported = true;
                return true;
            }
            else if(raw == 1) {
                reported = false;
            }
        }
        return false;
    }
}


