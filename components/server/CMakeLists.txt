idf_component_register(
    SRCS
        src/server.c
        src/server/response.c
        src/server/ws/ws.c
        src/server/api/wifi/ap.c
        src/server/api/wifi/sta.c
    
    INCLUDE_DIRS
        src/include
    
    PRIV_REQUIRES
    
    REQUIRES
        command
        esp_http_server
        esp_wifi
)