#pragma once
#ifndef MEMORY_PROFILER_HPP
#define MEMORY_PROFILER_HPP

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Classe Profiler de Memória para ESP32 (ESP-IDF)
 * Captura dados estáticos do mapa de compilação e consumos dinâmicos do Heap.
 */
class MemoryProfiler {
public:
    /**
     * @brief Construtor padrão da classe.
     * Não executa inicializações complexas de hardware para manter a neutralidade.
     */
    MemoryProfiler();

    /**
     * @brief Coleta as métricas estáticas equivalentes ao 'idf.py size'
     * e despacha via Serial estruturado em formato estrito de JSON.
     * * @param identificador_teste Tag ou nome amigável para rotular o log enviado.
     */
    void enviarSnapshotEstatico(const char* identificador_teste);
};

#endif 