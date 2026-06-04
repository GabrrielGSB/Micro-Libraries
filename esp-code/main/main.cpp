#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "MemoryProfiler.hpp"

extern "C" void app_main(void) {
    // Instanciação da nossa classe profiler
    MemoryProfiler profiler;
    
    uint32_t contador_ciclo = 1;

    while (true) {
        // Criamos um rótulo dinâmico para sabermos a qual ciclo pertence o JSON enviado
        char tag_teste[20];
        snprintf(tag_teste, sizeof(tag_teste), "Ciclo_%lu", contador_ciclo++);
        
        // Dispara o relatório estático estruturado para a linha Serial
        profiler.enviarSnapshotEstatico(tag_teste);
        
        // Aguarda 5 segundos utilizando o FreeRTOS para o próximo envio
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}