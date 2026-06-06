#include "Network.hpp"
#include "nvs_flash.h"
#include "esp_log.h"
#include <cstring>
#include <algorithm>

static const char* TAG = "NETWORK";

// Página HTML do portal
static const char* HTML_FORM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Configurar Wi-Fi</title>
    <style>
        body { font-family: Arial; background: #f2f2f2; margin: 2em; }
        .container { background: white; padding: 2em; border-radius: 10px; max-width: 400px; margin: auto; }
        input { width: 100%; padding: 10px; margin: 8px 0; box-sizing: border-box; }
        input[type=submit] { background: #4CAF50; color: white; border: none; border-radius: 5px; }
    </style>
</head>
<body>
    <div class="container">
        <h2>Conectar à rede</h2>
        <form action="/connect" method="POST">
            <label>SSID:</label>
            <input type="text" name="ssid" required>
            <label>Senha:</label>
            <input type="password" name="password">
            <input type="submit" value="Conectar">
        </form>
    </div>
</body>
</html>
)rawliteral";

// ==============================
// Singleton
// ==============================
Network& Network::getInstance() {
    static Network instance;
    return instance;
}

// ==============================
// Handlers de eventos WiFi
// ==============================
void Network::wifi_event_handler(void* arg, esp_event_base_t event_base,
                                 int32_t event_id, void* event_data) {
    Network* self = static_cast<Network*>(arg);
    if (!self) return;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Rádio iniciado, conectando...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (self->_tentativas < MAX_RECONEXOES) {
            esp_wifi_connect();
            self->_tentativas++;
            ESP_LOGW(TAG, "Reconexão %d/%d...", self->_tentativas, MAX_RECONEXOES);
        } else {
            xEventGroupSetBits(self->_wifi_event_group, WIFI_FAIL_BIT);
        }
        self->_conectado = false;
        ESP_LOGE(TAG, "Desconectado.");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        auto* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "IP obtido: " IPSTR, IP2STR(&event->ip_info.ip));
        self->_tentativas = 0;
        self->_conectado = true;
        xEventGroupSetBits(self->_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// ==============================
// Handlers HTTP do portal
// ==============================
esp_err_t Network::http_get_handler(httpd_req_t *req) {
    httpd_resp_send(req, HTML_FORM, strlen(HTML_FORM));
    return ESP_OK;
}

esp_err_t Network::http_post_handler(httpd_req_t *req) {
    char buf[100];
    int ret, remaining = req->content_len;
    std::string body;

    while (remaining > 0) {
        int to_read = std::min(remaining, (int)sizeof(buf) - 1);
        ret = httpd_req_recv(req, buf, to_read);
        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) continue;
            return ESP_FAIL;
        }
        buf[ret] = '\0';
        body += buf;
        remaining -= ret;
    }

    auto get_param = [&](const std::string& key) -> std::string {
        size_t start = body.find(key + "=");
        if (start == std::string::npos) return "";
        start += key.length() + 1;
        size_t end = body.find("&", start);
        if (end == std::string::npos) end = body.length();
        return body.substr(start, end - start);
    };

    // ✅ Decodifica URL encoding (ex: %40 → @, + → espaço)
    auto url_decode = [](const std::string& s) -> std::string {
        std::string out;
        out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i) {
            if (s[i] == '+') {
                out += ' ';
            } else if (s[i] == '%' && i + 2 < s.size()) {
                char hex[3] = { s[i+1], s[i+2], '\0' };
                out += static_cast<char>(strtol(hex, nullptr, 16));
                i += 2;
            } else {
                out += s[i];
            }
        }
        return out;
    };

    Network& net = Network::getInstance();
    net._ssid     = url_decode(get_param("ssid"));      // ✅ decodificado
    net._password = url_decode(get_param("password"));  // ✅ decodificado

    xSemaphoreGive(net._credenciais_prontas);

    const char* resp = "Credenciais recebidas! Tentando conectar...";
    httpd_resp_send(req, resp, strlen(resp));
    return ESP_OK;
}

