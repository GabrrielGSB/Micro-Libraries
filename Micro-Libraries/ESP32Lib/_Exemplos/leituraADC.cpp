#include <stdio.h>
#include "ADC.hpp"
#include "Tempo.hpp"

extern "C" void app_main() {
    ADC pot(GPIO34);

    while(1) {
        int leitura_bruta = pot.ler();
        int tensao_real   = pot.ler_mV();

        printf("Valor Bruto: %d | Tensão Real: %d mV\n", leitura_bruta, tensao_real);
        
        delay_ms(50);
    }
}