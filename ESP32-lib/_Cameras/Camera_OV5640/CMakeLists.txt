idf_component_register(
    SRCS "Camera_OV5640.cpp"
    INCLUDE_DIRS "include"
    REQUIRES GPIO PWM I2C Tempo esp32-camera
)