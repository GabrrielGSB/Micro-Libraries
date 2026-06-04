#include "PWM.hpp"
#include "Tempo.hpp"

extern "C" void app_main() {
    PWM led(GPIO42); // 5 kHz, 10 bits

    while (true) {
        led.fade(1023, 3000); // acende em 1 s
        delay_ms(1200);
        led.fade(0, 3000);    // apaga em 1 s
        delay_ms(1200);
    }
}