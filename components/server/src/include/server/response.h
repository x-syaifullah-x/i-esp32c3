#pragma once

#include "cJSON.h"
#include "command.h"
#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

cJSON* response_event_create(const command_type_t type, const double action, int status);

esp_err_t response_event_send(httpd_handle_t handle, cJSON* root);

cJSON* response_http_create(const char* const type, const char* const action, int status);

esp_err_t response_http_send(httpd_req_t* req, const char* status_code, cJSON* root);

esp_err_t response_http_send_error(httpd_req_t* req, const char* err_code, const char* const type, const char* const action, int status, const char* const message);

#ifdef __cplusplus
}
#endif