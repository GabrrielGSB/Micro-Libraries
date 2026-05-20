#pragma once
#ifndef NETWORK_HPP
#define NETWORK_HPP

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <string>
#include <vector> 

// Estrutura para encapsular os dados de cada rede encontrada
struct WifiNetworkInfo {
    std::string ssid;
    int rssi;           // Força do sinal (ex: -65 dBm)
    uint8_t canal;
    std::string autenticacao;
};

class Network {
private:
    // Configurações internas de estado
    bool _conectado;
    std::string _ssid;
    std::string _password;

    // Handles nativos da ESP-IDF e FreeRTOS
    esp_netif_t* _netif_control;
    static EventGroupHandle_t _wifi_event_group;
    
    // Bits de controle para sincronismo de eventos
    static const int WIFI_CONNECTED_BIT = BIT0;
    static const int WIFI_FAIL_BIT      = BIT1;

    // Contador de tentativas de reconexão interno
    static int _tentativas;
    static const int MAX_RECONEXOES = 5;

    // Handler de eventos estático (A ponte entre o C do IDF e o C++ da nossa Classe)
    static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data);

    // Métodos privados de inicialização do ecossistema
    void _inicializar_nvs();
    void _inicializar_stack_rede();

    std::string _mapear_autenticacao(wifi_auth_mode_t mode);

public:
    // Construtor: Inicializa NVS, LwIP e Event Loops básicos
    Network();
    ~Network();

    void escanear();

    // Bloqueia e tenta conectar. Retorna true se obtiver sucesso (IP alocado)
    bool conectar(const std::string& ssid, const std::string& password);
    
    // Desconecta do Wi-Fi e desliga o rádio temporariamente
    void desconectar();

    // Consulta rápida de estado
    bool estaConectado() const { return _conectado; }

    // Envio de dados HTTP POST (Ex: Strings brutas ou payloads em JSON)
    // Retorna o Status Code HTTP do servidor (ex: 200, 201) ou -1 em caso de erro interno
    int enviarDadosPOST(const std::string& url, const std::string& json_payload);
};

#endif // NETWORK_HPP