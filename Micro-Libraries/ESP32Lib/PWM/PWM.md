# Classe `PWM` — Documentação de Uso

Classe derivada de `GPIO` que abstrai o periférico **LEDC** do ESP-IDF para geração de sinais PWM. Oferece alocação automática de canais, controle de duty cycle, fade suave por hardware e suporte a clock de alta frequência para câmeras e outros periféricos.

---

## Dependências

| Arquivo / Biblioteca  | Papel                                              |
|-----------------------|----------------------------------------------------|
| `GPIO.hpp`            | Classe base — configura o pino como saída          |
| `driver/ledc.h`       | Driver LEDC do ESP-IDF — periférico PWM do chip    |
| `esp_log.h`           | Log de erros e diagnóstico via serial              |

---

## Conceitos do LEDC

O ESP32 possui um periférico chamado **LEDC** (LED Control) que gera sinais PWM por hardware. Ele é organizado em:

- **Timers** — definem a frequência e a resolução do sinal
- **Canais** — vinculam um timer a um pino físico e controlam o duty cycle

A classe `PWM` gerencia tudo isso automaticamente. Ao criar um objeto, um canal livre é reservado; ao destruí-lo, o canal é liberado para reuso.

---

## Resolução e duty cycle

A resolução define quantos níveis de duty cycle existem. Com `N` bits, o duty vai de `0` a `2^N - 1`:

| Resolução (`ledc_timer_bit_t`) | Valores de duty    | Uso típico              |
|--------------------------------|--------------------|-------------------------|
| `LEDC_TIMER_8_BIT`             | 0 – 255            | LEDs simples            |
| `LEDC_TIMER_10_BIT` (padrão)   | 0 – 1023           | LEDs, motores           |
| `LEDC_TIMER_12_BIT`            | 0 – 4095           | Controle de precisão    |
| `LEDC_TIMER_2_BIT`             | 0 – 3              | Clock de câmera (XCLK)  |

> **Regra de hardware:** `frequência × 2^resolução ≤ clock_fonte` (geralmente 80 MHz). Frequências altas exigem resoluções menores.

---

## Interface da classe

```cpp
class PWM : public GPIO {
public:
    PWM(gpio_num_t numPino,
        uint32_t freq_hz          = 5000,
        ledc_timer_bit_t res_bits = LEDC_TIMER_10_BIT);

    ~PWM(); // libera o canal automaticamente

    void definirDuty(uint32_t duty);
    void fade(uint32_t duty_alvo, uint32_t tempo_ms);
    void pausar();
    void retomar();
    void habilitarClockCamera(uint32_t frequencia_hz = 20000000);
};
```

---

## Como usar

### 1. Incluir o header

```cpp
#include "PWM.hpp"
```

### 2. Criar uma instância

```cpp
// Padrão: 5 kHz, resolução de 10 bits (duty 0–1023)
PWM led(GPIO2);

// Personalizado: 1 kHz, resolução de 8 bits (duty 0–255)
PWM motor(GPIO18, 1000, LEDC_TIMER_8_BIT);
```

O construtor configura o pino, reserva um canal LEDC livre e instala o serviço de fade (apenas uma vez, automaticamente).

---

## Métodos

### `definirDuty(duty)` — Ajusta o brilho / velocidade instantaneamente

```cpp
void definirDuty(uint32_t duty);
```

Muda o duty cycle imediatamente. O valor máximo depende da resolução configurada.

```cpp
// Com resolução de 10 bits (0–1023)
led.definirDuty(0);    // desligado
led.definirDuty(512);  // 50%
led.definirDuty(1023); // 100% (brilho máximo)
```

---

### `fade(duty_alvo, tempo_ms)` — Transição suave por hardware

```cpp
void fade(uint32_t duty_alvo, uint32_t tempo_ms);
```

Transita o duty cycle suavemente até `duty_alvo` ao longo de `tempo_ms` milissegundos, usando o circuito de fade do próprio chip — sem ocupar a CPU.

