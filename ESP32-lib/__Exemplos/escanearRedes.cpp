#include "esp_log.h"
#include "Network.hpp"

static const char* TAG = "MAIN_APP";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "Inicializando Aplicacao de Varredura Wi-Fi...");

    // Instancia o gerenciador de rede
    Network gerenciador_rede;

    // Aguarda estabilização inicial do chip
    vTaskDelay(pdMS_TO_TICKS(2000));

    while (true) {
        // Toda a lógica de varredura, limpeza e print de tabela agora é interna da classe!
        gerenciador_rede.escanear();

        // Aguarda 15 segundos antes de atualizar a tela novamente
        ESP_LOGI(TAG, "Aguardando 15 segundos para a próxima varredura...");
        vTaskDelay(pdMS_TO_TICKS(15000));
    }
}