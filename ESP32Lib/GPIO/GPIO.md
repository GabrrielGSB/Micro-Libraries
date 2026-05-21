# Classe `GPIO` 

Biblioteca wrapper para controle de pinos GPIO no ESP32 usando o **ESP-IDF**. Abstrai as chamadas de baixo nível do driver `driver/gpio.h` em uma interface orientada a objetos simples e legível.

---

## Compatibilidade

A classe suporta automaticamente os seguintes modelos:

| Modelo       | Pinos disponíveis |
|--------------|-------------------|
| ESP32        | GPIO0 – GPIO39    |
| ESP32-S3     | GPIO0 – GPIO48    |
| ESP32-C3     | GPIO0 – GPIO21    |

> Compilar em um modelo não suportado gera um erro em tempo de compilação.

---

## Modos de operação

Os modos são definidos como macros para facilitar a leitura do código:

| Macro          | Significado                        |
|----------------|------------------------------------|
| `OUTPUT`       | Saída digital                      |
| `INPUT`        | Entrada digital                    |
| `INPUT_OUTPUT` | Entrada e saída simultâneas        |
| `OPEN_DRAIN`   | Saída open-drain                   |

### Modos de pull (resistores internos)

| Macro       | Comportamento              |
|-------------|----------------------------|
| `PULLUP`    | Resistor de pull-up interno |
| `PULLDOWN`  | Resistor de pull-down interno |
| `NOPULL`    | Flutuante (sem resistor)    |

---

## Interface da classe

```cpp
class GPIO {
public:
    GPIO(gpio_num_t numPino, gpio_mode_t modoPino);

    void ligar();           // Define nível alto (1)
    void desligar();        // Define nível baixo (0)
    void inverter();        // Alterna o estado atual

    int  ler();             // Lê o nível lógico do pino
    void configPull(gpio_pull_mode_t modoPull); // Configura pull-up/down

    gpio_num_t getPino() const; // Retorna o número do pino
};
```

---

## Como usar

### 1. Incluir o header

```cpp
#include "GPIO.hpp"
```

### 2. Criar uma instância

O construtor já inicializa e configura o pino automaticamente — não é necessário chamar funções de setup separadas.

```cpp
// Pino 2 como saída
GPIO led(GPIO2, OUTPUT);

// Pino 5 como entrada com pull-up interno
GPIO botao(GPIO5, INPUT, PULLUP);
```
> A função `configPull(gpio_pull_mode_t modoPull)` pode ser usada posteriormente no objeto criado para trocar o modo de pull do pino.

### 3. Controlar saídas

```cpp
led.ligar();     // Pino vai para nível alto
led.desligar();  // Pino vai para nível baixo
led.inverter();  // Alterna: se estava em 1, vai para 0, e vice-versa
```

### 4. Ler entradas

```cpp
int nivel = botao.ler(); // Retorna 0 ou 1
if (nivel == 0) {
    // botão pressionado (com pull-up, nível baixo = ativo)
}
```

---

## Exemplos 
> Nos seguintes exemplos será incluida o abstração do Tempo, em `Tempo.hpp` que implementa uma camada de abstração para o uso de delays.
### Piscar LED

```cpp
#include "GPIO.hpp"
#include "Tempo.hpp"

extern "C" void app_main() {
    GPIO led(GPIO2, OUTPUT);

    while (true) {
        led.inverter();
        delay_ms(500); // 500 ms
    }
}
```

### Botão controla LED

```cpp
#include "GPIO.hpp"
#include "Tempo.hpp"

extern "C" void app_main() {
    GPIO led(GPIO2, OUTPUT);
    GPIO botao(GPIO4, INPUT, PULLUP);

    while (true) {
        if (botao.ler() == 0) { // botão pressionado (ativo baixo)
            led.ligar();
        } else {
            led.desligar();
        }
        delay_ms(10); // debounce simples
    }
}
```

---

## Comportamento dos métodos

- `ligar()`, `desligar()` e `inverter()` **só funcionam** se o pino estiver configurado como `OUTPUT` ou `INPUT_OUTPUT`. Em outros modos, as chamadas são ignoradas silenciosamente.
- `configPull()` **só tem efeito** em pinos configurados como `INPUT` ou `INPUT_OUTPUT`.
- `ler()` funciona em qualquer modo e retorna o nível lógico atual lido pelo hardware.

---