#pragma once
#ifndef BOTAO_HPP
#define BOTAO_HPP

#include "GPIO.hpp"
#include "Tempo.hpp"

#define RISING  GPIO_INTR_POSEDGE
#define FALLING GPIO_INTR_NEGEDGE
#define CHANGE  GPIO_INTR_ANYEDGE

typedef void (*callback_t)();

class Botao : public GPIO {
protected:
    uint32_t timer;
    uint32_t tempo_debounce;               // Limite de tempo
    volatile uint32_t tempo_ultimo_clique; // Memória do debounce
    int ultimo_estado;
    
    callback_t callback_usuario;           // Guarda a função do usuário


    static void IRAM_ATTR isr_interna(void* arg);
    

public:
    Botao(gpio_num_t numPino, uint32_t debounce_ms = 200);

    int estado();
    int estadoBruto(); 

    void definirInterrupcao(callback_t callback, gpio_int_type_t modo = FALLING);
};

#endif