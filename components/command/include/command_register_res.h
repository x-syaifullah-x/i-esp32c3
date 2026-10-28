#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COMMAND_REGISTER_RES_OK,
    COMMAND_REGISTER_RES_INVALID_TYPE,
    COMMAND_REGISTER_RES_ALREADY_REGISTERED,
} command_register_res_t;

#ifdef __cplusplus
}
#endif