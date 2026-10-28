#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COMMAND_TYPE_AUTH,
    COMMAND_TYPE_WIFI_AP,
    COMMAND_TYPE_WIFI_STA,
    COMMAND_TYPE_COUNT,
} command_type_t;

#ifdef __cplusplus
}
#endif