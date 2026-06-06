#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "MemoryProfiler.hpp"

extern "C" void app_main(void)
{
    //---------------------------------------------------------------------------
    // 1. Inicia o profiling periódico: prefixo "teste", intervalo de 1 segundo
    if (!MemoryProfiler::iniciarAnalise("teste", 1000)) {
        printf("Falha ao iniciar profiling.\n");
        return;
    }
    printf("Profiling iniciado. Aguardando dados...\n");
    //---------------------------------------------------------------------------
    

    // 2. Simula carga de trabalho: aloca e libera blocos de memória
    const size_t blockSize = 10 * 1024;  // 10 KB
    const int numBlocks = 20;            // até 200 KB alocados

    void* blocks[numBlocks] = { nullptr };

    // Fase 1: Alocar gradualmente
    printf("Fase 1: Alocando %d blocos de %d bytes...\n", numBlocks, blockSize);
    for (int i = 0; i < numBlocks; i++) {
        blocks[i] = malloc(blockSize);
        if (blocks[i] == nullptr) {
            printf("Falha ao alocar bloco %d. Parando.\n", i);
            break;
        }
        printf("Bloco %d alocado (%p)\n", i, blocks[i]);
        vTaskDelay(pdMS_TO_TICKS(1000));  // espera 1s entre alocações
    }

    // Fase 2: Manter tudo alocado por 5 segundos
    printf("Fase 2: Mantendo blocos alocados por 5s...\n");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Fase 3: Liberar gradualmente
    printf("Fase 3: Liberando blocos...\n");
    for (int i = 0; i < numBlocks; i++) {
        if (blocks[i] != nullptr) {
            free(blocks[i]);
            blocks[i] = nullptr;
            printf("Bloco %d liberado\n", i);
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }

    printf("Teste concluído. Profiling continua em segundo plano.\n");
    printf("Pressione Ctrl+C para parar.\n");

    // 3. Mantém o programa rodando (a task de profiling continua)
    //    Opcional: após um tempo, pode parar o profiling com:
    //    vTaskDelay(pdMS_TO_TICKS(30000));
    //    MemoryProfiler::pararAnalise();
}