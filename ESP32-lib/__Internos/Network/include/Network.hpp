#pragma once
#ifndef NETWORK_HPP
#define NETWORK_HPP

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_http_server.h"   // para o portal
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include <string>
#include <vector>

struct WifiNetworkInfo {
    std::string ssid;
    int rssi;
    uint8_t canal;
    std::string autenticacao;
};

class Network {
public:
    // Singleton
    static Network& getInstance();

    // Remover cópia/movimento
    Network(const Network&)            = delete;
    Network& operator=(const Network&) = delete;
    Network(Network&&)                 = delete;
    Network& operator=(Network&&)      = delete;

    ~Network();

    // Escaneia redes disponíveis
    std::vector<WifiNetworkInfo> escanear();

    /*
     * Método único de conexão:
     * - Se chamado sem argumentos (conectar()), abre portal cativo.
     * - Se chamado com SSID (e opcionalmente senha), conecta diretamente.
     */
    bool conectar(const std::string& ssid = "", const std::string& password = "");

    // Desconecta e desliga rádio
    void desconectar();

    // Estado da conexão
    bool estaConectado() const { return _conectado; }

    // Envio HTTP POST
    int enviarDadosPOST(const std::string& url, const std::string& json_payload);

private:
    Network();   // privado (Singleton)

    // --- Estado interno ---
    bool _conectado;
    std::string _ssid;
    std::string _password;
    int _tentativas;
    static const int MAX_RECONEXOES = 5;

    // Event Group para sincronismo WiFi
    EventGroupHandle_t _wifi_event_group;
    static const EventBits_t WIFI_CONNECTED_BIT = BIT0;
    static const EventBits_t WIFI_FAIL_BIT      = BIT1;

    // Handlers de evento (registrados uma única vez)
    esp_event_handler_instance_t _handler_any_id;
    esp_event_handler_instance_t _handler_got_ip;
    bool _handlers_registrados;

    // Callback estático (bridge C → C++)
    static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                   int32_t event_id, void* event_data);

    // --- Portal de configuração ---
    httpd_handle_t _server;
    SemaphoreHandle_t _credenciais_prontas;

    // Handlers HTTP do portal
    static esp_err_t http_get_handler(httpd_req_t *req);
    static esp_err_t http_post_handler(httpd_req_t *req);

    // Métodos internos do portal
    void _iniciar_portal(const std::string& ap_ssid = "ESP32_Config", 
                         const std::string& ap_password = "");
    void _parar_portal();

    // --- Inicialização única ---
    void _inicializar_nvs();
    void _inicializar_stack_rede();

    // Utilitário
    std::string _mapear_autenticacao(wifi_auth_mode_t mode);
};

#endif