#include "SSD1306.hpp"

extern "C" void app_main() {
    SSD1306 display(I2C_NUM_0, GPIO20, GPIO21);
    
    display.inicializar();

    display.limpar();
    display.escreverTexto("Ola, Mundo!", 0, 28);
    display.atualizar();
}