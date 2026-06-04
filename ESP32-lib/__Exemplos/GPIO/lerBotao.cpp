#include "GPIO.hpp"
#include "Tempo.hpp"

extern "C" void app_main() {
    GPIO led(GPIO42, OUTPUT);
    GPIO botao(GPIO40, INPUT, PULLUP);

    while (true) {
        if (botao.ler() == 0) { // botão pressionado (ativo baixo)
            led.ligar();
        } else {
            led.desligar();
        }
        delay_ms(10); // debounce simples
    }
}