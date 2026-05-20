// SSD1306.cpp
#include "SSD1306.hpp"
#include "font.h" 

// Construtor
SSD1306::SSD1306(I2C* barramento_i2c, uint16_t larg, uint16_t alt, uint8_t ender) {
    i2c = barramento_i2c;
    largura = larg;
    altura = alt;
    endereco = ender;
    paginas = altura / 8;
    
    // Aloca a memória na RAM do ESP32 (128x64 pixels = 1024 bytes)
    buffer_tela = new uint8_t[largura * paginas];
}

// Destrutor
SSD1306::~SSD1306() {
    delete[] buffer_tela; // Previne vazamento de memória se o objeto for destruído
}

// O byte de controle I2C: 0x80 indica que enviaremos um Comando. 0x40 indica Dados.
void SSD1306::enviar_comando(uint8_t cmd) {
    // Usando o método de enviar um byte único do barramento
    i2c->escrever(endereco, 0x80, cmd);
}

// Inicializa a tela traduzindo a matriz de comandos (init_display) do Python
void SSD1306::inicializar() {
    uint8_t pin_config = (altura == 32) ? 0x02 : 0x12;

    uint8_t comandos_iniciais[] = {
        (uint8_t)(SET_DISP | 0x00),  // display off
        SET_MEM_ADDR, 0x00,          // Endereçamento Horizontal
        (uint8_t)(SET_DISP_START_LINE | 0x00),
        (uint8_t)(SET_SEG_REMAP | 0x01),      // Mapeamento de Coluna
        SET_MUX_RATIO, (uint8_t)(altura - 1),
        (uint8_t)(SET_COM_OUT_DIR | 0x08),    // Scan Direction
        SET_DISP_OFFSET, 0x00,
        SET_COM_PIN_CFG, pin_config,
        SET_DISP_CLK_DIV, 0x80,
        SET_PRECHARGE, 0xF1,         // Usando VCC Interno (como no MicroPython False)
        SET_VCOM_DESEL, 0x30,        
        SET_CONTRAST, 0xFF,          // Contraste máximo inicial
        SET_ENTIRE_ON,               
        SET_NORM_INV,                // Sem inversão de tela
        SET_CHARGE_PUMP, 0x14,       // Liga Charge Pump do VCC Interno
        (uint8_t)(SET_DISP | 0x01)   // display on
    };

    // Despacha todos os comandos para o hardware
    for (int i = 0; i < sizeof(comandos_iniciais); i++) {
        enviar_comando(comandos_iniciais[i]);
    }

    limpar();
    atualizar();
}

void SSD1306::desligar() {
    enviar_comando(SET_DISP | 0x00);
}

void SSD1306::ligar() {
    enviar_comando(SET_DISP | 0x01);
}

void SSD1306::contraste(uint8_t nivel) {
    enviar_comando(SET_CONTRAST);
    enviar_comando(nivel);
}

void SSD1306::inverter(bool inverter_cores) {
    enviar_comando(SET_NORM_INV | (inverter_cores & 1));
}

// Zera todo o Framebuffer
void SSD1306::limpar() {
    memset(buffer_tela, 0x00, largura * paginas);
}

// Manipulação do Bit do Pixel usando Matemática de Ponteiros
void SSD1306::desenharPixel(int x, int y, bool ligado) {
    if (x < 0 || x >= largura || y < 0 || y >= altura) return; // Fora da tela

    // Calcula em qual "página" (linha de 8 pixels) e byte o pixel se encontra
    uint16_t indice_byte = x + ((y / 8) * largura);
    uint8_t bit = y % 8;

    if (ligado) {
        buffer_tela[indice_byte] |= (1 << bit); // Acende o bit
    } else {
        buffer_tela[indice_byte] &= ~(1 << bit); // Apaga o bit
    }
}

void SSD1306::desenharChar(char caractere, int x, int y) {
    // 1. Descobre a posição no array (O caractere espaço ' ' na tabela ASCII é 32. 
    // Subtraindo 32, o espaço vira o índice 0 da nossa matriz!)
    int indice = caractere - 32;

    // Se o caractere não estiver nos 95 que temos, sai da função
    if (indice < 0 || indice >= 95) return;

    // 2. Varre as 13 linhas de altura da nossa fonte
    for (int linha = 0; linha < 13; linha++) {
        
        // Pega a linha atual (ex: 0x18)
        uint8_t byte_atual = letters[indice][linha]; 
        
        // 3. Varre os 8 bits de largura dessa linha
        for (int coluna = 0; coluna < 8; coluna++) {
            
            bool pixel_aceso = byte_atual & (0x80 >> coluna);
            int y_desenho = y + (12 - linha);

            if (pixel_aceso) {
                desenharPixel(x + coluna, y_desenho, true); // Acende
            } else {
                desenharPixel(x + coluna, y_desenho, false); // Apaga (fundo transparente/preto)
            }
        }
    }
}

void SSD1306::escreverTexto(const char* texto, int x, int y) {
    int cursor_x = x; // Controla onde a próxima letra vai ser desenhada

    // Lê a string inteira até encontrar o final ('\0')
    while (*texto) {
        desenharChar(*texto, cursor_x, y);
        
        // Avança 8 pixels para a direita para desenhar a próxima letra ao lado
        cursor_x += 8; 
        
        texto++; // Pula para a próxima letra da string
    }
}

// Equivalente a função show(), despeja a memória RAM na tela via I2C
void SSD1306::atualizar() {
    uint8_t x0 = 0;
    uint8_t x1 = largura - 1;

    // Correção para displays estreitos do código original (shift de 32 colunas)
    if (largura == 64) {
        x0 += 32;
        x1 += 32;
    }

    enviar_comando(SET_COL_ADDR);
    enviar_comando(x0);
    enviar_comando(x1);

    enviar_comando(SET_PAGE_ADDR);
    enviar_comando(0);
    enviar_comando(paginas - 1);

    // Usa a sua classe I2C para fazer o block transfer, onde o registrador é 0x40 (Modo de Dados)
    i2c->escrever_bloco(endereco, 0x40, buffer_tela, largura * paginas);
}