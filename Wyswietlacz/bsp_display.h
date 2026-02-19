#ifndef BSP_DISPLAY_H
#define BSP_DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#define WIFI_SSID       "PicoNet"
#define WIFI_PASSWORD   "MosKonewka"
#define LISTEN_PORT     12345
#define MAX_MSG_LEN     32

#define BUZZER_PIN      10
#define BUTTON_PIN      11
#define BUTTON_DEBOUNCE_MS 50
#define BUZZ_DUTY_PC    50
#define PWM_WRAP        1249

typedef struct {
    int moisture;
    bool valve_state;
    bool is_charging;
    bool data_error;
} udp_data_t;

void bsp_display_init(void);

bool bsp_network_init(void);

bool bsp_get_new_message(udp_data_t *data);

void bsp_buzzer_set(bool state);

bool bsp_button_is_pressed(void);

#endif