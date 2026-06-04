#include "SSD1306.hpp"

extern "C" void app_main() {
    SSD1306 display(I2C_NUM_0, GPIO21, GPIO22);
    display.inicializar();

    display.limpar();

    // Borda superior e inferior
    for (int x = 0; x < 128; x++) {
        display.desenharPixel(x, 0,  true);
        display.desenharPixel(x, 63, true);
    }

    // Borda esquerda e direita
    for (int y = 0; y < 64; y++) {
        display.desenharPixel(0,   y, true);
        display.desenharPixel(127, y, true);
    }

    display.escreverTexto("ESP32", 44, 28);
    display.atualizar();
}