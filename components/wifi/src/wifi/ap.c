#include "wifi/ap.h"

#include <string.h>

#include "cJSON.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_wifi_default.h"
#include "esp_wifi_remote.h"
#include "esp_wifi_types_generic.h"

// static const char* const TAG = "wifi_ap";

static wifi_ap_t wifi_ap = {0};

static inline uint32_t find_client_index_by_mac(const uint8_t* mac) {
    for (uint32_t i = 0; i < wifi_ap.clients.count; i++) {
        if (!memcmp(wifi_ap.clients.list[i].mac, mac, sizeof(wifi_ap.clients.list[i].mac)))
            return i;
    }
    return UINT32_MAX;
}

static inline uint8_t hex(char c) {
    return (c <= '9') ? c - '0' : (c | 0x20) - 'a' + 10;
}

static inline void mac_from_str(const char* s, uint8_t mac[6]) {
    for (int i = 0; i < 6; i++)
        mac[i] = (hex(s[i * 3]) << 4) | hex(s[i * 3 + 1]);
}

command_res_t wifi_ap_assigned_ip_to_client(command_ctx_t* ctx) {
    if (wifi_ap.clients.count == 0)
        return (command_res_t){.status = COMMAND_STATUS_WIFI_AP_ERR_EMPTY, .reply = COMMAND_REPLY_SELF};

    cJSON* payload = ctx->in.data;
    const char* const ip_str = cJSON_GetObjectItem(payload, "ip")->valuestring;
    const char* const mac_str = cJSON_GetObjectItem(payload, "mac")->valuestring;
    const char* const hostname = cJSON_GetObjectItem(payload, "hostname")->valuestring;

    uint8_t mac[6];
    mac_from_str(mac_str, mac);
    const uint32_t index = find_client_index_by_mac(mac);
    if (index == UINT32_MAX)
        return (command_res_t){.status = COMMAND_STATUS_INVALID_CTX, .reply = COMMAND_REPLY_SELF};

    wifi_ap.clients.list[index].hostname = strdup(hostname);

    wifi_ap.clients.list[index].ip.addr = esp_ip4addr_aton(ip_str);

    return (command_res_t){.status = COMMAND_STATUS_OK, .reply = COMMAND_REPLY_BROADCAST};
}

command_res_t wifi_ap_sta_disconnected(command_ctx_t* ctx) {
    if (!ctx)
        return (command_res_t){.status = COMMAND_STATUS_INVALID_CTX, .reply = COMMAND_REPLY_NONE};

    const uint32_t index = find_client_index_by_mac(ctx->in.data);
    if (index == UINT32_MAX)
        return (command_res_t){.status = COMMAND_STATUS_INVALID_CTX, .reply = COMMAND_REPLY_NONE};

    wifi_ap.clients.list[index] = wifi_ap.clients.list[--wifi_ap.clients.count];

    return (command_res_t){.status = COMMAND_STATUS_OK, .reply = COMMAND_REPLY_BROADCAST};
}

command_res_t wifi_ap_sta_connected(command_ctx_t* ctx) {
    if (wifi_ap.clients.count >= sizeof(wifi_ap.clients) / sizeof(wifi_ap.clients.list[0]))
        return (command_res_t){.status = COMMAND_STATUS_WIFI_AP_ERR_FULL, .reply = COMMAND_REPLY_SELF};

    memcpy(wifi_ap.clients.list[wifi_ap.clients.count++].mac, ctx->in.data, 6);

    return (command_res_t){.status = COMMAND_STATUS_OK, .reply = COMMAND_REPLY_BROADCAST};
}

command_res_t wifi_ap_get_ip(command_ctx_t* ctx) {
    esp_ip4addr_ntoa(&wifi_ap.ap.ip, ctx->in.data, 16);
    return (command_res_t){.status = COMMAND_STATUS_OK, .reply = COMMAND_REPLY_SELF};
}

