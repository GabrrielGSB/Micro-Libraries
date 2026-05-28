#include <stdio.h>
#include "Botao.hpp"

volatile bool botao_foi_apertado = false;

static void interrup() {
    botao_foi_apertado = true; 
}

extern "C" void app_main() {
    Botao btt(GPIO4); 
    
    btt.definirInterrupcao(interrup);
    
    while(1) {
        if (botao_foi_apertado) {
            printf("Botão clicado perfeitamente (sem ruídos)!\n");
            botao_foi_apertado = false;
        }
        delay_ms(50);
    }
}