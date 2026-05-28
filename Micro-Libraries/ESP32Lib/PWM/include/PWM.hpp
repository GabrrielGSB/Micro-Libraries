// PWM.hpp ---------------------------------------------------------
#pragma once
#ifndef PWM_HPP
#define PWM_HPP

#include "GPIO.hpp"
#include "driver/ledc.h"
#include "esp_log.h"

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
    PWM(gpio_num_t numPino, uint32_t freq_hz = 5000, ledc_timer_bit_t res_bits = LEDC_TIMER_10_BIT);
    
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