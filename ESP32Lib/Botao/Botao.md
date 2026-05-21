# Classe `Botao`

Classe derivada de `GPIO` que encapsula a lógica de leitura de botões físicos no ESP32. Oferece debounce automático, leitura de estado filtrada e suporte a interrupções com callback.

---

## Dependências

| Arquivo      | Papel                                             |
|--------------|---------------------------------------------------|
| `GPIO.hpp`   | Classe base, gerencia o pino como entrada        |
| `Tempo.hpp`  | Fornece a função `millis()` usada no debounce     |

---

## Macros de modo de interrupção

| Macro     | Dispara quando...                        | 
|-----------|------------------------------------------|
| `RISING`  | Sinal vai de baixo → alto (borda de subida)  
| `FALLING` | Sinal vai de alto → baixo (borda de descida) 
| `CHANGE`  | Qualquer mudança de nível                

> Como o construtor já aplica pull-up interno, um botão conectado ao GND dispara em `FALLING` quando pressionado — que é o padrão adotado pela classe.

---

## Interface da classe

```cpp
class Botao : public GPIO {
public:
    Botao(gpio_num_t numPino, uint32_t debounce_ms = 200);

    int  estado();       // Leitura com debounce por software (polling)
    int  estadoBruto();  // Leitura direta do pino, sem filtro

    void definirInterrupcao(callback_t callback,
                            gpio_int_type_t modo = FALLING);
};
```

O tipo do callback é:

```cpp
typedef void (*callback_t)();
```

> Significa que em algum main é preciso definir o callback apenas como `void`

---

## Como usar

#### 1. Incluir o header

```cpp
#include "Botao.hpp"
```

#### 2. Criar uma instância

O construtor configura o pino automaticamente como entrada com pull-up. O segundo parâmetro define o tempo de debounce em milissegundos (padrão: 200 ms).

```cpp
Botao botao(GPIO4);           // debounce de 200 ms (padrão)
Botao botao(GPIO4, 50);       // debounce de 50 ms
```

---

## Modo 1 — Polling com debounce (`estado()`)

Use quando o botão é verificado periodicamente dentro de um loop. O método retorna o estado filtrado: só atualiza o valor se o sinal se mantiver estável por pelo menos 200 ms.

```cpp
// Retorna 0 quando pressionado (pull-up + botão ao GND)
// Retorna 1 quando solto
int nivel = botao.estado();
```

### Exemplos 
#### polling em loop

```cpp
#include "Botao.hpp"
#include "GPIO.hpp"
#include "Tempo.hpp"

extern "C" void app_main() {
    Botao botao(GPIO4);
    GPIO  led(GPIO2, OUTPUT);

    while (true) {
        if (botao.estado() == 0) { // pressionado
            led.ligar();
        } else {
            led.desligar();
        }
        delay_ms(10);
    }
}
```

---

## Modo 2 — Interrupção com callback (`definirInterrupcao()`)

Use quando a resposta precisa ser imediata ou quando o loop principal está ocupado com outras tarefas. O debounce é aplicado dentro da própria ISR.

```cpp
void definirInterrupcao(callback_t callback, gpio_int_type_t modo = FALLING);
```

| Parâmetro  | Tipo                | Padrão    | Descrição                              |
|------------|---------------------|-----------|----------------------------------------|
| `callback` | `void (*)()`        | —         | Função chamada ao detectar a borda     |
| `modo`     | `gpio_int_type_t`   | `FALLING` | Tipo de borda que dispara a interrupção|

> **Atenção:** a função de callback é executada dentro de uma ISR. Ela deve ser curta e não pode chamar funções bloqueantes (`vTaskDelay`, `printf`, alocação de memória, etc.). Use flags ou filas do FreeRTOS para comunicação com o loop principal.

### Exemplos
#### Interrupção simples

```cpp
#include "Botao.hpp"
#include "GPIO.hpp"
#include "Tempo.hpp"

GPIO led(GPIO2, OUTPUT);
Botao botao(GPIO4);

void ao_pressionar() {
    led.inverter(); // curto e seguro dentro de ISR
}

extern "C" void app_main() {
    botao.definirInterrupcao(ao_pressionar); // borda de descida (padrão)

    while (true) {
        delay_ms(1000);
    }
}
```

#### Interrupção com flag para o loop principal

```cpp
#include "Botao.hpp"
#include "GPIO.hpp"
#include "Tempo.hpp"

volatile bool foi_pressionado = false;

GPIO led(GPIO2, OUTPUT);
Botao botao(GPIO4);

void ao_pressionar() {
    foi_pressionado = true; // apenas sinaliza, sem bloqueio
}

extern "C" void app_main() {
    botao.definirInterrupcao(ao_pressionar, FALLING);

    while (true) {
        if (foi_pressionado) {
            foi_pressionado = false;
            led.inverter();
        }
        delay_ms(10);
    }
}
```

---

## Leitura bruta (`estadoBruto()`)

Retorna o nível lógico atual do pino diretamente, sem nenhum filtro de debounce. Útil para diagnóstico ou quando o debounce é tratado externamente.

```cpp
int nivel = botao.estadoBruto(); // 0 ou 1
```

---

## Resumo dos métodos

| Método                  | Debounce | Bloqueante | Uso recomendado                         |
|-------------------------|----------|------------|-----------------------------------------|
| `estado()`              | ✅ Sim   | Não        | Polling periódico no loop               |
| `estadoBruto()`         | ❌ Não   | Não        | Diagnóstico / debounce externo          |
| `definirInterrupcao()`  | ✅ Sim   | Não        | Resposta imediata a eventos de hardware |

---

### Observações sobre o serviço ISR

> O serviço de interrupções do ESP-IDF (`gpio_install_isr_service`) é instalado automaticamente na primeira vez que `definirInterrupcao()` é chamado. Chamadas subsequentes em outros botões reutilizam o mesmo serviço — não há conflito nem reinstalação.