// ==============================
// Construtor / Destrutor
// ==============================
Network::Network() : _conectado(false),
                     _tentativas(0),
                     _handlers_registrados(false),
                     _server(nullptr),
                     _credenciais_prontas(nullptr) {
    _inicializar_nvs();
    _inicializar_stack_rede();

    _wifi_event_group = xEventGroupCreate();
    if (!_wifi_event_group) {
        ESP_LOGE(TAG, "Falha no Event Group");
        abort();
    }

    if (esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                            &Network::wifi_event_handler, this,
                                            &_handler_any_id) == ESP_OK &&
        esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                            &Network::wifi_event_handler, this,
                                            &_handler_got_ip) == ESP_OK) {
        _handlers_registrados = true;
    } else {
        ESP_LOGE(TAG, "Falha ao registrar handlers");
    }
}

Network::~Network() {
    if (_handlers_registrados) {
        esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, _handler_any_id);
        esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, _handler_got_ip);
    }
    if (_wifi_event_group) {
        vEventGroupDelete(_wifi_event_group);
    }
    // Limpa semáforo se existir
    if (_credenciais_prontas) {
        vSemaphoreDelete(_credenciais_prontas);
    }
}

// ==============================
// Inicialização única
// ==============================
void Network::_inicializar_nvs() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void Network::_inicializar_stack_rede() {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();   // Interface STA
    esp_netif_create_default_wifi_ap();   
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
}

// ==============================
// Mapeamento de autenticação
// ==============================
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

// ==============================
// Escaneamento
// ==============================
std::vector<WifiNetworkInfo> Network::escanear() {
    std::vector<WifiNetworkInfo> resultado;

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    wifi_scan_config_t scan_cfg = {};
    scan_cfg.scan_type = WIFI_SCAN_TYPE_ACTIVE;

    ESP_LOGI(TAG, "Iniciando scan...");
    if (esp_wifi_scan_start(&scan_cfg, true) != ESP_OK) {
        ESP_LOGE(TAG, "Falha no scan");
        return resultado;
    }

    uint16_t num = 0;
    esp_wifi_scan_get_ap_num(&num);
    if (num == 0) {
        ESP_LOGW(TAG, "Nenhuma rede encontrada.");
        return resultado;
    }

    wifi_ap_record_t* lista = new wifi_ap_record_t[num];
    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&num, lista));

    for (int i = 0; i < num; ++i) {
        WifiNetworkInfo info;
        info.ssid = (strlen((char*)lista[i].ssid) == 0) ? "[Rede Oculta]" : (char*)lista[i].ssid;
        info.rssi = lista[i].rssi;
        info.canal = lista[i].primary;
        info.autenticacao = _mapear_autenticacao(lista[i].authmode);
        resultado.push_back(info);
    }
    delete[] lista;

    esp_wifi_stop();
    return resultado;
}

// ==============================
// Portal de configuração (privado)
// ==============================
void Network::_iniciar_portal(const std::string& ap_ssid, const std::string& ap_password) {
    // ✅ Desconectar antes de parar limpa o estado do driver STA
    esp_wifi_disconnect();
    esp_wifi_stop();
    vTaskDelay(pdMS_TO_TICKS(300));  // 100ms era pouco; 300ms garante cleanup

    // ✅ ESP_ERROR_CHECK aqui é crítico — sem ele, falha é invisível
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    if (!_credenciais_prontas) {
        _credenciais_prontas = xSemaphoreCreateBinary();
        if (!_credenciais_prontas) { ESP_LOGE(TAG, "Falha ao criar semáforo"); return; }
    }

    wifi_config_t ap_config = {};
    strncpy((char*)ap_config.ap.ssid, ap_ssid.c_str(), sizeof(ap_config.ap.ssid) - 1);
    ap_config.ap.ssid_len = (uint8_t)ap_ssid.length();
    ap_config.ap.max_connection = 4;
    ap_config.ap.channel = 6;  // Canal 1 tem mais interferência; 6 ou 11 são melhores

    if (ap_password.empty()) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    } else {
        strncpy((char*)ap_config.ap.password, ap_password.c_str(), sizeof(ap_config.ap.password) - 1);
        ap_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "AP '%s' criado. Acesse http://192.168.4.1", ap_ssid.c_str());

    // Inicia servidor HTTP
    httpd_config_t server_cfg = HTTPD_DEFAULT_CONFIG();
    server_cfg.server_port = 80;
    if (httpd_start(&_server, &server_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao iniciar servidor HTTP");
        return;
    }

    httpd_uri_t get_uri = { .uri = "/", .method = HTTP_GET, .handler = http_get_handler, .user_ctx = nullptr };
    httpd_uri_t post_uri = { .uri = "/connect", .method = HTTP_POST, .handler = http_post_handler, .user_ctx = nullptr };
    httpd_register_uri_handler(_server, &get_uri);
    httpd_register_uri_handler(_server, &post_uri);

    ESP_LOGI(TAG, "Portal acessível em http://192.168.4.1");
}

