// main.cpp
#include <stdio.h>
#include "esp_log.h"
#include "I2C.hpp"
#include "PWM.hpp"
#include "Camera_OV5640.hpp"
#include "Tempo.hpp" 

static const char* TAG = "MAIN_TESTE";

// IMPORTANTE: Altere os números dos pinos abaixo de acordo com a pinagem 
// física da sua placa de desenvolvimento ESP32-S3 (ex: Freenove, ESP32-S3-CAM, etc.)
#define pinSDA   GPIO4
#define pinSCL   GPIO5
#define pinXCLK  GPIO15  // Pino que será controlado pelo seu PWM

#define pinD0    GPIO11
#define pinD1    GPIO9
#define pinD2    GPIO8
#define pinD3    GPIO10
#define pinD4    GPIO12
#define pinD5    GPIO18
#define pinD6    GPIO17
#define pinD7    GPIO16

#define pinVSYNC GPIO6
#define pinHREF  GPIO7
#define pinPCLK  GPIO13
#define pinRESET GPIO41
#define pinPWDN  GPIO42

extern "C" void app_main(void) {
    I2C barramento(I2C_NUM_0, pinSDA, pinSCL, FAST_MODE);
    PWM clock_externo(pinXCLK, 5000); 

    PinosCamera pinos_config = {
        .d0    = pinD0,
        .d1    = pinD1,
        .d2    = pinD2,
        .d3    = pinD3,
        .d4    = pinD4,
        .d5    = pinD5,
        .d6    = pinD6,
        .d7    = pinD7,
        .vsync = pinVSYNC,
        .href  = pinHREF,
        .pclk  = pinPCLK,
        .reset = pinRESET, // Pino tratado por sua HAL GPIO interna
        .pwdn  = pinPWDN   // Pino tratado por sua HAL GPIO interna
    };

    Camera_OV5640 camera(pinos_config, &barramento, &clock_externo);

    esp_err_t resultado = camera.inicializar(FRAMESIZE_QVGA, PIXFORMAT_JPEG);
    
    if (resultado != ESP_OK) {
        ESP_LOGE(TAG, "Abortando execucao. Verifique as conexoes e os pinos fornecidos.");
        while (true) {
            delay_s(1); // Cede a CPU para evitar travar o Watchdog
        }
    }

    ESP_LOGI(TAG, "Aguardando 2 segundos para o sensor estabilizar a exposicao automatica...");
    delay_s(2);

    uint32_t contador_fotos = 1;
    
    while (true) {
        ESP_LOGI(TAG, "-------------------------------------------------");
        ESP_LOGI(TAG, "Disparando captura da foto #%lu...", contador_fotos);

        // Captura o quadro. O hardwareLCD_CAM joga os bytes na memória via DMA.
        camera_fb_t* foto = camera.capturarFoto();

        if (foto != nullptr) {
            ESP_LOGI(TAG, "[SUCESSO] Foto gravada na SRAM com sucesso!");
            ESP_LOGI(TAG, "-> Endereco do Buffer: %p", foto->buf);
            ESP_LOGI(TAG, "-> Tamanho do arquivo JPEG: %d bytes", foto->len);
            ESP_LOGI(TAG, "-> Dimensoes Reais: %dx%d", foto->width, foto->height);
            
            contador_fotos++;
        } else {
            ESP_LOGE(TAG, "[FALHA] Nao foi possivel obter o frame da camera.");
        }

        camera.liberarMemoria();

        delay_s(5); 
    }
}