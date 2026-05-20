/**
 * Captura imagem JPEG e envia via Serial em Base64.
 *
 * Por que Base64?
 *   O fwrite() binário pela console do IDF pode ter bytes corrompidos
 *   quando caracteres especiais (0x0A, 0x0D, 0x00) são interpretados
 *   pelo driver de UART/console. Base64 usa apenas caracteres ASCII
 *   imprimíveis, eliminando esse problema completamente.
 *
 * Protocolo:
 *   IMG_START\n
 *   SIZE:<n_bytes_originais>\n
 *   <dados em Base64, uma linha contínua>\n
 *   IMG_END\n
 */

#include "sdkconfig.h"
#include <esp_log.h>
#include <esp_system.h>
#include <string.h>
#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef portTICK_RATE_MS
#define portTICK_RATE_MS portTICK_PERIOD_MS
#endif

#include "esp_camera.h"

#if defined(CONFIG_CAMERA_AF_SUPPORT) && CONFIG_CAMERA_AF_SUPPORT
#include "esp_camera_af.h"
#endif

// ─── Pinout câmera ────────────────────────────────────────────────────────────
static camera_config_t camera_config = {
    .pin_pwdn       = -1,
    .pin_reset      = -1,
    .pin_xclk       = 15,
    .pin_sccb_sda   = 4,
    .pin_sccb_scl   = 5,
    .pin_d7 = 16, .pin_d6 = 17, .pin_d5 = 18, .pin_d4 = 12,
    .pin_d3 = 10, .pin_d2 = 8,  .pin_d1 = 9,  .pin_d0 = 11,
    .pin_vsync = 6, .pin_href = 7, .pin_pclk = 13,

    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,
    .frame_size   = FRAMESIZE_SVGA,
    .jpeg_quality = 10,
    .fb_count     = 2,
    .fb_location  = CAMERA_FB_IN_PSRAM,
    .grab_mode    = CAMERA_GRAB_LATEST,
};

static const char *TAG = "cam_serial";

// ─── Tabela Base64 ────────────────────────────────────────────────────────────
static const char B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Codifica `in_len` bytes de `src` em Base64 e imprime direto no stdout.
// Processa em blocos de 3 bytes → 4 chars ASCII.
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

// ─── Inicialização da câmera ──────────────────────────────────────────────────
static esp_err_t init_camera(void)
{
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
        ESP_LOGE(TAG, "Camera Init Failed: %s", esp_err_to_name(err));
    return err;
}

// ─── Envio da imagem ──────────────────────────────────────────────────────────
static void send_image(const uint8_t *data, size_t len)
{
    // Silencia logs ANTES de qualquer byte do protocolo
    esp_log_level_set("*", ESP_LOG_NONE);
    vTaskDelay(10 / portTICK_RATE_MS);  // drena resíduos da UART

    printf("IMG_START\n");
    printf("SIZE:%zu\n", len);
    fflush(stdout);

    print_base64(data, len);
    printf("\n");
    fflush(stdout);

    printf("IMG_END\n");
    fflush(stdout);

    // Reativa logs
    esp_log_level_set("*", ESP_LOG_INFO);
}

// ─── app_main ─────────────────────────────────────────────────────────────────
void app_main(void)
{
#if ESP_CAMERA_SUPPORTED
    ESP_LOGI(TAG, "Inicializando camera...");
    if (init_camera() != ESP_OK) return;

    // Descarta primeiros frames para estabilizar exposição
    ESP_LOGI(TAG, "Estabilizando...");
    for (int i = 0; i < 5; i++) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (fb) esp_camera_fb_return(fb);
        vTaskDelay(200 / portTICK_RATE_MS);
    }

    ESP_LOGI(TAG, "Pronto! Enviando imagens a cada 5s...");

    while (1) {
        camera_fb_t *pic = esp_camera_fb_get();
        if (!pic) {
            ESP_LOGE(TAG, "Falha ao capturar frame");
            vTaskDelay(1000 / portTICK_RATE_MS);
            continue;
        }

        size_t frame_len = pic->len;
        send_image(pic->buf, frame_len);
        esp_camera_fb_return(pic);

        ESP_LOGI(TAG, "Enviado: %zu bytes JPEG (~%zu bytes Base64)",
                 frame_len, (frame_len * 4 / 3) + 4);

        vTaskDelay(1000 / portTICK_RATE_MS);
    }
#else
    ESP_LOGE(TAG, "Camera nao suportada neste chip");
#endif
}