```cpp
led.fade(1023, 1000); // aumenta ao máximo em 1 segundo
led.fade(0, 500);     // apaga suavemente em 500 ms
```

> O fade roda de forma não bloqueante (`LEDC_FADE_NO_WAIT`). O loop principal continua executando durante a transição.

---

### `pausar()` e `retomar()` — Controle do timer

```cpp
void pausar();
void retomar();
```

Pausa e retoma o timer associado ao canal. O sinal PWM para completamente enquanto pausado.

```cpp
pwm.pausar();
delay_ms(200);
pwm.retomar();
```

---

### `habilitarClockCamera(frequencia_hz)` — Clock de alta frequência para câmera

```cpp
void habilitarClockCamera(uint32_t frequencia_hz = 20000000);
```

Reconfigura o timer para gerar uma onda quadrada de alta frequência (padrão: 20 MHz), adequada para alimentar o pino **XCLK** de módulos de câmera como o OV2640. A resolução é automaticamente reduzida para 2 bits para que o hardware suporte a frequência solicitada.

```cpp
PWM xclk(GPIO0);
xclk.habilitarClockCamera();          // 20 MHz (padrão)
xclk.habilitarClockCamera(24000000);  // 24 MHz
```

> Após chamar este método, `definirDuty` e `fade` não devem ser usados no mesmo objeto — o timer foi reconfigurado para uso exclusivo como clock.

---

## Alocação automática de canais

O ESP32 possui um número limitado de canais LEDC (`LEDC_CHANNEL_MAX`, geralmente 8). A classe mantém uma tabela estática interna e reserva o primeiro canal disponível a cada novo objeto criado. Quando o objeto é destruído (sai de escopo ou é deletado), o canal é liberado automaticamente.

```cpp
{
    PWM led1(GPIO2);  // ocupa canal 0
    PWM led2(GPIO4);  // ocupa canal 1
    PWM led3(GPIO5);  // ocupa canal 2
}
// ao sair do escopo, os 3 canais são liberados
```

Se todos os canais estiverem ocupados, um erro é logado via `ESP_LOGE` e o canal 0 é sobrescrito.

---

## Exemplos

### Dimmer de LED com fade

```cpp
#include "PWM.hpp"
#include "Tempo.hpp"

extern "C" void app_main() {
    PWM led(GPIO2); // 5 kHz, 10 bits

    while (true) {
        led.fade(1023, 1000); // acende em 1 s
        delay_ms(1200);
        led.fade(0, 1000);    // apaga em 1 s
        delay_ms(1200);
    }
}
```

### Controle de velocidade de motor

```cpp
#include "PWM.hpp"

extern "C" void app_main() {
    PWM motor(GPIO18, 1000, LEDC_TIMER_8_BIT); // 1 kHz, 8 bits (0–255)

    motor.definirDuty(0);    // parado
    delay_ms(1000);
    motor.definirDuty(128);  // 50% de velocidade
    delay_ms(2000);
    motor.definirDuty(255);  // velocidade máxima
    delay_ms(2000);
    motor.definirDuty(0);    // para
}
```

### Clock para módulo de câmera OV2640

```cpp
#include "PWM.hpp"

extern "C" void app_main() {
    PWM xclk(GPIO0);
    xclk.habilitarClockCamera(); // gera 20 MHz no pino GPIO0

    // inicializa o driver da câmera após o clock estar estável
    // camera_config_t config = { ... };
    // esp_camera_init(&config);
}
```

---

## Resumo dos métodos

| Método                   | Bloqueante | Descrição                                     |
|--------------------------|------------|-----------------------------------------------|
| `definirDuty(duty)`      | Não        | Ajuste instantâneo do duty cycle              |
| `fade(alvo, ms)`         | Não        | Transição suave por hardware                  |
| `pausar()`               | Não        | Para o sinal PWM                              |
| `retomar()`              | Não        | Reinicia o sinal PWM                          |
| `habilitarClockCamera()` | Não        | Gera clock de alta frequência (ex: XCLK)      |