command_res_t wifi_ap_status(command_ctx_t* ctx) {
    cJSON* root = ctx->out.data;

    if (!root)
        return (command_res_t){.status = COMMAND_STATUS_INVALID_CTX, .reply = COMMAND_REPLY_SELF};

    cJSON* root_data = cJSON_CreateObject();

    char ip_str[16], netmask_str[16], gateway_str[16];
    esp_ip4addr_ntoa(&wifi_ap.ap.ip, ip_str, sizeof(ip_str));
    esp_ip4addr_ntoa(&wifi_ap.ap.netmask, netmask_str, sizeof(netmask_str));
    esp_ip4addr_ntoa(&wifi_ap.ap.gateway, gateway_str, sizeof(gateway_str));

    cJSON_AddStringToObject(root_data, "ssid", wifi_ap.ap.ssid);
    cJSON_AddStringToObject(root_data, "password", wifi_ap.ap.password);
    cJSON_AddStringToObject(root_data, "ip_address", ip_str);
    cJSON_AddStringToObject(root_data, "netmask", netmask_str);
    cJSON_AddStringToObject(root_data, "gateway", gateway_str);

    cJSON* root_clients = cJSON_AddArrayToObject(root_data, "clients");
    for (size_t i = 0; i < wifi_ap.clients.count; i++) {
        const wifi_ap_client_t* client = &wifi_ap.clients.list[i];
        cJSON* data_client = cJSON_CreateObject();
        char mac[18];
        snprintf(mac, sizeof(mac),
                 "%02X:%02X:%02X:%02X:%02X:%02X",
                 client->mac[0], client->mac[1], client->mac[2], client->mac[3], client->mac[4], client->mac[5]);
        cJSON_AddStringToObject(data_client, "mac", mac);

        const esp_ip4_addr_t ap_client_ip = client->ip;
        esp_ip4addr_ntoa(&ap_client_ip, ip_str, sizeof(ip_str));
        cJSON_AddStringToObject(data_client, "ip_address", ip_str);

        if (client->hostname)
            cJSON_AddStringToObject(data_client, "hostname", client->hostname);
        else
            cJSON_AddNullToObject(data_client, "hostname");

        cJSON_AddItemToArray(root_clients, data_client);
    }

    cJSON_AddNumberToObject(root, "type", COMMAND_TYPE_WIFI_AP);
    cJSON_AddNumberToObject(root, "action", WIFI_AP_ACTION_STATUS);
    cJSON_AddNumberToObject(root, "status", COMMAND_STATUS_OK);
    cJSON_AddItemToObject(root, "data", root_data);

    return (command_res_t){.status = COMMAND_STATUS_OK, .reply = COMMAND_REPLY_SELF};
}

static const command_handler_t handler[WIFI_AP_ACTION_COUNT] = {
    wifi_ap_status,
    wifi_ap_get_ip,
    wifi_ap_sta_connected,
    wifi_ap_sta_disconnected,
    wifi_ap_assigned_ip_to_client,
};

esp_netif_t* wifi_ap_init(void) {
    esp_netif_t* esp_netif_ap = esp_netif_create_default_wifi_ap();
    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = CONFIG_ESP_WIFI_AP_SSID,
            .ssid_len = strlen(CONFIG_ESP_WIFI_AP_SSID),
            .channel = CONFIG_ESP_WIFI_AP_CHANNEL,
            .password = CONFIG_ESP_WIFI_AP_PASSWORD,
            .max_connection = CONFIG_ESP_MAX_STA_CONN_AP,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {.required = false},
        },
    };
    if (sizeof(CONFIG_ESP_WIFI_AP_PASSWORD) == 0)
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    esp_netif_dns_info_t dns;
    dns.ip.type = ESP_IPADDR_TYPE_V4;
    dns.ip.u_addr.ip4.addr = ESP_IP4TOADDR(8, 8, 8, 8);

    uint8_t dhcps_offer_option = 0x02;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(esp_netif_ap));
    ESP_ERROR_CHECK(esp_netif_dhcps_option(esp_netif_ap, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_offer_option, sizeof(dhcps_offer_option)));
    ESP_ERROR_CHECK(esp_netif_set_dns_info(esp_netif_ap, ESP_NETIF_DNS_MAIN, &dns));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(esp_netif_ap));

    esp_netif_ip_info_t esp_netif_ip_info;
    esp_netif_get_ip_info(esp_netif_ap, &esp_netif_ip_info);

    wifi_ap.ap.ssid = CONFIG_ESP_WIFI_AP_SSID;
    wifi_ap.ap.password = CONFIG_ESP_WIFI_AP_PASSWORD;
    wifi_ap.ap.ip = esp_netif_ip_info.ip;
    wifi_ap.ap.netmask = esp_netif_ip_info.netmask;
    wifi_ap.ap.gateway = esp_netif_ip_info.gw;

    command_register((command_t){.handlers = handler, .count = sizeof(handler)}, COMMAND_TYPE_WIFI_AP);

    return esp_netif_ap;
}