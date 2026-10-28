#pragma once

#include "command.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    AUTH_OK = 0,
} auth_err_t;

typedef enum {
    AUTH_ACTION_LOGIN,
} auth_action_t;

// void command_wifi_ap_register_handler(wifi_ap_action_t action, command_handler_t fn);

#ifdef __cplusplus
}
#endif