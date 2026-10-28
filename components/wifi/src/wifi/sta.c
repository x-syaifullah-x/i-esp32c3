#include "wifi/sta.h"

#include <string.h>

#include "cJSON.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "sdkconfig.h"

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#else
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#endif

// static const char* const TAG = "wifi_sta";

command_res_t wifi__sta_scan(command_ctx_t* ctx) {
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
    };
    return (command_res_t){.status = esp_wifi_scan_start(&scan_config, false), .reply = COMMAND_REPLY_NONE};
}

command_res_t wifi__sta_connect(command_ctx_t* ctx) {
    // wifi_ap_config__t* c = ctx->in.data;
    cJSON* p = ctx->in.data;
    // esp_wifi_disconnect();
    // wifi_config_t wifi_sta_config = {0};
    // strncpy((char*)wifi_sta_config.sta.ssid, c->ssid, sizeof(wifi_sta_config.sta.ssid) - 1);
    // strncpy((char*)wifi_sta_config.sta.password, c->password, sizeof(wifi_sta_config.sta.password) - 1);
    // wifi_sta_config.sta.threshold.authmode = c->authmode;
    // if (wifi_sta_config.sta.threshold.authmode == WIFI_AUTH_WPA3_PSK || wifi_sta_config.sta.threshold.authmode == WIFI_AUTH_WPA2_WPA3_PSK)
    //     wifi_sta_config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    // ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));
    return (command_res_t){.status = esp_wifi_connect(), .reply = COMMAND_REPLY_NONE};
}

command_res_t wifi__sta_disconnect(command_ctx_t* ctx) {
    int err = esp_wifi_disconnect();
    cJSON* c = ctx->in.data;
    cJSON_AddStringToObject(c, "type", "wifi_sta");
    cJSON_AddStringToObject(c, "action", "disconnect");
    cJSON_AddNumberToObject(c, "status", err);
    cJSON* data = cJSON_CreateObject();
    // cJSON_AddStringToObject(data, "ssid", "");
    // cJSON_AddStringToObject(data, "message", "disconnecting");
    cJSON_AddItemToObject(c, "data", data);

    return (command_res_t){.status = err, .reply = COMMAND_REPLY_BROADCAST};
}

command_res_t wifi__sta_status(command_ctx_t* ctx) {
    cJSON* root = ctx->out.data;

    if (!root)
        return (command_res_t){.status = COMMAND_STATUS_INVALID_CTX, .reply = COMMAND_REPLY_SELF};

    cJSON* root_data = cJSON_CreateObject();

    wifi_ap_record_t ap_record;
    esp_wifi_sta_get_ap_info(&ap_record);
    esp_netif_t* sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    esp_netif_ip_info_t esp_netif_ip_info;
    esp_netif_get_ip_info(sta_netif, &esp_netif_ip_info);

    if (ap_record.ssid[0] == '\0') {
        cJSON_AddNullToObject(root_data, "ssid");
    } else {
        cJSON_AddStringToObject(root_data, "ssid", (char*)ap_record.ssid);
    }

    if (!esp_netif_ip_info.ip.addr) {
        cJSON_Delete(root_data);
        root_data = NULL;
    } else {
        char ip_str[16];
        esp_ip4addr_ntoa(&esp_netif_ip_info.ip, ip_str, sizeof(ip_str));
        cJSON_AddStringToObject(root_data, "ip_address", ip_str);
    }

    cJSON_AddNumberToObject(root, "type", COMMAND_TYPE_WIFI_STA);
    cJSON_AddNumberToObject(root, "action", WIFI_STA_ACTION_STATUS);
    cJSON_AddNumberToObject(root, "status", COMMAND_STATUS_OK);
    if (root_data) {
        cJSON_AddItemToObject(root, "data", root_data);
    } else {
        cJSON_AddNullToObject(root, "data");
    }

    return (command_res_t){.status = COMMAND_STATUS_OK, .reply = COMMAND_REPLY_SELF};
}

static command_handler_t handler[WIFI_STA_ACTION_COUNT] = {
    [WIFI_STA_ACTION_SCAN] = wifi__sta_scan,
    [WIFI_STA_ACTION_CONNECT] = wifi__sta_connect,
    [WIFI_STA_ACTION_DISCONNECT] = wifi__sta_disconnect,
    [WIFI_STA_ACTION_STATUS] = wifi__sta_status,
};

void wifi__sta_init(void) {
    esp_netif_create_default_wifi_sta();
    wifi_config_t wifi_sta_config = {
        .sta = {
            .ssid = CONFIG_ESP_WIFI_REMOTE_AP_SSID,
            .password = CONFIG_ESP_WIFI_REMOTE_AP_PASSWORD,
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config));

    command_register((command_t){.handlers = handler, .count = sizeof(handler)}, COMMAND_TYPE_WIFI_STA);
}