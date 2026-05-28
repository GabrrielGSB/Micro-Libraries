#include "I2C.hpp"
#include "SSD1306.hpp"

extern "C" void app_main() {
    // 1. Inicia o Barramento I2C na porta 0, com pinos SDA 21, SCL 22 a 400kHz(necessário para o display)
    I2C meu_barramento(I2C_NUM_0, GPIO14, GPIO21, I2C_FAST_MODE);

    // 2. Inicia o Display entregando o endereço daquele barramento para ele usar
    SSD1306 oled(&meu_barramento, 128, 64);
    oled.inicializar();

    oled.escreverTexto("Hello, ESP32", 0,0);
    oled.desenharChar('?', 0, 32);
    oled.desenharChar('%', 12, 32);
    // oled.desenhar_pixel(64, 32, true); // Acende o pixel central
    
    oled.atualizar();
}