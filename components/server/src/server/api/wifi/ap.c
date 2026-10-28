#include "server/api/wifi/ap.h"

#include "command.h"
#include "command/wifi/ap.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_wifi_types_generic.h"
#include "server/response.h"

// static const char* const TAG = "server_api_wifi_ap";

static esp_err_t http_uri_handler(httpd_req_t* req);

static const httpd_uri_t uri_status = {
    .uri = "/api/wifi/ap",
    .method = HTTP_POST,
    .handler = http_uri_handler,
};

static esp_err_t http_uri_handler(httpd_req_t* req) {
    char buf[128];

    if (req->content_len >= sizeof(buf))
        return response_http_send_error(req, "500 Internal Server Error", NULL, NULL, ESP_FAIL, "Request too large");

    if (httpd_req_recv(req, buf, req->content_len) <= 0)
        return ESP_FAIL;

    cJSON* root = cJSON_Parse(buf);
    cJSON* action = cJSON_GetObjectItem(root, "action");

    if (!strcmp(action->valuestring, "status")) {
        cJSON* root = response_http_create("wifi_ap", "status", ESP_OK);
        if (!root)
            return httpd_resp_send_500(req);
        command_ctx_t c = {.in.data = root};
        command_execute(COMMAND_TYPE_WIFI_AP, WIFI_AP_ACTION_STATUS, &c);
        return response_http_send(req, "200 Ok", root);
    }

    return ESP_FAIL;
}

esp_err_t api_wifi_ap_register_uri(httpd_handle_t handle) {
    esp_err_t err;

    err = httpd_register_uri_handler(handle, &uri_status);
    if (err) return err;

    return ESP_OK;
}