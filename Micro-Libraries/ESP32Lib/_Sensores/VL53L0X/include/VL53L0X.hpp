#pragma once

#include "I2C.hpp"
#include "esp_err.h"

class VL53L0X {
private:
    I2C& barramentoI2C;
    uint8_t endereco;
    
    // Registradores básicos do VL53L0X
    static constexpr uint8_t REG_SYSRANGE_START = 0x00;
    static constexpr uint8_t REG_RESULT_RANGE_STATUS = 0x14;

public:
    // Construtor associando a sua classe I2C
    VL53L0X(I2C& i2c, uint8_t endereco_dispositivo = 0x29);

    // Inicialização básica do sensor
    esp_err_t iniciar();

    // Realiza uma leitura única e retorna a distância em milímetros
    uint16_t lerDistancia();
};