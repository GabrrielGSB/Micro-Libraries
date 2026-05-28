idf_component_register(
    SRCS "PWM.cpp"
    INCLUDE_DIRS "include"
    REQUIRES GPIO esp_driver_ledc log
)