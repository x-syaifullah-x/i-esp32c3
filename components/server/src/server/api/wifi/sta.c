#include "server/api/wifi/sta.h"

#include "cJSON.h"
#include "command/wifi/sta.h"
#include "server/response.h"
#include "server/ws/ws.h"

// static const char* const TAG = "server_api_wifi_sta";

static esp_err_t server_api_wifi_sta_handler(httpd_req_t* req);

static const httpd_uri_t uri_scan = {
    .uri = "/api/wifi/sta/scan",
    .method = HTTP_POST,
    .handler = server_api_wifi_sta_handler,
    .user_ctx = NULL,
};

static const httpd_uri_t uri_connect = {
    .uri = "/api/wifi/sta/connect",
    .method = HTTP_POST,
    .handler = server_api_wifi_sta_handler,
    .user_ctx = NULL,
};

static const httpd_uri_t uri_disconnect = {
    .uri = "/api/wifi/sta/disconnect",
    .method = HTTP_POST,
    .handler = server_api_wifi_sta_handler,
    .user_ctx = NULL,
};

static const httpd_uri_t uri_status = {
    .uri = "/api/wifi/sta/status",
    .method = HTTP_POST,
    .handler = server_api_wifi_sta_handler,
    .user_ctx = NULL,
};

static const httpd_uri_t uris[] = {
    uri_scan,
    uri_connect,
    uri_disconnect,
    uri_status,
};

static inline esp_err_t server_api_wifi_sta_handler_scan(httpd_req_t* req) {
    int err = command_execute(COMMAND_TYPE_WIFI_STA, WIFI_STA_ACTION_SCAN, NULL).status;

    cJSON* root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "type", "wifi_sta");
    cJSON_AddStringToObject(root, "action", "scan");
    cJSON_AddNumberToObject(root, "status", err);
    cJSON* data = cJSON_CreateObject();
    cJSON_AddItemToObject(root, "data", data);
    char* json = cJSON_PrintUnformatted(root);
    if (json == NULL) {
        cJSON_Delete(root);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    response_http_send(req, "200 Ok", root);

    return ESP_OK;
}

static esp_err_t server_api_wifi_sta_handler(httpd_req_t* req) {
    if (strcmp(req->uri, uri_scan.uri) == 0)
        return server_api_wifi_sta_handler_scan(req);

    if (strcmp(req->uri, uri_connect.uri) == 0) {
        char buf[256];
        if (req->content_len >= sizeof(buf)) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Request too large");
            return ESP_FAIL;
        }

        int received = httpd_req_recv(req, buf, req->content_len);
        if (received <= 0)
            return ESP_FAIL;

        cJSON* root = cJSON_Parse(buf);

        const cJSON* ssid = cJSON_GetObjectItem(root, "ssid");
        const cJSON* password = cJSON_GetObjectItem(root, "password");
        const cJSON* authmode = cJSON_GetObjectItem(root, "authmode");

        wifi_ap_config__t wifi_ap_config = {.ssid = ssid->valuestring, .password = password->valuestring, .authmode = authmode->valueint};
        command_ctx_t c = {.in.data = &wifi_ap_config};
        int err = command_execute(COMMAND_TYPE_WIFI_STA, WIFI_STA_ACTION_CONNECT, &c).status;

        cJSON_Delete(root);

        root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "type", "wifi_sta");
        cJSON_AddStringToObject(root, "action", "connect");
        cJSON_AddNumberToObject(root, "status", err);
        cJSON* data = cJSON_CreateObject();
        cJSON_AddStringToObject(data, "ssid", ssid->valuestring);
        cJSON_AddStringToObject(data, "message", "connecting");
        cJSON_AddItemToObject(root, "data", data);
        char* json = cJSON_PrintUnformatted(root);
        if (json == NULL) {
            cJSON_Delete(root);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        response_http_send(req, "200 Ok", root);

        return ESP_OK;
    }

    if (strcmp(req->uri, uri_disconnect.uri) == 0) {
        cJSON* root = cJSON_CreateObject();
        command_ctx_t c = {.in.data = root};
        int err = command_execute(COMMAND_TYPE_WIFI_STA, WIFI_STA_ACTION_DISCONNECT, &c).status;
        char* json = cJSON_PrintUnformatted(root);
        if (json == NULL) {
            cJSON_Delete(root);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        response_http_send(req, "200 Ok", root);

        return ESP_OK;
    }

    if (strcmp(req->uri, uri_status.uri) == 0) {
        cJSON* root = cJSON_CreateObject();
        command_ctx_t c = {.in.data = root};
        int err = command_execute(COMMAND_TYPE_WIFI_STA, WIFI_STA_ACTION_STATUS, &c).status;
        char* json = cJSON_PrintUnformatted(root);
        if (json == NULL) {
            cJSON_Delete(root);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        response_http_send(req, "200 Ok", root);

        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t api_wifi_sta_register_uri(httpd_handle_t handle) {
    for (size_t i = 0; i < sizeof(uris) / sizeof(uris[0]); i++) {
        esp_err_t r = httpd_register_uri_handler(handle, &uris[i]);
        if (r)
            return r;
    }

    return ESP_OK;
}