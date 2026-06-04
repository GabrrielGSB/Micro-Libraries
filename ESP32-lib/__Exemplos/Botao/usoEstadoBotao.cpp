#include "Botao.hpp"
#include "GPIO.hpp"
#include "Tempo.hpp"

extern "C" void app_main() {
    Botao botao(GPIO4);
    GPIO  led(GPIO2, OUTPUT);

    while (true) {
        if (botao.estado() == 0) { // pressionado
            led.ligar();
        } else {
            led.desligar();
        }
        delay_ms(10);
    }
}