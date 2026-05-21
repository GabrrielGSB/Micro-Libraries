# Módulo `Tempo`

Conjunto de funções utilitárias para controle de tempo no ESP32 com ESP-IDF. Fornece delays em múltiplas unidades e uma função `millis()` para medir intervalos — inspirado na API do Arduino, mas construído sobre as primitivas nativas do FreeRTOS e do ESP-IDF.

---

## Dependências

| Biblioteca          | Uso                                              |
|---------------------|--------------------------------------------------|
| `freertos/task.h`   | `vTaskDelay` — delay cooperativo com o RTOS      |
| `esp_rom_sys.h`     | `esp_rom_delay_us` — delay de precisão em µs     |
| `esp_timer.h`       | `esp_timer_get_time` — tempo desde o boot em µs  |

> Todas as funções são `static inline` definidas diretamente no header — não há arquivo `.cpp` com implementação separada.

---

## Funções disponíveis

### `delay_ms(ms)` — Aguarda em milissegundos

```cpp
void delay_ms(uint32_t ms);
```

Pausa a task atual pelo tempo especificado, cedendo o processador ao FreeRTOS. Ideal para a maioria dos casos.

```cpp
delay_ms(500);  // aguarda 500 ms
delay_ms(1000); // aguarda 1 segundo
```

---

### `delay_s(s)` — Aguarda em segundos

```cpp
void delay_s(uint32_t s);
```

Converte segundos para milissegundos e chama `vTaskDelay`. Conveniência para esperas longas.

```cpp
delay_s(5);  // aguarda 5 segundos
delay_s(60); // aguarda 1 minuto
```

---

### `delay_h(h)` — Aguarda em horas

```cpp
void delay_h(uint32_t h);
```

Para esperas muito longas, como ciclos de monitoramento periódico.

```cpp
delay_h(1); // aguarda 1 hora
delay_h(8); // aguarda 8 horas
```

> **Atenção:** `vTaskDelay` recebe um valor do tipo `TickType_t` (geralmente `uint32_t`). Para horas muito grandes, o valor calculado pode ultrapassar o limite de ticks. Prefira múltiplas chamadas a `delay_ms` em loops longos se precisar de maior controle.

---

### `delay_us(us)` — Aguarda em microssegundos

```cpp
void delay_us(uint32_t us);
```

Implementado com `esp_rom_delay_us` — realiza **busy-wait** (loop de espera ativa), sem ceder o processador. Use apenas para delays muito curtos onde precisão é essencial.

```cpp
delay_us(10);  // aguarda 10 µs
delay_us(500); // aguarda 500 µs
```

> **Atenção:** por ser busy-wait, essa função **bloqueia o processador** durante toda a espera e não coopera com o FreeRTOS. Evite valores altos — para esperas maiores que ~1 ms, use `delay_ms`.

---

### `millis()` — Tempo decorrido desde o boot

```cpp
uint64_t millis();
```

Retorna o número de milissegundos desde que o ESP32 foi iniciado, baseado no timer de alta resolução do ESP-IDF (`esp_timer_get_time`). Útil para medir intervalos sem bloquear o programa.

```cpp
uint64_t agora = millis(); // ex: 3520 ms desde o boot
```

---

## Diferença entre os delays

| Função       | Unidade  | Mecanismo        | Cede CPU? | Uso recomendado                    |
|--------------|----------|------------------|-----------|------------------------------------|
| `delay_ms`   | ms       | `vTaskDelay`     | ✅ Sim    | Esperas gerais no loop             |
| `delay_s`    | s        | `vTaskDelay`     | ✅ Sim    | Esperas longas legíveis            |
| `delay_h`    | h        | `vTaskDelay`     | ✅ Sim    | Ciclos de monitoramento            |
| `delay_us`   | µs       | busy-wait        | ❌ Não    | Temporização de precisão curta     |

---

## Medição de intervalos com `millis()`

O padrão mais comum é registrar o instante inicial e comparar com o tempo atual — sem bloquear o loop.

```cpp
#include "Tempo.hpp"

uint64_t ultimo = millis();

while (true) {
    if ((millis() - ultimo) >= 1000) { // 1 segundo passou
        ultimo = millis();
        // executa ação periódica
    }
    // restante do loop roda livremente
}
```

Esse padrão substitui `delay_ms` nos casos em que o programa precisa fazer outras coisas enquanto aguarda.

---

## Exemplo completo — LED piscando sem delay bloqueante

```cpp
#include "Tempo.hpp"
#include "GPIO.hpp"

extern "C" void app_main() {
    GPIO led(GPIO2, OUTPUT);
    uint64_t ultimo_pisca = millis();

    while (true) {
        if ((millis() - ultimo_pisca) >= 500) {
            ultimo_pisca = millis();
            led.inverter();
        }

        // aqui podem entrar outras tarefas sem atraso
    }
}
```

## Exemplo completo — uso misto de unidades

```cpp
#include "Tempo.hpp"

extern "C" void app_main() {
    delay_us(50);    // pulso curto preciso
    delay_ms(200);   // aguarda estabilizar
    delay_s(2);      // espera de inicialização legível

    // loop periódico a cada 10 s
    while (true) {
        // executa tarefa
        delay_s(10);
    }
}
```