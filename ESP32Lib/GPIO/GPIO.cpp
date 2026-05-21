#include "GPIO.hpp"

GPIO::GPIO(gpio_num_t numPino, gpio_mode_t modoPino, gpio_pull_mode_t modoPull) {
    pino = numPino;
    modo = modoPino;
    estado_atual = 0; 

    gpio_reset_pin(pino);
    gpio_set_direction(pino, modo);
    gpio_set_pull_mode(pino, modoPull);
}

void GPIO::ligar() {
    if (modo == GPIO_MODE_OUTPUT || modo == GPIO_MODE_INPUT_OUTPUT) {
        estado_atual = 1;
        gpio_set_level(pino, estado_atual);
    }
}

void GPIO::desligar() {
    if (modo == GPIO_MODE_OUTPUT || modo == GPIO_MODE_INPUT_OUTPUT) {
        estado_atual = 0;
        gpio_set_level(pino, estado_atual);
    }
}

void GPIO::inverter() {
    if (modo == GPIO_MODE_OUTPUT || modo == GPIO_MODE_INPUT_OUTPUT) {
        estado_atual = !estado_atual; 
        gpio_set_level(pino, estado_atual);
    }
}

int GPIO::ler() {
    return gpio_get_level(pino);
}

void GPIO::configPull(gpio_pull_mode_t modoPull) {
    if (modo == GPIO_MODE_INPUT || modo == GPIO_MODE_INPUT_OUTPUT) {
        gpio_set_pull_mode(pino, modoPull);
    }
}