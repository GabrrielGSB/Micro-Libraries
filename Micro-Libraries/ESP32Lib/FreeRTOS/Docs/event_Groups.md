# Event Groups
## Conceito Geral
Um Event Group é essencialmente um inteiro de 24 bits, onde cada bit representa um evento independente. Tasks podem setar bits, limpar bits e esperar por combinações de bits.

```
Bit:  23  22  21  ...  2    1    0
       0   0   0  ...  0    1    1
                            ↑    ↑
                         WiFi  Sensor
                       conectado  pronto
```

Pense como uma lousa compartilhada onde qualquer task pode escrever "evento X aconteceu" e outras tasks ficam olhando e esperando pela combinação que precisam.

## Como usar

```C
EventGroupHandle_t xEventGroup;
xEventGroup = xEventGroupCreate();
```

```C
// Define os bits que você quer usar (como constantes)
#define BIT_WIFI     (1 << 0)  // bit 0 = 0x01
#define BIT_SENSOR   (1 << 1)  // bit 1 = 0x02
#define BIT_USUARIO  (1 << 2)  // bit 2 = 0x04

// Em alguma task ou ISR:
xEventGroupSetBits(xEventGroup, BIT_WIFI);
```

```C
EventBits_t bits = xEventGroupWaitBits(
    xEventGroup,           // o handle
    BIT_WIFI | BIT_SENSOR, // quais bits esperar
    pdTRUE,                // limpar os bits ao sair? (pdTRUE = sim)
    pdTRUE,                // esperar TODOS (pdTRUE) ou QUALQUER UM (pdFALSE)?
    pdMS_TO_TICKS(5000)    // timeout: 5 segundos
);

if ((bits & (BIT_WIFI | BIT_SENSOR)) == (BIT_WIFI | BIT_SENSOR)) {
    // Ambos os eventos ocorreram!
}
```

```C
xEventGroupClearBits(xEventGroup, BIT_WIFI);
```

```C
EventBits_t estado = xEventGroupGetBits(xEventGroup);
```