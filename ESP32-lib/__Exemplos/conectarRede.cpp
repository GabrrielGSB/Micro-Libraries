#include "esp_log.h"
#include "Network.hpp"
#include "Tempo.hpp"


extern "C" void app_main() {

    Network wifi; delay_ms(2000);

    wifi.conectar("ggsb", "21022002");

    while (true) {
        delay_ms(100);
    }
}