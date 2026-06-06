idf_component_register(
    SRCS "Network.cpp"
    INCLUDE_DIRS "include"
    REQUIRES 
        esp_wifi 
        esp_event 
        esp_netif 
        esp_http_client
        esp_http_server
    PRIV_REQUIRES 
        nvs_flash
)