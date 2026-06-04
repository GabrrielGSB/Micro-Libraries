# Classe `I2C`

Classe para comunicação com periféricos via protocolo **I2C** no ESP32, construída sobre a API `driver/i2c_master.h` do ESP-IDF. Gerencia o barramento e os dispositivos automaticamente — sem necessidade de configurar handles manualmente.

---

## Dependências

| Arquivo / Biblioteca    | Papel                                              |
|-------------------------|----------------------------------------------------|
| `driver/i2c_master.h`   | Driver I2C master do ESP-IDF (nova API)            |
| `esp_err.h`             | Tipo `esp_err_t` para retorno de erros             |
| `GPIO.hpp`              | Tipos `gpio_num_t` para definir os pinos SDA/SCL   |

---

## Macros de frequência

| Macro         | Frequência  | Uso                                      |
|---------------|-------------|------------------------------------------|
| `NORMAL_MODE` | 100 kHz     | Padrão — compatível com todos os sensores|
| `FAST_MODE`   | 400 kHz     | Sensores e displays mais rápidos como a câmera        |

---

## Interface da classe

```cpp
class I2C {
public:
    I2C(i2c_port_t porta_i2c,
        gpio_num_t pino_sda,
        gpio_num_t pino_scl,
        uint32_t frequencia_hz = NORMAL_MODE);

    ~I2C(); // remove todos os dispositivos registrados

    // Escrita de 1 byte em registrador de 8 bits
    esp_err_t escrever(uint8_t endereco, uint8_t registrador, uint8_t dado);

    // Escrita de 1 byte em registrador de 16 bits
    esp_err_t escrever(uint8_t endereco, uint16_t registrador, uint8_t dado);

    // Escrita de múltiplos bytes a partir de um registrador
    esp_err_t escrever_bloco(uint8_t endereco, uint8_t registrador,
                             const uint8_t *dados, size_t tamanho);

    // Leitura de N bytes a partir de um registrador de 8 bits
    esp_err_t ler(uint8_t endereco, uint8_t registrador,
                  uint8_t *buffer, size_t tamanho);

    // Leitura de N bytes a partir de um registrador de 16 bits
    esp_err_t ler(uint8_t endereco, uint16_t registrador,
                  uint8_t *buffer, size_t tamanho);

    i2c_port_t getPorta();
};
```

---

## Como usar

### 1. Incluir o header

```cpp
#include "I2C.hpp"
```

### 2. Criar uma instância do barramento

```cpp
// Porta 0, pinos SDA=21 e SCL=22, velocidade padrão (100 kHz)
I2C barramento(I2C_NUM_0, GPIO21, GPIO22);

// Porta 1 em fast mode (400 kHz)
I2C barramento(I2C_NUM_1, GPIO33, GPIO32, FAST_MODE);
```

O construtor inicializa o barramento automaticamente. Se a mesma porta já tiver sido inicializada por outro objeto, o barramento é compartilhado sem reinstalação.

---

## Métodos de comunicação

### `escrever()` — Escreve 1 byte em um registrador

Suporta registradores de **8 bits** (maioria dos sensores) e **16 bits** (EEPROMs, câmeras, displays).

```cpp
// Registrador de 8 bits
barramento.escrever(0x68, 0x6B, 0x00); // acorda o MPU-6050 (endereço 0x68)

// Registrador de 16 bits
barramento.escrever(0x3C, 0x0100, 0xFF); // display com endereçamento estendido
```

---

### `escrever_bloco()` — Escreve múltiplos bytes consecutivos

```cpp
esp_err_t escrever_bloco(uint8_t endereco, uint8_t registrador,
                         const uint8_t *dados, size_t tamanho);
```

Transmite um array de bytes a partir de um registrador. Útil para configurar múltiplos registradores de uma vez ou enviar buffers de dados.

```cpp
uint8_t config[3] = { 0x01, 0x80, 0x00 };
barramento.escrever_bloco(0x48, 0x01, config, sizeof(config));
```

---

### `ler()` — Lê N bytes a partir de um registrador

Realiza a sequência padrão I2C: envia o endereço do registrador e lê os bytes de resposta em uma única transação.

```cpp
uint8_t buffer[6];

// Registrador de 8 bits — lê 6 bytes (ex: acelerômetro + giroscópio do MPU-6050)
barramento.ler(0x68, 0x3B, buffer, 6);

// Registrador de 16 bits
barramento.ler(0x50, 0x0000, buffer, 4);
```

---

## Gerenciamento automático de dispositivos

