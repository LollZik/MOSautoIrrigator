#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"


//Pins
#define MOISTURE_SENSOR_PIN 26
#define VALVE_PIN 2


#define MOISTURE_THRESHOLD 500 //0-4095


void setup(){
    stdio_init_all();
    
    adc_init();
    adc_gpio_init(MOISTURE_SENSOR_PIN);
    adc_select_input(0);

    gpio_init(VALVE_PIN);
    gpio_set_dir(VALVE_PIN, GPIO_OUT);
    gpio_put(VALVE_PIN,0); //Closed by default
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

int main(){
    setup();

    while (true) {
        int moisture = read_moisture();
        watering_controls(moisture);
        sleep_ms(1800000); //Potential watering once every 30min - its still more often than required. Although idk how often pico will need to 
    }

    return 0;
}
