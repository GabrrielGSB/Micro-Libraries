#include "Botao.hpp"

static bool servico_isr_instalado = false;

Botao::Botao(gpio_num_t numPino, uint32_t debounce_ms) : GPIO(numPino, INPUT, PULLUP) {
    tempo_debounce = debounce_ms;
    timer = 0;

    callback_usuario = nullptr;
    xTaskHandle      = nullptr;

    xSemaforoDebounce = xSemaphoreCreateBinary();

    if (xSemaforoDebounce != nullptr) {
        
        // Criamos com prioridade alta para responder rápido após o debounce
        xTaskCreate(
            Botao::pressionado,       // Função da Task
            "TaskDebounce",           // Nome para debug
            2048,                     // Tamanho da pilha (Stack)
            (void*)this,              // Passa o ponteiro do próprio objeto como argumento
            configMAX_PRIORITIES - 1, // Prioridade alta
            &xTaskHandle              // Handler da Task
        );
    }
}

Botao::~Botao() {
    if (xTaskHandle       != nullptr) vTaskDelete(xTaskHandle);
    if (xSemaforoDebounce != nullptr) vSemaphoreDelete(xSemaforoDebounce);
}

int Botao::estado() {
    bool leitura_atual = ler();
    
    if (leitura_atual != ultimo_estado) {
        if ((millis() - timer) > 200) {
            timer = millis();
            ultimo_estado = leitura_atual;
        }
    }
    
    return ultimo_estado;
}

void IRAM_ATTR Botao::isr_interna(void* arg) {
    Botao* botao = static_cast<Botao*>(arg);

    BaseType_t xHigherPriorityTaskWoken = pdFALSE; 

    /*
    Checa se a classe foi inicializada corretamente e se o semáforo foi criado,
    a fim de não desabilitar as interrupções sem motivo.
    */
    if (botao->xSemaforoDebounce != nullptr) {
        gpio_intr_disable(botao->pino);

        xSemaphoreGiveFromISR(botao->xSemaforoDebounce, &xHigherPriorityTaskWoken); 
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void Botao::pressionado(void* arg) {
    Botao* botao = static_cast<Botao*>(arg);

    while (true) {
        if (xSemaphoreTake(botao->xSemaforoDebounce, portMAX_DELAY)) {
            delay_ms(botao->tempo_debounce); 

            if (botao->ler() == 0) { 
                if (botao->callback_usuario != nullptr) {
                    botao->callback_usuario(); 
                }
            }
            gpio_intr_enable(botao->pino);
        }
    }
}

void Botao::definirInterrupcao(callback_t callback, gpio_int_type_t modo) {
    callback_usuario  = callback;

    gpio_set_intr_type(pino, modo);

    // Garante que a interrupção seja instalada apenas uma vez para o serviço de ISR
    if (!servico_isr_instalado) {
        gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
        servico_isr_instalado = true;
    }

    gpio_isr_handler_add(pino, isr_interna, (void*) this);
}