Os dispositivos não precisam ser registrados manualmente. Na primeira comunicação com um endereço, a classe cria e armazena o handle internamente. Chamadas posteriores reutilizam o mesmo handle sem custo adicional.

```cpp
// Na primeira chamada com 0x68, o dispositivo é criado automaticamente
barramento.escrever(0x68, 0x6B, 0x00);

// Chamadas seguintes reutilizam o handle já existente
barramento.ler(0x68, 0x3B, buffer, 6);
```

Ao destruir o objeto `I2C`, todos os dispositivos registrados são removidos corretamente.

---

## Tratamento de erros

Todos os métodos de comunicação retornam `esp_err_t`. O valor `ESP_OK` indica sucesso; qualquer outro indica falha.

```cpp
esp_err_t resultado = barramento.escrever(0x68, 0x6B, 0x00);

if (resultado != ESP_OK) {
    // dispositivo não respondeu ou erro de comunicação
}
```

Valores comuns de erro:

| Código                 | Causa provável                                  |
|------------------------|-------------------------------------------------|
| `ESP_OK`               | Sucesso                                         |
| `ESP_ERR_TIMEOUT`      | Dispositivo não respondeu no tempo limite (1 s) |
| `ESP_ERR_NOT_FOUND`    | Nenhum dispositivo no endereço especificado     |
| `ESP_ERR_NO_MEM`       | Falha de alocação de memória (em `escrever_bloco`) |

---

## Compartilhamento de barramento

Múltiplos objetos `I2C` podem compartilhar a mesma porta física. O barramento é criado apenas na primeira instância e reutilizado pelas demais — padrão útil quando drivers diferentes precisam acessar o mesmo barramento.

```cpp
I2C sensor1(I2C_NUM_0, GPIO21, GPIO22); // cria o barramento
I2C sensor2(I2C_NUM_0, GPIO21, GPIO22); // compartilha o mesmo barramento
```

---

## Exemplos completos

### Lendo o MPU-6050 (acelerômetro/giroscópio)

```cpp
#include "I2C.hpp"
#include "Tempo.hpp"

#define MPU6050_ADDR  0x68
#define REG_PWR_MGMT  0x6B
#define REG_ACCEL     0x3B

extern "C" void app_main() {
    I2C i2c(I2C_NUM_0, GPIO21, GPIO22);

    // Acorda o sensor (sai do modo sleep)
    i2c.escrever(MPU6050_ADDR, REG_PWR_MGMT, 0x00);
    delay_ms(100);

    uint8_t raw[6];
    while (true) {
        i2c.ler(MPU6050_ADDR, REG_ACCEL, raw, 6);

        int16_t ax = (raw[0] << 8) | raw[1];
        int16_t ay = (raw[2] << 8) | raw[3];
        int16_t az = (raw[4] << 8) | raw[5];

        // usa ax, ay, az...
        delay_ms(10);
    }
}
```

### Escrevendo em uma EEPROM 24C02 (registrador 16 bits)

```cpp
#include "I2C.hpp"

#define EEPROM_ADDR 0x50

extern "C" void app_main() {
    I2C i2c(I2C_NUM_0, GPIO21, GPIO22);

    // Escreve 0xAB no endereço de memória 0x0010
    i2c.escrever(EEPROM_ADDR, (uint16_t)0x0010, 0xAB);
    delay_ms(5); // EEPROMs precisam de tempo para gravar

    // Lê de volta
    uint8_t valor;
    i2c.ler(EEPROM_ADDR, (uint16_t)0x0010, &valor, 1);
}
```

### Dois sensores no mesmo barramento

```cpp
#include "I2C.hpp"

extern "C" void app_main() {
    I2C imu(I2C_NUM_0, GPIO21, GPIO22);          // barramento criado aqui
    I2C baro(I2C_NUM_0, GPIO21, GPIO22, FAST_MODE); // compartilha o mesmo

    uint8_t buf_imu[6], buf_baro[3];

    imu.ler(0x68, 0x3B, buf_imu,  6);
    baro.ler(0x76, 0xF7, buf_baro, 3);
}
```

---

## Resumo dos métodos

| Método           | Registrador | Direção  | Bytes     |
|------------------|-------------|----------|-----------|
| `escrever()`     | 8 ou 16 bits| Escrita  | 1         |
| `escrever_bloco` | 8 bits      | Escrita  | N         |
| `ler()`          | 8 ou 16 bits| Leitura  | N         |
| `getPorta()`     | —           | —        | Retorna a porta I2C em uso |