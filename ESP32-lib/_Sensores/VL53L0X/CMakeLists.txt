idf_component_register(
    SRCS "VL53L0X.cpp"
    INCLUDE_DIRS "include"
    REQUIRES I2C 
             esp_driver_i2c 
             log
)