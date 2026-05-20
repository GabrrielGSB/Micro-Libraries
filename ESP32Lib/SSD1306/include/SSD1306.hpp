// SSD1306.hpp
#pragma once
#ifndef SSD1306_HPP
#define SSD1306_HPP

#include "I2C.hpp"
#include <stdint.h>
#include <string.h>

// Definição dos Registradores (Igual ao MicroPython)
#define SET_CONTRAST        0x81
#define SET_ENTIRE_ON       0xA4
#define SET_NORM_INV        0xA6
#define SET_DISP            0xAE
#define SET_MEM_ADDR        0x20
#define SET_COL_ADDR        0x21
#define SET_PAGE_ADDR       0x22
#define SET_DISP_START_LINE 0x40
#define SET_SEG_REMAP       0xA0
#define SET_MUX_RATIO       0xA8
#define SET_COM_OUT_DIR     0xC0
#define SET_DISP_OFFSET     0xD3
#define SET_COM_PIN_CFG     0xDA
#define SET_DISP_CLK_DIV    0xD5
#define SET_PRECHARGE       0xD9
#define SET_VCOM_DESEL      0xDB
#define SET_CHARGE_PUMP     0x8D

class SSD1306 {
protected:
    I2C* i2c;                 // Ponteiro para o barramento I2C
    uint8_t endereco;         // Endereço I2C do display (padrão 0x3C)
    uint16_t largura;
    uint16_t altura;
    uint8_t paginas;
    
    // O FrameBuffer: A nossa "tela" desenhada na memória RAM do ESP32
    uint8_t* buffer_tela;

    // Métodos internos de transmissão
    void enviar_comando(uint8_t cmd);
    
public:
    // Construtor e Destrutor
    SSD1306(I2C* barramento_i2c, uint16_t larg = 128, uint16_t alt = 64, uint8_t ender = 0x3C);
    ~SSD1306();

    // Funções de Inicialização e Energia baseadas no MicroPython
    void inicializar();
    void ligar();
    void desligar();
    void contraste(uint8_t nivel);
    void inverter(bool inverter_cores);

    // Funções de Desenho Básico (Framebuffer)
    void desenharPixel(int x, int y, bool ligado);
    void desenharChar(char caractere, int x, int y);
    void escreverTexto(const char* texto, int x, int y);
    void limpar();
    
    // Atualiza a tela física com o que foi desenhado no FrameBuffer
    void atualizar();
};

#endif