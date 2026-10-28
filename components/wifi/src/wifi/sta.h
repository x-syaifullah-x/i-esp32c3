#pragma once

#include "command/wifi/sta.h"

#ifdef __cplusplus
extern "C" {
#endif

void wifi__sta_init(void);

command_res_t wifi__sta_scan(command_ctx_t* ctx);

command_res_t wifi__sta_connect(command_ctx_t* ctx);

command_res_t wifi__sta_disconnect(command_ctx_t* ctx);

command_res_t wifi__sta_status(command_ctx_t* ctx);

#ifdef __cplusplus
}
#endif