#pragma once

#include <stdint.h>

#include "esp_http_server.h"
#include "sdkconfig.h"

#ifndef CONFIG_HTTPD_WS_SUPPORT
#error "CONFIG_HTTPD_WS_SUPPORT is not enabled. Enable it via 'idf.py menuconfig' (Component config -> HTTP Server -> WebSocket Support) or add CONFIG_HTTPD_WS_SUPPORT=y to sdkconfig.defaults"
#endif

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t ws_register_uri(httpd_handle_t handle);

void ws_broadcast(const httpd_handle_t handle, const uint8_t* const data, size_t len, httpd_ws_type_t type);

#ifdef __cplusplus
}
#endif