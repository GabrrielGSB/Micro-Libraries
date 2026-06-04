#include "SSD1306.hpp"
#include "font.h"

SSD1306::SSD1306(i2c_port_t porta_i2c,
                 gpio_num_t pino_sda,
                 gpio_num_t pino_scl,
                 uint16_t larg,
                 uint16_t alt,
                 uint8_t  ender,
                 uint32_t freq)
    : I2C(porta_i2c, pino_sda, pino_scl, freq)
{
    endereco_display = ender;
    largura  = larg;
    altura   = alt;
    paginas  = altura / 8;

    buffer_tela = new uint8_t[largura * paginas];
}

SSD1306::~SSD1306() {
    delete[] buffer_tela;
}

// O registrador 0x80 sinaliza ao SSD1306 que o próximo byte é um comando
void SSD1306::enviar_comando(uint8_t cmd) {
    escrever(endereco_display, (uint8_t)0x80, cmd);
}

void SSD1306::inicializar() {
    uint8_t pin_config = (altura == 32) ? 0x02 : 0x12;

    uint8_t comandos_iniciais[] = {
        (uint8_t)(SET_DISP | 0x00),
        SET_MEM_ADDR, 0x00,
        (uint8_t)(SET_DISP_START_LINE | 0x00),
        (uint8_t)(SET_SEG_REMAP | 0x01),
        SET_MUX_RATIO, (uint8_t)(altura - 1),
        (uint8_t)(SET_COM_OUT_DIR | 0x08),
        SET_DISP_OFFSET, 0x00,
        SET_COM_PIN_CFG, pin_config,
        SET_DISP_CLK_DIV, 0x80,
        SET_PRECHARGE, 0xF1,
        SET_VCOM_DESEL, 0x30,
        SET_CONTRAST, 0xFF,
        SET_ENTIRE_ON,
        SET_NORM_INV,
        SET_CHARGE_PUMP, 0x14,
        (uint8_t)(SET_DISP | 0x01)
    };

    for (int i = 0; i < (int)sizeof(comandos_iniciais); i++) {
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

void SSD1306::limpar() {
    memset(buffer_tela, 0x00, largura * paginas);
}

void SSD1306::desenharPixel(int x, int y, bool ligado) {
    if (x < 0 || x >= largura || y < 0 || y >= altura) return;

    uint16_t indice_byte = x + ((y / 8) * largura);
    uint8_t  bit         = y % 8;

    if (ligado) {
        buffer_tela[indice_byte] |=  (1 << bit);
    } else {
        buffer_tela[indice_byte] &= ~(1 << bit);
    }
}

void SSD1306::desenharChar(char caractere, int x, int y) {
    int indice = caractere - 32;
    if (indice < 0 || indice >= 95) return;

    for (int linha = 0; linha < 13; linha++) {
        uint8_t byte_atual = letters[indice][linha];
        for (int coluna = 0; coluna < 8; coluna++) {
            bool pixel_aceso = byte_atual & (0x80 >> coluna);
            desenharPixel(x + coluna, y + (12 - linha), pixel_aceso);
        }
    }
}

void SSD1306::escreverTexto(const char* texto, int x, int y) {
    int cursor_x = x;
    while (*texto) {
        desenharChar(*texto, cursor_x, y);
        cursor_x += 8;
        texto++;
    }
}

void SSD1306::atualizar() {
    uint8_t x0 = 0;
    uint8_t x1 = largura - 1;

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

    // 0x40 = registrador de dados do SSD1306
    escrever_bloco(endereco_display, (uint8_t)0x40, buffer_tela, largura * paginas);
}