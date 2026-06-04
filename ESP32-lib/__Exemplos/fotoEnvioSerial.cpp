// main.cpp
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "I2C.hpp"
#include "PWM.hpp"
#include "Camera_OV5640.hpp"
#include "Tempo.hpp" 
#include "driver/uart_vfs.h" 
#include "driver/uart.h"      
#include <fcntl.h>

static const char* TAG = "MAIN_TESTE";

// Tabela de caracteres padrão do Base64
static const char B64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

#define pinSDA   GPIO4
#define pinSCL   GPIO5
#define pinXCLK  GPIO15  

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

static void print_base64(const uint8_t *src, size_t in_len)
{
    // Buffer de saída: 60 bytes originais → 80 chars Base64 por vez
    char buf[81];
    size_t i = 0;

    while (i < in_len) {
        size_t chunk = (in_len - i) >= 60 ? 60 : (in_len - i);
        size_t out = 0;
        size_t j = 0;

        while (j < chunk) {
            uint32_t b0 = src[i + j];
            uint32_t b1 = (j + 1 < chunk) ? src[i + j + 1] : 0;
            uint32_t b2 = (j + 2 < chunk) ? src[i + j + 2] : 0;

            buf[out++] = B64[(b0 >> 2) & 0x3F];
            buf[out++] = B64[((b0 << 4) | (b1 >> 4)) & 0x3F];
            buf[out++] = (j + 1 < chunk) ? B64[((b1 << 2) | (b2 >> 6)) & 0x3F] : '=';
            buf[out++] = (j + 2 < chunk) ? B64[b2 & 0x3F] : '=';
            j += 3;
        }

        buf[out] = '\0';
        printf("%s", buf);
        i += chunk;
    }
}

extern "C" void app_main(void) {
    // 1. Instala o driver com buffers generosos
    uart_driver_install(UART_NUM_0, 4096, 16384, 0, NULL, 0);

    // 2. Reconfigura para 921600 baud usando inicialização limpa {}
    uart_config_t uart_config = {};
    uart_config.baud_rate           = 921600;
    uart_config.data_bits           = UART_DATA_8_BITS;
    uart_config.parity              = UART_PARITY_DISABLE;
    uart_config.stop_bits           = UART_STOP_BITS_1;
    uart_config.flow_ctrl           = UART_HW_FLOWCTRL_DISABLE;
    uart_config.rx_flow_ctrl_thresh = 0;
    uart_config.source_clk          = UART_SCLK_DEFAULT;
    
    uart_param_config(UART_NUM_0, &uart_config);

    // 3. Configura VFS para manter o comportamento padrão de texto da UART0
    uart_vfs_dev_port_set_tx_line_endings(UART_NUM_0, ESP_LINE_ENDINGS_LF);
    uart_vfs_dev_port_set_rx_line_endings(UART_NUM_0, ESP_LINE_ENDINGS_LF);
    uart_vfs_dev_use_driver(UART_NUM_0);

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
        .reset = pinRESET, 
        .pwdn  = pinPWDN   
    };

    Camera_OV5640 camera(pinos_config, &barramento, &clock_externo);

    if (camera.inicializar(FRAMESIZE_VGA, PIXFORMAT_JPEG) != ESP_OK) {
        ESP_LOGE(TAG, "Falha na inicializacao. Sistema parado.");
        while (true) { delay_s(1); }
    }

    ESP_LOGI(TAG, "Camera pronta. Iniciando transmissao Base64 Customizada...");
    delay_s(2);

    while (true) {
        camera_fb_t* foto = camera.capturarFoto();

        if (foto != nullptr) {
            // --- INÍCIO DO PROTOCOLO BASE64 ---
            printf("START_IMAGE_B64\n");
            fflush(stdout);

            // Chama sua função para transmitir o JPEG em pedacinhos de texto
            print_base64(foto->buf, foto->len);
            
            // Adiciona a quebra de linha obrigatória ao terminar o dump da imagem
            printf("\n"); 

            printf("END_IMAGE_B64\n");
            fflush(stdout);
            // --- FIM DO PROTOCOLO ---
        }

        camera.liberarMemoria();
        delay_s(5); // Aguarda 5 segundos para a próxima captura
    }
}