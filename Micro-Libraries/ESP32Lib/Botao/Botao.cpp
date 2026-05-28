#include "Botao.hpp"

static bool servico_isr_instalado = false;

Botao::Botao(gpio_num_t numPino, uint32_t debounce_ms) : GPIO(numPino, INPUT) {
    configPull(PULLUP);
    tempo_ultimo_clique = 0;
    tempo_debounce = debounce_ms;
    callback_usuario = nullptr;
}

int Botao::estado() {
    int leitura_atual = ler();
    
    if (leitura_atual != ultimo_estado) {
        if ((millis() - timer) > 200) {
            timer = millis();
            ultimo_estado = leitura_atual;
        }
    }
    
    return ultimo_estado;
}

int Botao::estadoBruto() {
    return ler();
}

void IRAM_ATTR Botao::isr_interna(void* arg) {
    Botao* botao = static_cast<Botao*>(arg);

    if ((millis() - botao->tempo_ultimo_clique) > botao->tempo_debounce) {
        botao->tempo_ultimo_clique = millis(); 
        
        if (botao->callback_usuario != nullptr) {
            botao->callback_usuario();
        }
    }
}

void Botao::definirInterrupcao(callback_t callback, gpio_int_type_t modo) {
    callback_usuario  = callback;

    gpio_set_intr_type(pino, modo);

    if (!servico_isr_instalado) {
        gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
        servico_isr_instalado = true;
    }

    gpio_isr_handler_add(pino, isr_interna, (void*) this);
}