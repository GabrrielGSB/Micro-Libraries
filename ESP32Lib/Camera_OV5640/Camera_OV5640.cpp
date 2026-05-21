// Camera_OV5640.cpp
#include "Camera_OV5640.hpp"
#include "Tempo.hpp" // Para usarmos os métodos delay_ms
#include "esp_log.h"

static const char* TAG = "CAMERA_OV5640";

// 2. O Construtor
Camera_OV5640::Camera_OV5640(PinosCamera pinos, I2C* i2c_configurado, PWM* pwm_configurado) {
    this->mapa_pinos = pinos;
    this->barramento_i2c = i2c_configurado;
    this->gerador_clock = pwm_configurado;
    
    this->inicializada = false;
    this->buffer_foto = nullptr;

    // Instanciamos dinamicamente os pinos de manutenção usando sua classe GPIO
    this->pino_reset = new GPIO(pinos.reset, OUTPUT);
    this->pino_pwdn  = new GPIO(pinos.pwdn, OUTPUT);

    // Estado inicial seguro para os pinos de energia e reset
    this->pino_pwdn->desligar(); // Garante que a câmera não está dormindo
    this->pino_reset->ligar();   // Reset em nível ALTO (fora de reset)
}

Camera_OV5640::~Camera_OV5640() {
    // Se houver uma foto pendente na memória, libera antes de destruir o objeto
    liberarMemoria();
    
    // Desliga o driver da Espressif se ele estiver rodando
    if (this->inicializada) {
        esp_camera_deinit();
    }

    // Libera a memória dos objetos GPIO criados no construtor
    delete this->pino_reset;
    delete this->pino_pwdn;
}

// 3. O Método de Arranque
esp_err_t Camera_OV5640::inicializar(framesize_t resolucao, pixformat_t formato) {
    if (this->inicializada) {
        ESP_LOGW(TAG, "A camera ja foi inicializada anteriormente.");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Iniciando sequencia de boot da OV5640...");

    // Passo A: Sequência de Hardware Reset usando sua HAL de GPIO e Tempo
    this->pino_pwdn->desligar(); // Acorda o chip eletricamente
    delay_ms(10);
    
    this->pino_reset->desligar(); // Entra em modo Reset (Nível Baixo)
    delay_ms(20);
    
    this->pino_reset->ligar();    // Sai do modo Reset (Nível Alto)
    delay_ms(20);

    // Passo B: Ativar o Coração (XCLK) usando a nova função da sua classe PWM
    // A OV5640 opera de forma ideal com um clock externo estável de 20MHz
    this->gerador_clock->habilitarClockCamera(20000000);
    delay_ms(10); // Aguarda o clock estabilizar

    // Passo C: Preencher a estrutura nativa do driver da Espressif com o mapa de pinos
    camera_config_t config = {};
    config.ledc_channel = LEDC_CHANNEL_0; // Canal lógico interno (pode ser qualquer um livre)
    config.ledc_timer   = LEDC_TIMER_0;   // Vinculado ao timer interno do driver
    
    // Vinculação dos 8 pinos paralelos de dados (D0 - D7)
    config.pin_d0    = this->mapa_pinos.d0;
    config.pin_d1    = this->mapa_pinos.d1;
    config.pin_d2    = this->mapa_pinos.d2;
    config.pin_d3    = this->mapa_pinos.d3;
    config.pin_d4    = this->mapa_pinos.d4;
    config.pin_d5    = this->mapa_pinos.d5;
    config.pin_d6    = this->mapa_pinos.d6;
    config.pin_d7    = this->mapa_pinos.d7;
    
    // Vinculação dos pinos de controle de ritmo e clocks
    config.pin_xclk  = this->gerador_clock->getPino(); // Mapeia o pino do seu PWM como XCLK
    config.pin_pclk  = this->mapa_pinos.pclk;
    config.pin_vsync = this->mapa_pinos.vsync;
    config.pin_href  = this->mapa_pinos.href;
    
    // Vinculação do barramento de comandos (I2C / SCCB)
    config.pin_sccb_sda = -1; // Passamos -1 porque sua classe I2C já inicializou
    config.pin_sccb_scl = -1; // o barramento globalmente no hardware!
    
    // Mapeamento dos pinos de gerenciamento de energia na struct nativa
    config.pin_pwdn  = this->mapa_pinos.pwdn;
    config.pin_reset = this->mapa_pinos.reset;
    
    // Configurações de imagem desejadas pelo usuário
    config.xclk_freq_hz = 20000000;
    config.frame_size   = resolucao;
    config.pixel_format = formato;
    config.jpeg_quality = 12; // Qualidade JPEG (1-63, onde menor é melhor qualidade)
    config.fb_count     = 1;  // Quantidade de buffers alocados no DMA (1 é suficiente para fotos)
    
    // CONFIGURAÇÃO CRUCIAL PARA ESP32-S3: Alocação automática em PSRAM externa
    config.fb_location = CAMERA_FB_IN_PSRAM; 
    config.grab_mode   = CAMERA_GRAB_WHEN_EMPTY;

    // Passo D: Invocar o motor oficial da Espressif para orquestrar o DMA e ler os registradores
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha critica ao iniciar o motor esp_camera: 0x%x", err);
        return err;
    }

    // Passo E: Teste de comunicação I2C usando o seu novo método de Leitura de 16-bits
    // O ID do fabricante da OV5640 fica armazenado no registrador de 16-bits 0x300A
    uint8_t chip_id_high = 0;

    uint16_t reg_id = 0x300A; 
    err = this->barramento_i2c->ler(0x3C, reg_id, &chip_id_high, 1);
    
    if (err != ESP_OK || chip_id_high != 0x56) {
        ESP_LOGE(TAG, "Erro de barramento SCCB. Sensor OV5640 nao respondeu com o ID esperado (0x56). Obtido: 0x%X", chip_id_high);
        esp_camera_deinit();
        return ESP_FAIL;
    }

    this->inicializada = true;
    ESP_LOGI(TAG, "Camera OV5640 inicializada e validada com sucesso via I2C!");
    return ESP_OK;
}

