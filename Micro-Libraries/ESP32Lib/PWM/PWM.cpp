// PWM.cpp ---------------------------------------------------------
#include "PWM.hpp"

// 1. Inicializando as variáveis estáticas da classe
// Isso cria a nossa "planilha de reservas" de canais totalmente limpa (tudo false)
bool PWM::canais_ocupados[LEDC_CHANNEL_MAX] = {false};

// Variável estática global (escondida no .cpp) para garantir que o serviço 
// de fade seja instalado apenas uma vez no microcontrolador.
static bool fade_instalado = false; 

// --- Métodos Internos ---
ledc_channel_t PWM::alocarCanalLivre() {
    for (int i = 0; i < LEDC_CHANNEL_MAX; i++) {
        if (!canais_ocupados[i]) {
            canais_ocupados[i] = true; // Marca como ocupado
            return (ledc_channel_t)i;  // Retorna o canal encontrado
        }
    }

    ESP_LOGE("PWM", "ERRO: Nenhum canal LEDC livre! Sobrescrevendo canal 0.");
    return LEDC_CHANNEL_0;
}

void PWM::liberarCanal() {
    canais_ocupados[this->canal] = false; // Devolve o canal para o pool
}

// --- Construtor e Destrutor ---
PWM::PWM(gpio_num_t numPino, uint32_t freq_hz, ledc_timer_bit_t res_bits) 
    : GPIO(numPino, OUTPUT), frequencia(freq_hz), resolucao(res_bits) {
    
    this->canal = alocarCanalLivre();
    this->timer = LEDC_TIMER_0; 

    // 1. Configurando o Timer do Hardware
    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode       = LEDC_LOW_SPEED_MODE; // Compatível com ESP32 clássico, S2, S3, C3
    timer_conf.duty_resolution  = this->resolucao;
    timer_conf.timer_num        = this->timer;
    timer_conf.freq_hz          = this->frequencia;
    timer_conf.clk_cfg          = LEDC_AUTO_CLK;       // Escolhe o melhor clock fonte
    
    ledc_timer_config(&timer_conf);

    // 2. Configurando o Canal e Vinculando ao Pino
    ledc_channel_config_t channel_conf = {};
    channel_conf.gpio_num       = numPino; 
    channel_conf.speed_mode     = LEDC_LOW_SPEED_MODE;
    channel_conf.channel        = this->canal;
    channel_conf.intr_type      = LEDC_INTR_DISABLE;
    channel_conf.timer_sel      = this->timer;
    channel_conf.duty           = 0;                  // Inicialmente desligado (duty 0)    
    channel_conf.hpoint         = 0;
    
    ledc_channel_config(&channel_conf);

    // 3. Inicializa o serviço de Hardware Fading apenas no primeiro objeto criado
    if (!fade_instalado) {
        ledc_fade_func_install(0);
        fade_instalado = true;
    }
}

PWM::~PWM() {
    // Para o sinal PWM e zera o duty
    ledc_stop(LEDC_LOW_SPEED_MODE, this->canal, 0);
    // Libera o canal na "planilha"
    liberarCanal();
}

// --- Funcionalidades Públicas ---
void PWM::definirDuty(uint32_t duty) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, this->canal, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, this->canal);
}

void PWM::fade(uint32_t duty_alvo, uint32_t tempo_ms) {
    ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE, this->canal, duty_alvo, tempo_ms);
    ledc_fade_start(LEDC_LOW_SPEED_MODE, this->canal, LEDC_FADE_NO_WAIT);
}

void PWM::pausar() {
    ledc_timer_pause(LEDC_LOW_SPEED_MODE, this->timer);
}

void PWM::retomar() {
    ledc_timer_resume(LEDC_LOW_SPEED_MODE, this->timer);
}

void PWM::habilitarClockCamera(uint32_t frequencia_hz) {
    // 1. Pausar o timer atual (por segurança) antes de alterar a "caixa de velocidades"
    ledc_timer_pause(LEDC_LOW_SPEED_MODE, this->timer);

    // 2. Configurar o Timer para Altíssima Frequência
    // O APB_CLK roda a 80MHz. 80MHz / 20MHz = 4 (que é 2^2). 
    // Logo, a resolução TEM de ser 2 bits para o hardware aguentar.
    ledc_timer_config_t timer_conf = {};
    timer_conf.speed_mode      = LEDC_LOW_SPEED_MODE;
    timer_conf.timer_num       = this->timer;
    timer_conf.duty_resolution = LEDC_TIMER_2_BIT;  // Esmagamos a resolução para 2 bits
    timer_conf.freq_hz         = frequencia_hz;     // Os 20 MHz solicitados
    timer_conf.clk_cfg         = LEDC_AUTO_CLK;     // Força a usar o relógio de alta velocidade da placa
    
    // Aplica a nova configuração do hardware
    ledc_timer_config(&timer_conf);

    // 3. Definir o Duty Cycle para exatos 50% (Onda Quadrada Perfeita)
    // Com 2 bits (4 estados), 50% é o valor 2.
    uint32_t duty_50_percent = 2; 

    // 4. Injetar o novo Duty Cycle no canal e aplicar
    ledc_set_duty(LEDC_LOW_SPEED_MODE, this->canal, duty_50_percent);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, this->canal);
    
    // 5. Retomar o funcionamento do timer
    ledc_timer_resume(LEDC_LOW_SPEED_MODE, this->timer);
    
    ESP_LOGI("PWM", "Clock de Alta Frequencia ativado no pino %d a %lu Hz", this->pino, frequencia_hz);
}