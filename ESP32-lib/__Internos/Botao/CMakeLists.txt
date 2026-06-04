idf_component_register(
    SRCS "Botao.cpp"
    INCLUDE_DIRS "include"
    REQUIRES 
        GPIO  
        Tempo
        freertos
)