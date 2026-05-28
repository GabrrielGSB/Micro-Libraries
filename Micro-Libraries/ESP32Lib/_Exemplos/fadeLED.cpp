#include <stdio.h>
#include "esp_log.h"
#include "PWM.hpp" 
#include "Tempo.hpp"

extern "C" void app_main(void) {
    ESP_LOGI("MAIN", "Iniciando teste de PWM (Fade)");
    PWM led(GPIO14, 5000);

    while (true) {
        ESP_LOGI("MAIN", "Iniciando Fade UP (Aumentando brilho)...");
        // Vai para o duty máximo (1023) em 2000 milissegundos (2 segundos)
        led.fade(1023, 2000);
        
        // Como o fade() não trava o processador, precisamos usar um delay
        // para dar tempo do efeito terminar antes de mandar o próximo comando.
        // Durante esse delay, o FreeRTOS poderia estar rodando outras tasks!
        delay_ms(2000); 


        ESP_LOGI("MAIN", "Iniciando Fade DOWN (Diminuindo brilho)...");
        // Retorna para o duty zero (desligado) em 2000 milissegundos
        led.fade(0, 2000);
        
        // Aguarda terminar o efeito novamente
        delay_ms(2000);
    }
}