#pragma once
#ifndef I2C_HPP
#define I2C_HPP

#include "driver/i2c_master.h" 
#include "esp_err.h"
#include "GPIO.hpp"

#define NORMAL_MODE 100000 
#define FAST_MODE   400000 

class I2C {
protected:
    i2c_port_t porta;
    uint32_t frequencia_padrao;

    static i2c_master_bus_handle_t bus_handle_0;
    static i2c_master_bus_handle_t bus_handle_1;

    i2c_master_dev_handle_t dispositivos[128];

    i2c_master_dev_handle_t obter_dispositivo(uint8_t endereco);

public:
    I2C(i2c_port_t porta_i2c, gpio_num_t pino_sda, gpio_num_t pino_scl, uint32_t frequencia_hz = NORMAL_MODE);
    ~I2C();

    esp_err_t escrever(uint8_t endereco_dispositivo, uint8_t registrador, uint8_t dado);
    esp_err_t escrever(uint8_t endereco_dispositivo, uint16_t registrador, uint8_t dado);
    
    esp_err_t escrever_bloco(uint8_t endereco_dispositivo, uint8_t registrador, const uint8_t *dados, size_t tamanho);

    esp_err_t ler(uint8_t endereco_dispositivo, uint8_t registrador, uint8_t *buffer_recepcao, size_t tamanho);
    esp_err_t ler(uint8_t endereco_dispositivo, uint16_t registrador, uint8_t *buffer_recepcao, size_t tamanho);
    
    i2c_port_t getPorta();
};

#endif