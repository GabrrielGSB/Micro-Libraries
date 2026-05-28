idf_component_register(
    SRCS "ADC.cpp"
    INCLUDE_DIRS "include"
    REQUIRES GPIO esp_driver_gpio esp_adc
)