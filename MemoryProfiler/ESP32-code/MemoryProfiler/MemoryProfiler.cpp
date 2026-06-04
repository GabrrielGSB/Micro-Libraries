#include "MemoryProfiler.hpp"
#include <stdio.h>
#include "esp_heap_caps.h"

// Vinculação dos símbolos externos gerados pela tabela de mapeamento do Linker do ESP-IDF
extern const int _data_start;
extern const int _data_end;
extern const int _bss_start;
extern const int _bss_end;
extern const int _iram_start;
extern const int _iram_end;

MemoryProfiler::MemoryProfiler() {
    // Inicialização neutra
}

void MemoryProfiler::enviarSnapshotEstatico(const char* identificador_teste) {
    // Cálculo matemático de endereços (Fim - Início) para descobrir o tamanho real das seções em Bytes
    size_t tamanho_data = (size_t)(&_data_end - &_data_start) * sizeof(int);
    size_t tamanho_bss  = (size_t)(&_bss_end  - &_bss_start)  * sizeof(int);
    size_t tamanho_iram = (size_t)(&_iram_end - &_iram_start) * sizeof(int);
    
    // Captura adicional da capacidade total restante das memórias internas no momento atual
    size_t sram_total  = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
    // size_t sram2_total = heap_caps_get_total_size(MALLOC_CAP_8BIT | MALLOC_CAP_NON_DIRAM);  // DRAM (EXCLUSIVA)
    // size_t sram0_total = heap_caps_get_total_size(MALLOC_CAP_EXEC | MALLOC_CAP_NON_DIRAM);  // IRAM (EXCLUSIVA)
    // size_t sram1_total = heap_caps_get_total_size(MALLOC_CAP_8BIT | MALLOC_CAP_EXEC);       // SRAM (HEAP COMPARTILHADA)

    // size_t sram_interna_livre  = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    // size_t psram_externa_livre = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    // Delimitador inicial exclusivo para o script Python interceptar na Serial e ignorar outros logs comuns
    printf("\n[MP_JSON_START]");
    
    // Formatação direta do JSON via printf sem instanciar objetos std::string (vazamento zero de RAM temporária)
    printf("{"
           "\"teste\":\"%s\","
           "\"sram_total\":%u,"
        //    "\"dram_total\":%u,"
        //    "\"iram_total\":%u,"
        //    "\"sram_comp_total\":%u,"
        //    "\"dram_data_usado\":%u,"
        //    "\"dram_bss_usado\":%u,"
        //    "\"iram_usado\":%u,"
        //    "\"heap_livre\":%u,"
        //    "\"psram_livre\":%u"
           "}", 
           identificador_teste,
           sram_total
        //    dram_total,
        //    iram_total,
        //    tamanho_data, 
        //    tamanho_bss, 
        //    tamanho_iram,
        //    sram_interna_livre,
        //    psram_externa_livre
);
           
    // Delimitador final para validação de pacote completo no script receptor
    printf("[MP_JSON_END]\n");
}