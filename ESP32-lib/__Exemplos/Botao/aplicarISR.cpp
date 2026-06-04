#include "GPIO.hpp"
#include "Tempo.hpp"
#include "Botao.hpp"

GPIO led(GPIO42, OUTPUT);
Botao botao(GPIO40);

void ao_pressionar();

extern "C" void app_main() {
    botao.definirInterrupcao(ao_pressionar); // borda de descida (padrão)
}

void ao_pressionar() {
    led.inverter(); // curto e seguro dentro de ISR
}