// 4. Ações da Câmara
camera_fb_t* Camera_OV5640::capturarFoto() {
    if (!this->inicializada) {
        ESP_LOGE(TAG, "Erro: Tentativa de capturar foto sem inicializar a camera.");
        return nullptr;
    }

    // Garante que não há vazamento de memória liberando o frame anterior se ele ainda existir
    liberarMemoria();

    ESP_LOGD(TAG, "DMA iniciando captura de quadro...");
    // A CPU envia o comando e o hardware LCD_CAM assume, jogando os bytes direto na PSRAM
    this->buffer_foto = esp_camera_fb_get();
    
    if (!this->buffer_foto) {
        ESP_LOGE(TAG, "Falha no DMA: Nao foi possivel capturar o quadro da camera.");
        return nullptr;
    }

    ESP_LOGI(TAG, "Foto capturada! Tamanho: %d bytes. Endereco na PSRAM: %p", 
             this->buffer_foto->len, this->buffer_foto->buf);
             
    return this->buffer_foto;
}

void Camera_OV5640::liberarMemoria() {
    if (this->buffer_foto != nullptr) {
        // Devolve o buffer para o pool do driver e limpa a região ocupada na PSRAM
        esp_camera_fb_return(this->buffer_foto);
        this->buffer_foto = nullptr;
        ESP_LOGD(TAG, "Buffer da PSRAM liberado para a proxima captura.");
    }
}

// 5. Gestão de Energia (Usando a sua classe GPIO)
void Camera_OV5640::dormir() {
    if (this->inicializada) {
        this->pino_pwdn->ligar(); // Coloca o pino PWDN em nível ALTO para desligar os reguladores internos da câmera
        ESP_LOGI(TAG, "Sensor OV5640 em modo Power Down (Economia de Energia).");
    }
}

void Camera_OV5640::acordar() {
    if (this->inicializada) {
        this->pino_pwdn->desligar(); // Retorna o pino PWDN para nível BAIXO
        delay_ms(10);                // Tempo necessário para os estágios internos de tensão se estabilizarem
        ESP_LOGI(TAG, "Sensor OV5640 acordado.");
    }
}