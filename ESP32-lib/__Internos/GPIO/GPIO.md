# Classe `GPIO` 

Biblioteca wrapper para controle de pinos GPIO no ESP32 usando a programação nativa.

---

## Compatibilidade

A classe suporta automaticamente os seguintes modelos:

| Modelo       | Pinos disponíveis |
|--------------|-------------------|
| ESP32        | GPIO0 – GPIO39    |
| ESP32-S3     | GPIO0 – GPIO48    |
| ESP32-C3     | GPIO0 – GPIO21    |

---

## Modos de operação

Os modos que um GPIO pode assumir:

| Macro          | Significado                        |
|----------------|------------------------------------|
| `OUTPUT`       | Saída digital                      |
| `INPUT`        | Entrada digital                    |
| `INPUT_OUTPUT` | Entrada e saída simultâneas        |
| `OPEN_DRAIN`   | Saída open-drain                   |

> **Atenção:** Checar na documentação as possibilidades para cada pino, pois **para alguns pinos pode haver restrições!**

### Modos de pull (resistores internos)
Os modelos de ESP32 tem resistores internos que podem ser conectados a um GPIO, a fim de segurar o nível lógico em alto ou baixo. 

| Macro       | Comportamento              |
|-------------|----------------------------|
| `PULLUP`    | Resistor de pull-up interno |
| `PULLDOWN`  | Resistor de pull-down interno |
| `NOPULL`    | Flutuante (sem resistor)    |

> **Atenção:** Para uso de botões recomenda-se utilizar apenas `PULLUP`.
---



## Interface da classe

```cpp
class GPIO {
public:
    GPIO(gpio_num_t numPino, gpio_mode_t modoPino, gpio_pull_mode_t modoPull = NOPULL);

    void ligar();           // Define nível alto (1)
    void desligar();        // Define nível baixo (0)
    void inverter();        // Alterna o estado atual

    int  ler();             // Lê o nível lógico do pino

    void configPull(gpio_pull_mode_t modoPull); // Configura pull-up/down

    gpio_num_t obterPino() const; // Retorna o número do pino
};
```

---

## Como usar

#### 1. Incluir o header

```cpp
#include "GPIO.hpp"
```

#### 2. Criar uma instância

O construtor já inicializa e configura o pino automaticamente, logo não é necessário chamar funções de setup separadas.

```cpp
// Pino 2 como saída
GPIO led(GPIO2, OUTPUT);

// Pino 5 como entrada com pull-up interno
GPIO botao(GPIO5, INPUT, PULLUP);
```
> **Atenção:** A função `configPull(gpio_pull_mode_t modoPull)` pode ser usada posteriormente no objeto criado para trocar o modo de pull do pino.

#### 3. Controlar saídas

```cpp
led.ligar();     // Pino vai para nível alto
led.desligar();  // Pino vai para nível baixo
led.inverter();  // Alterna: se estava em 1, vai para 0, e vice-versa
```

#### 4. Ler entradas

```cpp
int nivel = botao.ler(); // Retorna 0 ou 1
if (nivel == 0) {
    // botão pressionado (com pull-up, nível baixo = ativo)
}
``` 

---

## Resumo dos métodos

<table>

  <tr>
    <th colspan="2">Métodos</th>
  </tr>

  <tr>
    <td><i>ligar()</i></td>
    <td>Coloca o pino no estado alto</td>
  </tr>

  <tr>
    <td><i>desligar()</i></td>
    <td>Coloca o pino no estado baixo</td>
  </tr>

  <tr>
    <td><i>inverter()</i></td>
    <td>Inverte o estado do pino</td>
  </tr>

  <tr>
    <td><i>configPull()</i></td>
    <td>Muda o resistor de pull conectado ao pino</td>
  </tr>

  <tr>
    <td><i>ler()</i></td>
    <td>Obtem o estado do pino </td>
  </tr>

</table>

> **Atenção:** `ligar()`, `desligar()` e `inverter()` só funcionam se o pino estiver configurado como `OUTPUT` ou `INPUT_OUTPUT`. Em outros modos, as chamadas são ignoradas silenciosamente.

> **Atenção:** `configPull()` só tem efeito em pinos configurados como `INPUT` ou `INPUT_OUTPUT`.

> **Atenção:** `ler()` funciona em qualquer modo e retorna o nível lógico atual lido pelo hardware.

---

## Exemplos 
> **Atenção:** Nos seguintes exemplos será incluida a abstração do Tempo, em `Tempo.hpp` que implementa uma camada de abstração para o uso de delays não bloqueantes.

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
> **Atenção:** Para aproveitar mais funcionalidades dos botões considere utilizar a classe `Botão`, que tem métodos mais especializados

---
