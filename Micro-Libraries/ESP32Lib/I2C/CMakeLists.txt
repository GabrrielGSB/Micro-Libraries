idf_component_register(
    SRCS "I2C.cpp"
    INCLUDE_DIRS "include"
    REQUIRES GPIO esp_driver_i2c driver log
)