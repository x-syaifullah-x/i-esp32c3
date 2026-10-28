#pragma once

#include <stdint.h>

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

httpd_handle_t server_start(const uint16_t port);

#ifdef __cplusplus
}
#endif