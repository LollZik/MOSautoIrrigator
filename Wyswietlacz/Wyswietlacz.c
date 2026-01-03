#include <stdio.h>
#include <string.h>
#include <ssd1680.h>
#include <text_renderer.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "lwip/udp.h"
#include "lwip/ip_addr.h"
#include "lwip/pbuf.h"

#ifdef CYW43_WL_GPIO_LED_PIN
#include "pico/cyw43_arch.h"
#endif

#define SPI_RX_PIN      16
#define SPI_SCK_PIN     18
#define SPI_TX_PIN      19

#define DC_PIN          20
#define RST_PIN         21
#define BUSY_PIN        22
#define EN_PIN          26
#define SRAM_CS_PIN     27
#define EINK_CS_PIN     17

#define WIFI_SSID     "PicoNet"
#define WIFI_PASSWORD "MosKonewka"
#define LISTEN_PORT 12345
#define MAX_MSG_LEN 32

#define BUZZER_PIN 10
#define BUTTON_PIN 11
#define BUTTON_DEBOUNCE_MS 50
#define BUZZ_DUTY_PC 50
#define BUZZ_TIME_MS 10000
#define PWM_WRAP 1249

static struct udp_pcb *udp_server;

static volatile bool new_data_received = true;
static volatile bool data_error = false;
static volatile int moisture = 0;
static volatile bool valve_state = true;


static volatile bool buzzer_active = false;
static volatile uint32_t buzzer_end_ms = 0;


// =============== E-ink =============================
void eink_init(){
    spi_init(spi0, 10*1000*1000);

    gpio_init(EINK_CS_PIN);
    gpio_set_dir(EINK_CS_PIN, GPIO_OUT);
    gpio_put(EINK_CS_PIN, 1);

    gpio_init(DC_PIN);
    gpio_set_dir(DC_PIN, GPIO_OUT);
    gpio_put(DC_PIN, 0);

    gpio_init(EN_PIN);
    gpio_set_dir(EN_PIN, GPIO_OUT);
    gpio_put(EN_PIN, 1);

    sleep_ms(100);

    gpio_init(RST_PIN);
    gpio_set_dir(RST_PIN, GPIO_OUT);
    gpio_put(RST_PIN, 0);

    sleep_ms(20);

    gpio_init(RST_PIN);
    gpio_set_dir(RST_PIN, GPIO_OUT);
    gpio_put(RST_PIN, 1);

    sleep_ms(200);

    gpio_init(SRAM_CS_PIN);
    gpio_set_dir(SRAM_CS_PIN, GPIO_OUT);
    gpio_put(SRAM_CS_PIN, 1);

    gpio_init(BUSY_PIN);
    gpio_set_dir(BUSY_PIN, GPIO_IN);

    gpio_set_function(SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI_SCK_PIN, GPIO_FUNC_SPI);

    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    epd_init();
}

void generate_text_bitmap(int moisture, bool valve){
    text_renderer_init(); // Clear framebuff first
    char line1[32];
    char line2[32];
    snprintf(line1, sizeof(line1), "Moisture: %d", moisture);
    snprintf(line2, sizeof(line2),"Valve: %s", valve ? "ON" : "OFF");
    draw_string(10,40, line1);
    draw_string(10, 60, line2);
    draw_logo();
    epd_display_image(framebuf);
}



// =============== Buzzer =============================
static uint slice;
void buzzer_init(){
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    
    pwm_set_wrap(slice, PWM_WRAP);
    pwm_set_clkdiv(slice, 50.0f);

    pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER_PIN),0);
    pwm_set_enabled(slice, false);

}

void buzzer_start(){
    uint32_t now_ms = time_us_64() / 1000ULL;
    buzzer_end_ms = now_ms + BUZZ_TIME_MS;

    if(!buzzer_active){
        buzzer_active = true;
        uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);

        uint32_t level = (PWM_WRAP+1) * BUZZ_DUTY_PC / 100;
        pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER_PIN), level);
        pwm_set_enabled(slice, true);
    }
}

void stop_buzzer(){
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    pwm_set_enabled(slice, false);
    pwm_set_chan_level(slice, pwm_gpio_to_channel(BUZZER_PIN), 0);
    buzzer_active = false;
    buzzer_end_ms = 0;
}

// =============== Button =============================

void button_init(){
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);           // Active low button
}

bool button_pressed(){
    static int last_stable_state = 1;
    static uint32_t last_change_ms = 0;
    static bool reported = false;

    int raw = gpio_get(BUTTON_PIN);
    uint32_t now_ms = time_us_64() / 1000ULL;

    if(raw != last_stable_state){
       // State has changed, reset debouncing timer
       last_change_ms = now_ms;
       last_stable_state = raw;
       reported = false;
       return false; 
    }
    else{
        if(now_ms - last_change_ms >= BUTTON_DEBOUNCE_MS){
            if(raw == 0 && !reported){
                reported = true;
                return true;
            }
            else if(raw == 1){
                reported = false;
            }
        }
        return false;
    }
}


// =============== WIFI & UDP =============================
bool setup_wifi(){
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
    return true;
}

static void udp_receive_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port){
    char buf[MAX_MSG_LEN+1];
    int len = p->len < MAX_MSG_LEN ? p->len : MAX_MSG_LEN;
    memcpy(buf, p->payload, len);
    buf[len]='\0';

    int temp_moisture = 0;
    int temp_valve_state = 0;
    if(sscanf(buf, "%d,%d", &temp_moisture, &temp_valve_state) == 2){
        moisture = temp_moisture;
        valve_state = temp_valve_state;
        data_error = false;
        new_data_received = true;
    }
    else{
        data_error = true;
        new_data_received = true;
    }
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

int main(){
    stdio_init_all();

    if(!setup_wifi()){
        return -1;
    }
    if(!setup_udp()){
        return -1;
    }
    eink_init();

    buzzer_init();
    button_init();

    
    buzzer_start();
    sleep_ms(1000);
    stop_buzzer();

    generate_text_bitmap(9999, true);
    epd_display_image(framebuf);

    while (true){
       // Messages received via UDP are handled automatically because of 
       // threadsafe_background mode, we only need to check the values of
       // static values callback changes and act accordingly

       if(new_data_received){
        new_data_received = false;
        if(!data_error){
            generate_text_bitmap(moisture, valve_state);

            if(valve_state){
                buzzer_start();
            }
        }
        else{
            text_renderer_init();
            draw_string(0,0,"Error");
            epd_display_image(framebuf);
        }
       }

        if(buzzer_active){ 
            uint32_t now = (uint32_t) (time_us_64()/1000UL);

            if((int32_t)(now - buzzer_end_ms) >= 0){
                stop_buzzer();
            }
            else if(button_pressed()){
                stop_buzzer();
            }
        }
        else{
            (void)button_pressed(); // Update static variables inside the function
        }
        sleep_ms(10); // Slight delay so as not to overload the system
    }

    return 0;
}