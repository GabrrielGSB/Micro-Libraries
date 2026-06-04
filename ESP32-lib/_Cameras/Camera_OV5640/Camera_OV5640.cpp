// Camera_OV5640.cpp
#include "Camera_OV5640.hpp"
#include "Tempo.hpp" 
// #include "esp_log.h"

// static const char* TAG = "CAMERA_OV5640";

// O Construtor
Camera_OV5640::Camera_OV5640(PinosCamera pinos, I2C* i2c_configurado, PWM* pwm_configurado) {
    this->mapa_pinos = pinos;
    this->barramento_i2c = i2c_configurado;
    this->gerador_clock = pwm_configurado;
    
    this->inicializada = false;
    this->buffer_foto = nullptr;

    // Instanciamento usando sua HAL de GPIO
    this->pino_reset = new GPIO(pinos.reset, OUTPUT);
    this->pino_pwdn  = new GPIO(pinos.pwdn, OUTPUT);

    this->pino_pwdn->desligar(); 
    this->pino_reset->ligar();   
}

Camera_OV5640::~Camera_OV5640() {
    liberarMemoria();
    if (this->inicializada) {
        esp_camera_deinit();
    }
    delete this->pino_reset;
    delete this->pino_pwdn;
}

// O Método de Inicialização
esp_err_t Camera_OV5640::inicializar(framesize_t resolucao, pixformat_t formato) {
    if (this->inicializada) {
        // ESP_LOGW(TAG, "A camera ja foi inicializada.");
        return ESP_OK;
    }

    // ESP_LOGI(TAG, "Iniciando sequencia de boot da OV5640 com Nova HAL I2C...");

    // Sequência de Hardware Reset
    this->pino_pwdn->desligar(); 
    delay_ms(10);
    this->pino_reset->desligar(); 
    delay_ms(20);
    this->pino_reset->ligar();    
    delay_ms(20);

    // Acorda o XCLK com o seu PWM a 20MHz
    this->gerador_clock->habilitarClockCamera(20000000);
    delay_ms(10); 

    // Configuração da estrutura nativa do driver
    camera_config_t config = {};
    config.ledc_channel = LEDC_CHANNEL_0; 
    config.ledc_timer   = LEDC_TIMER_0;   
    
    config.pin_d0    = this->mapa_pinos.d0;
    config.pin_d1    = this->mapa_pinos.d1;
    config.pin_d2    = this->mapa_pinos.d2;
    config.pin_d3    = this->mapa_pinos.d3;
    config.pin_d4    = this->mapa_pinos.d4;
    config.pin_d5    = this->mapa_pinos.d5;
    config.pin_d6    = this->mapa_pinos.d6;
    config.pin_d7    = this->mapa_pinos.d7;
    
    // CORREÇÃO ERRO 1: Acessando o pino pelo getter público da sua classe base GPIO
    config.pin_xclk  = this->gerador_clock->getPino(); 
    config.pin_pclk  = this->mapa_pinos.pclk;
    config.pin_vsync = this->mapa_pinos.vsync;
    config.pin_href  = this->mapa_pinos.href;
    
    config.pin_sccb_sda = GPIO_NUM_4;  
    config.pin_sccb_scl = GPIO_NUM_5;  
    
    config.sccb_i2c_port = I2C_NUM_0;
    
    config.pin_pwdn  = this->mapa_pinos.pwdn;
    config.pin_reset = this->mapa_pinos.reset;
    
    config.xclk_freq_hz = 20000000;
    config.frame_size   = resolucao;
    config.pixel_format = formato;
    config.jpeg_quality = 12; 
    config.fb_count     = 1;  
    
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode   = CAMERA_GRAB_WHEN_EMPTY;

    // Inicia o motor físico de captura (DMA/LCD_CAM)
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        // ESP_LOGE(TAG, "Falha ao iniciar o motor nativo esp_camera: 0x%x", err);
        return err;
    }

    // // CORREÇÃO ERRO 2: Forçando explicitamente os tipos e resolvendo a ambiguidade
    // uint8_t endereco_ov5640 = 0x3C;
    // uint16_t registrador_id = 0x300A; // Variável estrita de 16-bits para casar com sua sobrecarga
    // uint8_t chip_id_high = 0;

    // // Agora o C++ sabe exatamente que deve chamar o método: ler(uint8_t, uint16_t, uint8_t*, size_t)
    // err = this->barramento_i2c->ler(endereco_ov5640, registrador_id, &chip_id_high, 1);
    
    // if (err != ESP_OK || chip_id_high != 0x56) {
    //     ESP_LOGE(TAG, "Erro de comunicacao SCCB Nova HAL. Resposta obtida: 0x%X", chip_id_high);
    //     esp_camera_deinit();
    //     return ESP_FAIL;
    // }

    this->inicializada = true;
    // ESP_LOGI(TAG, "Camera OV5640 sincronizada com a Nova HAL I2C com sucesso!");
    return ESP_OK;
}

camera_fb_t* Camera_OV5640::capturarFoto() {
    if (!this->inicializada) return nullptr;
    liberarMemoria();
    this->buffer_foto = esp_camera_fb_get();
    if (!this->buffer_foto) return nullptr;
    
    // ESP_LOGI(TAG, "Foto tirada! Tamanho: %d bytes.", this->buffer_foto->len);
    return this->buffer_foto;
}

void Camera_OV5640::liberarMemoria() {
    if (this->buffer_foto != nullptr) {
        esp_camera_fb_return(this->buffer_foto);
        this->buffer_foto = nullptr;
    }
}

void Camera_OV5640::dormir() {
    if (this->inicializada) this->pino_pwdn->ligar();
}

void Camera_OV5640::acordar() {
    if (this->inicializada) {
        this->pino_pwdn->desligar();
        delay_ms(10);
    }
}