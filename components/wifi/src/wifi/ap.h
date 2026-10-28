#pragma once

#include "command/wifi/ap.h"
#include "esp_netif_types.h"
#include "sdkconfig.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t mac[6];
    esp_ip4_addr_t ip;
    const char* hostname;
} wifi_ap_client_t;

typedef struct {
    const char* ssid;
    const char* password;
    esp_ip4_addr_t ip;
    esp_ip4_addr_t netmask;
    esp_ip4_addr_t gateway;
} wifi_ap_info_t;

typedef struct {
    wifi_ap_info_t ap;
    struct {
        wifi_ap_client_t list[CONFIG_ESP_MAX_STA_CONN_AP];
        uint32_t count;
    } clients;
} wifi_ap_t;

command_res_t wifi_ap_status(command_ctx_t* ctx);

command_res_t wifi_ap_get_ip(command_ctx_t* ctx);

command_res_t wifi_ap_sta_connected(command_ctx_t* ctx);

command_res_t wifi_ap_sta_disconnected(command_ctx_t* ctx);

command_res_t wifi_ap_assigned_ip_to_client(command_ctx_t* ctx);

esp_netif_t* wifi_ap_init(void);

#ifdef __cplusplus
}
#endif