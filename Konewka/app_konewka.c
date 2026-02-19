#include "bsp_konewka.h"
#define SLEEP_TIME 5000

static uint16_t current_moisture = 0;
static bool is_charging = false;
static bool valve_active = false;
static uint8_t battery_percent = 0;

void task_read_sensors(uint8_t channel_id) {
    current_moisture = bsp_read_moisture(channel_id);
    is_charging = bsp_is_battery_charging();
}

void task_watering_logic(void) {
    if (current_moisture < MOISTURE_THRESHOLD) {
        bsp_pump_on(0);
        valve_active = true;
        
        bsp_sleep_ms(WATERING_TIME); 
        
        bsp_pumps_off();
    } else {
        bsp_pumps_off();
        valve_active = false;
    }
}

void task_network_send(void) {
    bsp_send_message(current_moisture, valve_active, is_charging);
}