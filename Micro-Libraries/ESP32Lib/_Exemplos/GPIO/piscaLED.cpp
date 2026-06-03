#include "GPIO.hpp"
#include "Tempo.hpp"

extern "C" void app_main() {
    GPIO led(GPIO2, OUTPUT);

    while (true) {
        led.inverter();
        delay_ms(500); // 500 ms
    }
}