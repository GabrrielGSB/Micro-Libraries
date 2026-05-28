idf_component_register(
    SRCS "SSD1306.cpp" "font.h"
    INCLUDE_DIRS "include"
    REQUIRES I2C log
)