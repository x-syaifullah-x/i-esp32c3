#pragma once

#include <stdint.h>

#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_netif_ip_addr.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t api_wifi_ap_register_uri(httpd_handle_t handle);

#ifdef __cplusplus
}
#endif