// I2C.hpp
#pragma once
#ifndef I2C_HPP
#define I2C_HPP

#include "driver/i2c.h"
#include "esp_err.h"
#include "GPIO.hpp"

// Frequências Padrão do Barramento I2C
#define I2C_STANDARD_MODE 100000 // 100 kHz (Para sensores mais antigos/lentos)
#define I2C_FAST_MODE     400000 // 400 kHz (Ideal para o display SSD1306)

class I2C {
protected:
    i2c_port_t porta;

    // Variáveis estáticas globais. 
    // Servem como "memória" para a classe saber se o hardware 
    // já foi configurado por outro dispositivo no mesmo barramento.
    static bool porta_0_inicializada;
    static bool porta_1_inicializada;

public:
    // Construtor: Prepara os pinos e o hardware nativo
    I2C(i2c_port_t porta_i2c, gpio_num_t pino_sda, gpio_num_t pino_scl, uint32_t frequencia_hz = I2C_FAST_MODE);

    // Destrutor: Desliga a porta se o objeto for destruído
    ~I2C();

    // ==========================================
    // MÉTODOS DE COMUNICAÇÃO
    // ==========================================

    // Envia um único byte de comando ou dado
    esp_err_t escrever(uint8_t endereco_dispositivo, uint8_t registo, uint8_t dado);
    esp_err_t escrever(uint8_t endereco_dispositivo, uint16_t registrador, uint8_t dado);
    
    // Envia um pacote completo de dados (Essencial para desenhar pixels no ecrã SSD1306)
    esp_err_t escrever_bloco(uint8_t endereco_dispositivo, uint8_t registo, const uint8_t *dados, size_t tamanho);

    // Lê informações de sensores (Ex: pedir a temperatura a um sensor I2C)
    esp_err_t ler(uint8_t endereco_dispositivo, uint8_t registo, uint8_t *buffer_recepcao, size_t tamanho);
    esp_err_t ler(uint8_t endereco_dispositivo, uint16_t registrador, uint8_t *buffer_recepcao, size_t tamanho);
    
    // Método utilitário para classes externas (como o display) saberem qual porta usar
    i2c_port_t getPorta();


};

#endif