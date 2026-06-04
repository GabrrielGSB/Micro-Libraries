#include "SSD1306.hpp"
#include "Tempo.hpp"
#include <stdio.h>

extern "C" void app_main() {
    SSD1306 display(I2C_NUM_0, GPIO20, GPIO21);
    display.inicializar();

    char buf[32];
    int contador = 0;

    while (true) {
        snprintf(buf, sizeof(buf), "Count: %d", contador++);

        display.limpar();
        display.escreverTexto(buf, 0, 28);
        display.atualizar();
        
        delay_ms(500);
    }
}