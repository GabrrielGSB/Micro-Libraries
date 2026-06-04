#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h" 
#include "esp_timer.h"

static inline void delay_ms(uint32_t ms) {
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

static inline void delay_s(uint32_t s) {
    vTaskDelay((s * 1000) / portTICK_PERIOD_MS);
}

static inline void delay_h(uint32_t h) {
    vTaskDelay((h * 3600000) / portTICK_PERIOD_MS);
}

static inline void delay_us(uint32_t us) {
    esp_rom_delay_us(us);
}

static inline uint64_t millis() {
    return (esp_timer_get_time() / 1000ULL);
}



