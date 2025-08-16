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

#define EINK_CS 17
#define EINK_DC 16
#define EINK_RST 15
#define EINK_BUSY 14 

#define WIFI_SSID "PicoNet"
#define WIFI_PASSWORD "MosKonewka"
#define LISTEN_PORT 12345
#define MAX_MSG_LEN 32

#define BUZZER_PIN 10
#define BUTTON_PIN 11
#define BUTTON_DEBOUNCE_MS 50
#define BUZZ_DUTY_PC 50
#define BUZZ_TIME_MS 10000

static struct udp_pcb *udp_server;

static volatile bool buzzer_active = false;
static volatile uint32_t buzzer_end_ms = 0;


// =============== E-ink =============================
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

void generate_text_bitmap(int moisture, bool valve){
    text_renderer_init(); // Clear framebuff first

    char line1[32];
    char line2[32];
    snprintf(line1, sizeof(line1), "Moisture: %d", moisture);
    snprintf(line2, sizeof(line2),"Valve: %s", valve ? "ON" : "OFF");
    draw_string(0,0, line1);
    draw_string(0, 10, line2);
    draw_logo();
    epd_display_image(framebuf);
}



// =============== Buzzer =============================
void buzzer_init(){
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(BUZZER_PIN);
    
    uint16_t wrap = 1249;
    pwm_set_wrap(slice, wrap);
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
        uint16_t wrap = pwm_get_wrap(slice);

        uint32_t level = (wrap+1) * BUZZ_DUTY_PC / 100;
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

    int moisture = 0;
    int valve_state = 0;
    if(sscanf(buf, "%d,%d", &moisture, &valve_state) == 2){
        generate_text_bitmap(moisture, valve_state != 0);

        if(valve_state){
            buzzer_start();
        }

    }
    else{
        text_renderer_init();
        draw_string(0,0,"Error");
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

    while (true){
       // Messages received via UDP are handled automatically because of 
       // threadsafe_background mode, and e-ink is automatically updated
       // inside the udp_receive_callback function,
       // therefore while true loop only needs to handle the buzzer

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