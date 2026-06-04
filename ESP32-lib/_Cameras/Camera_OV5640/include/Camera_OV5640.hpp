// Camera_OV5640.hpp
#pragma once
#ifndef CAMERA_OV5640_HPP
#define CAMERA_OV5640_HPP

#include "esp_camera.h" // O motor oficial da Espressif (que fará a gestão do DMA)
#include "I2C.hpp"
#include "PWM.hpp"
#include "GPIO.hpp"

// 1. Estrutura de Pinos (A "Ficha" de Ligação)
// Agrupamos todos os pinos aqui para manter o construtor limpo e organizado.
struct PinosCamera {
    // Pinos de Dados (O autocarro de 8 bits)
    gpio_num_t d0, d1, d2, d3, d4, d5, d6, d7;
    
    // Pinos de Sincronia (O ritmo da imagem)
    gpio_num_t vsync; // Começo da foto
    gpio_num_t href;  // Começo da linha
    gpio_num_t pclk;  // Leitura do pixel
    
    // Pinos de Manutenção
    gpio_num_t reset; // Hard reset
    gpio_num_t pwdn;  // Power down (Dormir)
};

class Camera_OV5640 {
protected:
    // --- Ferramentas Injetadas ---
    // Guardamos ponteiros para não recriar o I2C e o PWM, 
    // mas sim usar os que você já criou no main.cpp
    I2C* barramento_i2c; 
    PWM* gerador_clock;

    // --- Pinos de Controle Direto ---
    GPIO* pino_reset;
    GPIO* pino_pwdn;

    // --- Estado do Sistema ---
    bool inicializada;
    camera_fb_t* buffer_foto; // O ponteiro para a foto salva na PSRAM

    // Configuração dos pinos que o utilizador vai fornecer
    PinosCamera mapa_pinos;

public:
    // 2. O Construtor
    // Recebe a planta dos pinos, e os endereços de memória das ferramentas prontas
    Camera_OV5640(PinosCamera pinos, I2C* i2c_configurado, PWM* pwm_configurado);
    
    ~Camera_OV5640();

    // 3. O Método de Arranque
    // É aqui que chamaremos o motor esp32-camera para iniciar o DMA e o sensor
    esp_err_t inicializar(framesize_t resolucao = FRAMESIZE_VGA, pixformat_t formato = PIXFORMAT_JPEG);

    // 4. Ações da Câmara
    camera_fb_t* capturarFoto(); // Tira a foto e devolve o ficheiro da PSRAM
    void liberarMemoria();       // OBRIGATÓRIO: Limpa a PSRAM após o uso da foto

    // 5. Gestão de Energia (Usando a sua classe GPIO)
    void dormir();
    void acordar();
};

#endif
