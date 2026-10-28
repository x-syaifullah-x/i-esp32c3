#pragma once

#include <stdint.h>

#include "command.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    WIFI_STA_ACTION_STATUS = 0,
    WIFI_STA_ACTION_SCAN,
    WIFI_STA_ACTION_CONNECT,
    WIFI_STA_ACTION_DISCONNECT,
    WIFI_STA_ACTION_COUNT,
} wifi_sta_action_t;

typedef struct {
    const char* const ssid;
    const char* const password;
    const uint32_t authmode;
} wifi_ap_config__t;

#ifdef __cplusplus
}
#endif