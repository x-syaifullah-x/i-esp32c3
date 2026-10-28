idf_component_register(
    SRCS
        src/main.c

    INCLUDE_DIRS
    
    PRIV_REQUIRES
        nvs_flash
        wifi
        server
        esp_driver_gpio
        esp_driver_uart
        esp_timer
)

add_compile_options(
    -O3
    -ffunction-sections
    -fdata-sections
)