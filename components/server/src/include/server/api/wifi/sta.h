#pragma once

#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t api_wifi_sta_register_uri(httpd_handle_t handle);

#ifdef __cplusplus
}
#endif