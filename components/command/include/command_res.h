#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    COMMAND_REPLY_NONE,
    COMMAND_REPLY_SELF,
    COMMAND_REPLY_BROADCAST,
} command_reply_t;

typedef enum {
    COMMAND_STATUS_OK,
    COMMAND_STATUS_INVALID_TYPE,
    COMMAND_STATUS_INVALID_ACTION,
    COMMAND_STATUS_INVALID_CTX,
} command_status_t;

typedef struct {
    command_status_t status;
    command_reply_t reply;
} command_res_t;

#ifdef __cplusplus
}
#endif