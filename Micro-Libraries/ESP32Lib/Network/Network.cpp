#include "Network.hpp"
#include "nvs_flash.h"
#include "esp_log.h"
#include <cstring>

// Tag de log para monitoramento via terminal
static const char* TAG = "NETWORK_CLASS";

// Inicialização dos membros estáticos declarados no .hpp
EventGroupHandle_t Network::_wifi_event_group = nullptr;
int Network::_tentativas = 0;

// ==========================================
// HANDLER DE EVENTOS (Ponte C -> C++)
// ==========================================
void Network::wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    Network* instance = static_cast<Network*>(arg);

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Rádio Wi-Fi iniciado. Conectando...");
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (_tentativas < MAX_RECONEXOES) {
            esp_wifi_connect();

            _tentativas++;
            ESP_LOGW(TAG, "Tentativa de reconexão %d/%d...", _tentativas, MAX_RECONEXOES);
        } else {
            // Se esgotar as tentativas, avisa o Event Group liberando o fluxo da CPU
            xEventGroupSetBits(_wifi_event_group, WIFI_FAIL_BIT);
        }
        instance->_conectado = false;
        ESP_LOGE(TAG, "Desconectado do ponto de acesso.");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;

        ESP_LOGI(TAG, "Conectado com Sucesso! Endereço IP: " IPSTR, IP2STR(&event->ip_info.ip));

        _tentativas = 0;

        instance->_conectado = true;
        // Avisa o Event Group que a conexão foi um sucesso
        xEventGroupSetBits(_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}


// ==========================================
// CONSTRUTOR E MÉTODOS PRIVADOS
// ==========================================
Network::Network() : _conectado(false), _netif_control(nullptr) {
    _inicializar_nvs();
    _inicializar_stack_rede();
}
Network::~Network() {
    desconectar();
}

void Network::_inicializar_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}
void Network::_inicializar_stack_rede() {
    // Inicializa a camada LwIP interna
    ESP_ERROR_CHECK(esp_netif_init());
    
    // Cria o loop de eventos padrão do sistema se ele ainda não existir
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Cria a interface padrão Wi-Fi Station
    _netif_control = esp_netif_create_default_wifi_sta();
    
    // Configura o driver Wi-Fi com os parâmetros de fábrica padrão
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Cria o Event Group do FreeRTOS necessário para a sincronia
    if (_wifi_event_group == nullptr) {
        _wifi_event_group = xEventGroupCreate();
    }
}

std::string Network::_mapear_autenticacao(wifi_auth_mode_t mode) {
    switch (mode) {
        case WIFI_AUTH_OPEN:          return "Aberta";
        case WIFI_AUTH_WEP:           return "WEP";
        case WIFI_AUTH_WPA_PSK:       return "WPA-PSK";
        case WIFI_AUTH_WPA2_PSK:      return "WPA2-PSK";
        case WIFI_AUTH_WPA_WPA2_PSK:  return "WPA/WPA2-PSK";
        case WIFI_AUTH_WPA3_PSK:      return "WPA3-PSK";
        case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3-PSK";
        default:                      return "Desconhecida";
    }
}


void Network::escanear() {
    bool estava_conectado = _conectado;
    if (estava_conectado) {
        esp_wifi_disconnect();
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    wifi_scan_config_t scan_config = {};
    scan_config.ssid = nullptr;
    scan_config.bssid = nullptr;
    scan_config.channel = 0;
    scan_config.show_hidden = false;
    scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;

    ESP_LOGI(TAG, "Iniciando varredura de redes Wi-Fi...");
    
    esp_err_t err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar o scan: %s", esp_err_to_name(err));
        return;
    }

    uint16_t quantidade_redes = 0;
    esp_wifi_scan_get_ap_num(&quantidade_redes);
    
    if (quantidade_redes == 0) {
        ESP_LOGW(TAG, "Nenhuma rede Wi-Fi encontrada por perto.");
        if (estava_conectado) esp_wifi_connect();
        return;
    }

    wifi_ap_record_t* lista_bruta = new wifi_ap_record_t[quantidade_redes];
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&quantidade_redes, lista_bruta));

    // Imprime o cabeçalho formatado a partir da própria classe modular
    std::printf("\n======================================================================\n");
    std::printf("%-32s | %-8s | %-5s | %-15s\n", "SSID (Nome da Rede)", "Sinal", "Canal", "Seguranca");
    std::printf("======================================================================\n");

    for (int i = 0; i < quantidade_redes; i++) {
        // Filtra e trata redes com SSID oculto
        std::string nome_rede = (strlen((char*)lista_bruta[i].ssid) == 0) ? 
                                "[Rede Oculta]" : (char*)lista_bruta[i].ssid;

        std::string seguranca = _mapear_autenticacao(lista_bruta[i].authmode);

        std::printf("%-32s | %-5d dBm | %-5d | %-15s\n", 
                    nome_rede.c_str(), 
                    lista_bruta[i].rssi, 
                    lista_bruta[i].primary, 
                    seguranca.c_str());
    }
    std::printf("======================================================================\n");
    std::printf("Total de redes encontradas: %d\n\n", quantidade_redes);

    delete[] lista_bruta;

    if (estava_conectado) {
        esp_wifi_connect();
    }
}

