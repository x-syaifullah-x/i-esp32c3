#pragma once

#include "command.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COMMAND_STATUS_WIFI_AP_ERR_EMPTY,
    COMMAND_STATUS_WIFI_AP_ERR_FULL,
} wifi_ap_err_t;

typedef enum {
    WIFI_AP_ACTION_STATUS = 0,
    WIFI_AP_ACTION_GET_IP,
    WIFI_AP_ACTION_STACONNECTED,
    WIFI_AP_ACTION_STADISCONNECTED,
    WIFI_AP_ACTION_ASSIGNED_IP_TO_CLIENT,
    WIFI_AP_ACTION_COUNT
} wifi_ap_action_t;

#ifdef __cplusplus
}
#endif