// PWM.hpp ---------------------------------------------------------
#pragma once
#ifndef PWM_HPP
#define PWM_HPP

#include "GPIO.hpp"
#include "driver/ledc.h"
#include "esp_log.h"

#pragma region RESOLUCOES
    #define _1_BIT LEDC_TIMER_1_BIT 
    #define _2_BIT LEDC_TIMER_2_BIT 
    #define _3_BIT LEDC_TIMER_3_BIT 
    #define _4_BIT LEDC_TIMER_4_BIT 
    #define _5_BIT LEDC_TIMER_5_BIT 
    #define _6_BIT LEDC_TIMER_6_BIT 
    #define _7_BIT LEDC_TIMER_7_BIT 
    #define _8_BIT LEDC_TIMER_8_BIT 
    #define _9_BIT LEDC_TIMER_9_BIT 
    #define _10_BIT LEDC_TIMER_10_BIT 
    #define _11_BIT LEDC_TIMER_11_BIT 
    #define _12_BIT LEDC_TIMER_12_BIT
    #define _13_BIT LEDC_TIMER_13_BIT
    #define _14_BIT LEDC_TIMER_14_BIT
#pragma endregion

class PWM : public GPIO {
protected:
    // --- Atributos do Hardware ---
    ledc_channel_t   canal;
    ledc_timer_t     timer;
    uint32_t         frequencia;
    ledc_timer_bit_t resolucao;

    // --- Controle de Alocação Automática ---
    static bool canais_ocupados[LEDC_CHANNEL_MAX]; 
    
    // Métodos internos de gestão do canal
    ledc_channel_t alocarCanalLivre();
    void liberarCanal();

public:
    // Construtor original (mantido para motores, LEDs, etc.)
    PWM(gpio_num_t numPino, uint32_t freq_hz = 5000, 
        ledc_timer_bit_t res_bits = _10_BIT);
    ~PWM();

    // --- Funcionalidades Padrão do PWM ---
    void definirDuty(uint32_t duty);
    
    // O fade transita o duty suavemente usando o próprio circuito do chip
    void fade(uint32_t duty_alvo, uint32_t tempo_ms);
    
    void pausar();
    void retomar();

    void habilitarClockCamera(uint32_t frequencia_hz = 20000000);
};

#endif