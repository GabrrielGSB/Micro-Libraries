// ADC.hpp
#pragma once
#ifndef ADC_HPP
#define ADC_HPP

#include "GPIO.hpp"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_continuous.h" // Incluído para o DMA

#define _0DB   ADC_ATTEN_DB_0
#define _2_5DB ADC_ATTEN_DB_2_5
#define _6DB   ADC_ATTEN_DB_6
#define _12DB  ADC_ATTEN_DB_12

struct EstatisticaADC {
    uint32_t minimo;
    uint32_t maximo;
    float rms;
    int amostras_lidas;
};

class ADC : public GPIO {
protected:
    // ==========================================
    //      CONFIGURAÇÕES GERAIS (Compartilhadas)
    // ==========================================
    adc_unit_t unidade;
    adc_channel_t canal;
    adc_atten_t atenuacao;
    
    // ==========================================
    //      VARIÁVEIS: MODO ONE-SHOT
    // ==========================================
    adc_cali_handle_t handle_calibracao;
    bool calibrado;
    bool oneshot_inicializado; // <--- NOVIDADE AQUI

    static adc_oneshot_unit_handle_t handle_unidade_adc1;
    static adc_oneshot_unit_handle_t handle_unidade_adc2;

    void inicializar_calibracao();
    void inicializar_oneshot();    // <--- NOVIDADE AQUI

    // ==========================================
    //      VARIÁVEIS: MODO CONTÍNUO (DMA)
    // ==========================================
    adc_continuous_handle_t handle_continuo;
    uint32_t tamanho_buffer;
    bool modo_continuo_ativo;

    void configLeituraContinua(uint32_t amostragem_hz = 20000, uint32_t tamanho_buf = 1024);

public:
    // Niveis de atenuação padrão: 0dB (0-800mV), 2.5dB (0-1100mV), 6dB (0-1350mV), 12dB (0-3.1mV)
    ADC(gpio_num_t numPino, adc_atten_t atenuacao_db = _12DB);
    ~ADC();

    // ==========================================
    //      MÉTODOS: MODO ONE-SHOT
    // ==========================================
    int ler();         // Retorna o valor cru (0 a 4095)
    int ler_mV();      // Retorna o valor real em milivolts (mV) calibrado

    // ==========================================
    //      MÉTODOS: MODO CONTÍNUO
    // ==========================================
    
    void comecarLeitura(uint32_t amostragem_hz = 20000, uint32_t tamanho_buf = 1024);
    void pararLeitura();
    EstatisticaADC lerLeituras(); 
};

#endif