void Network::_parar_portal() {
    if (_server) {
        httpd_stop(_server);
        _server = nullptr;
    }
    esp_wifi_stop();   
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "Portal encerrado.");
}

// ==============================
// Conexão (única interface pública)
// ==============================
bool Network::conectar(const std::string& ssid, const std::string& password) {
    // Se ambos os parâmetros estão vazios → modo portal
    if (ssid.empty() && password.empty()) {
        ESP_LOGI(TAG, "Modo portal: aguardando credenciais...");
        _iniciar_portal();  // usa valores padrão (AP "ESP32_Config", aberto)

        // Aguarda até receber as credenciais
        xSemaphoreTake(_credenciais_prontas, portMAX_DELAY);

        // Para o portal
        _parar_portal();

        // Agora _ssid e _password estão preenchidos (via http_post_handler)
        // Continua o fluxo normal de conexão
        // Nota: o ssid/password já estão em _ssid/_password
    } else {
        // Conexão direta: armazena os parâmetros
        _ssid = ssid;
        _password = password;
    }

    // Configura modo STA
    _tentativas = 0;
    xEventGroupClearBits(_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);

    wifi_config_t wifi_cfg = {};
    strncpy((char*)wifi_cfg.sta.ssid, _ssid.c_str(), sizeof(wifi_cfg.sta.ssid) - 1);
    wifi_cfg.sta.ssid[sizeof(wifi_cfg.sta.ssid) - 1] = '\0';

    strncpy((char*)wifi_cfg.sta.password, _password.c_str(), sizeof(wifi_cfg.sta.password) - 1);
    wifi_cfg.sta.password[sizeof(wifi_cfg.sta.password) - 1] = '\0';

    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    // Aguarda resultado
    EventBits_t bits = xEventGroupWaitBits(_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE, pdFALSE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Conectado a %s", _ssid.c_str());
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Falha ao conectar a %s", _ssid.c_str());
        return false;
    }
    ESP_LOGE(TAG, "Evento inesperado.");
    return false;
}

// ==============================
// Desconexão
// ==============================
void Network::desconectar() {
    if (_conectado) {
        esp_wifi_disconnect();
        esp_wifi_stop();
        _conectado = false;
        xEventGroupClearBits(_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
        ESP_LOGI(TAG, "Wi-Fi desligado.");
    }
}

// ==============================
// HTTP POST
// ==============================
int Network::enviarDadosPOST(const std::string& url, const std::string& json_payload) {
    if (!_conectado) {
        ESP_LOGW(TAG, "Sem conexão, envio cancelado.");
        return -1;
    }

    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.method = HTTP_METHOD_POST;
    config.timeout_ms = 5000;

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        ESP_LOGE(TAG, "Falha ao criar cliente HTTP.");
        return -1;
    }

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, json_payload.c_str(), json_payload.length());

    int status = -1;
    if (esp_http_client_perform(client) == ESP_OK) {
        status = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "POST OK, status: %d", status);
    } else {
        ESP_LOGE(TAG, "Falha no POST");
    }

    esp_http_client_cleanup(client);
    return status;
}