#include "VL53L0X.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "VL53L0X_Driver";

VL53L0X::VL53L0X(I2C& i2c, uint8_t endereco_dispositivo) 
    : barramentoI2C(i2c), endereco(endereco_dispositivo) {}

esp_err_t VL53L0X::iniciar() {
    // Verificação básica de comunicação testando a leitura de um registrador padrão
    uint8_t revisao_id = 0;
    esp_err_t err = barramentoI2C.ler(endereco, (uint8_t)0xC0, &revisao_id, 1);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Não foi possível comunicar com o VL53L0X no endereço 0x%02X", endereco);
        return err;
    }

    ESP_LOGI(TAG, "Sensor VL53L0X detectado com sucesso!");

    // Nota: Em um driver comercial, aqui entrariam cerca de 30 a 40 escritas
    // de calibração que a STMicroelectronics fornece na API oficial deles.
    // Para testes em bancada curta, o padrão de fábrica funcional resolve.
    
    // Configura o sensor para o modo de tensão padrão interno (2V8)
    barramentoI2C.escrever(endereco, (uint8_t)0x89, (uint8_t)0x01);
    
    return ESP_OK;
}

uint16_t VL53L0X::lerDistancia() {
    // 1. Inicia a medição (Single Measurement Mode)
    barramentoI2C.escrever(endereco, REG_SYSRANGE_START, (uint8_t)0x01);

    // 2. Aguarda o bit de pronto (polling simples com timeout seguro)
    uint8_t pronto = 0;
    int tentativas = 0;
    do {
        // Lendo o status do registrador de resultado
        barramentoI2C.ler(endereco, REG_RESULT_RANGE_STATUS, &pronto, 1);
        vTaskDelay(pdMS_TO_TICKS(5));
        tentativas++;
    } while ((pronto & 0x01) == 0 && tentativas < 50);

    if (tentativas >= 50) {
        ESP_LOGW(TAG, "Timeout esperando leitura do sensor.");
        return 0xFFFF; // Retorna valor de erro
    }

    // 3. O VL53L0X retorna a distância em 16 bits (2 bytes) a partir do registrador 0x1E
    uint8_t buffer_dados[2] = {0, 0};
    esp_err_t err = barramentoI2C.ler(endereco, (uint8_t)0x1E, buffer_dados, 2);

    if (err != ESP_OK) {
        return 0xFFFF;
    }

    // Reconstrói o valor de 16 bits (MSB << 8 | LSB)
    uint16_t distancia_mm = (buffer_dados[0] << 8) | buffer_dados[1];

    return distancia_mm;
}