#include "server/response.h"

#include <string.h>

#include "esp_log.h"
#include "server/ws/ws.h"

// static const char* const TAG = "response";

cJSON* response_event_create(const command_type_t type, const double action, int status) {
    cJSON* root = cJSON_CreateObject();
    if (!root)
        return NULL;

    if (type < 0 || type > COMMAND_TYPE_COUNT)
        cJSON_AddNullToObject(root, "type");
    else
        cJSON_AddNumberToObject(root, "type", type);

    cJSON_AddNumberToObject(root, "action", action);

    cJSON_AddNumberToObject(root, "status", status);

    return root;
}

esp_err_t response_event_send(httpd_handle_t handle, cJSON* root) {
    char* json = cJSON_PrintUnformatted(root);
    if (json == NULL) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }
    ws_broadcast(handle, (uint8_t*)json, strlen(json), HTTPD_WS_TYPE_TEXT);

    cJSON_free(json);
    cJSON_Delete(root);

    return ESP_OK;
}

cJSON* response_http_create(const char* const type, const char* const action, int status) {
    cJSON* root = cJSON_CreateObject();
    if (!root)
        return NULL;

    if (type)
        cJSON_AddStringToObject(root, "type", type);
    else
        cJSON_AddNullToObject(root, "type");

    if (action)
        cJSON_AddStringToObject(root, "action", action);
    else
        cJSON_AddNullToObject(root, "action");

    cJSON_AddNumberToObject(root, "status", status);

    return root;
}

esp_err_t response_http_send(httpd_req_t* req, const char* status_code, cJSON* root) {
    if (root == NULL) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char* json = cJSON_PrintUnformatted(root);
    if (!json) {
        cJSON_Delete(root);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_status(req, status_code);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    esp_err_t err = httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);

    cJSON_free(json);
    cJSON_Delete(root);

    return err;
}

esp_err_t response_http_send_error(httpd_req_t* req, const char* err_code, const char* const type, const char* const action, int status, const char* const message) {
    cJSON* root = response_http_create(type, action, status);
    if (!root) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    if (message)
        cJSON_AddStringToObject(root, "message", message);

    return response_http_send(req, err_code, root);
}