#pragma once

#ifndef BOTAO_HPP
#define BOTAO_HPP

#include "GPIO.hpp"
#include "Tempo.hpp"

#include "freertos/FreeRTOS.h" // Adicionando o prefixo da pasta nativa
#include "freertos/semphr.h"
#include "freertos/task.h"

#define RISING  GPIO_INTR_POSEDGE
#define FALLING GPIO_INTR_NEGEDGE
#define CHANGE  GPIO_INTR_ANYEDGE

typedef void (*callback_t)();

class Botao : public GPIO {
protected:
    uint32_t timer;
    bool ultimo_estado;

    uint32_t tempo_debounce; 
    callback_t callback_usuario; 

    SemaphoreHandle_t xSemaforoDebounce;
    TaskHandle_t xTaskHandle;

    static void IRAM_ATTR isr_interna(void* arg);
    
    static void pressionado(void* arg);

public:
    Botao(gpio_num_t numPino, uint32_t debounce_ms = 50); 
    ~Botao(); 

    int estado();

    void definirInterrupcao(callback_t callback, gpio_int_type_t modo = FALLING);
};

#endif