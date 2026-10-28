#include "server.h"

#include "esp_err.h"
#include "esp_log.h"
#include "server/api/wifi/ap.h"
#include "server/api/wifi/sta.h"
#include "server/ws/ws.h"

const char* const TAG = "server";

static esp_err_t server_options_handler(httpd_req_t* req) {
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

static const httpd_uri_t api_uri_option = {
    .uri = "*",
    .method = HTTP_OPTIONS,
    .handler = server_options_handler,
};

httpd_handle_t server_start(const uint16_t port) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.max_open_sockets = CONFIG_WS_MAX_CLIENTS + 2; /* +2 untuk margin HTTP biasa */
    config.uri_match_fn = httpd_uri_match_wildcard;

    httpd_handle_t handle = NULL;
    if (httpd_start(&handle, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start httpd");
        return NULL;
    }
    httpd_register_uri_handler(handle, &api_uri_option);

    ws_register_uri(handle);
    api_wifi_ap_register_uri(handle);
    api_wifi_sta_register_uri(handle);

    return handle;
}