// ==========================================
// CONECTAR / DESCONECTAR
// ==========================================
bool Network::conectar(const std::string& ssid, const std::string& password) {
    _ssid       = ssid;
    _password   = password;
    _tentativas = 0;

    // Registra os Handlers de Eventos passando a instância atual (this) como argumento
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, 
                                                        &Network::wifi_event_handler, this, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, 
                                                        &Network::wifi_event_handler, this, &instance_got_ip));

    // Configura as credenciais na estrutura nativa da ESP-IDF
    wifi_config_t wifi_config = {};
    std::strncpy((char*)wifi_config.sta.ssid, _ssid.c_str(), sizeof(wifi_config.sta.ssid));
    std::strncpy((char*)wifi_config.sta.password, _password.c_str(), sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK; // Configuração padrão de segurança

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    // Liga fisicamente o rádio
    ESP_ERROR_CHECK(esp_wifi_start());

    // Bloqueia de forma eficiente a CPU aguardando a resposta do Event Loop
    EventBits_t bits = xEventGroupWaitBits(
        _wifi_event_group,
        WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
        pdFALSE, // Não limpa os bits automaticamente antes de sair
        pdFALSE, // Não precisa esperar ambos os bits, qualquer um serve
        portMAX_DELAY // Aguarda o tempo que for necessário
    );

    // Avalia o resultado com base no bit que destravou o Group
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Interface de Rede pronta!");
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Falha crítica ao conectar no SSID: %s", _ssid.c_str());
        return false;
    } else {
        ESP_LOGE(TAG, "Evento inesperado no timeout.");
        return false;
    }
}

void Network::desconectar() {
    if (_conectado) {
        esp_wifi_disconnect();
        esp_wifi_stop();
        _conectado = false;
        xEventGroupClearBits(_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
        ESP_LOGI(TAG, "Módulo Wi-Fi desligado com sucesso.");
    }
}

// ==========================================
// ENVIO DE DADOS HTTP CLIENT (POST)
// ==========================================
int Network::enviarDadosPOST(const std::string& url, const std::string& json_payload) {
    if (!_conectado) {
        ESP_LOGW(TAG, "Envio cancelado: Sem conexão com a internet.");
        return -1;
    }

    // Configuração do Cliente HTTP
    esp_http_client_config_t config_client = {};
    config_client.url = url.c_str();
    config_client.method = HTTP_METHOD_POST;
    config_client.timeout_ms = 5000; // 5 segundos de limite

    // Inicializa o handle do cliente HTTP
    esp_http_client_handle_t client = esp_http_client_init(&config_client);
    if (client == nullptr) {
        ESP_LOGE(TAG, "Falha ao alocar memória para o Cliente HTTP.");
        return -1;
    }

    // Define os cabeçalhos da requisição para aceitar JSON
    esp_http_client_set_header(client, "Content-Type", "application/json");
    
    // Atribui o corpo (Payload) da mensagem
    esp_http_client_set_post_field(client, json_payload.c_str(), json_payload.length());

    int status_code = -1;
    // Executa a requisição de forma síncrona/bloqueante
    esp_err_t err = esp_http_client_perform(client);

    if (err == ESP_OK) {
        status_code = esp_http_client_get_status_code(client);
        long long tamanho_resposta = esp_http_client_get_content_length(client);
        ESP_LOGI(TAG, "HTTP POST Sucesso. Status Code: %d, Tamanho da Resposta: %lld", status_code, tamanho_resposta);
    } else {
        ESP_LOGE(TAG, "Falha na requisição HTTP POST: %s", esp_err_to_name(err));
    }

    // OBRIGATÓRIO: Limpa a memória e fecha os sockets abertos
    esp_http_client_cleanup(client);

    return status_code;
}





