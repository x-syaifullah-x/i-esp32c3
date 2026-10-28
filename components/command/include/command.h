#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "command_action.h"
#include "command_ctx.h"
#include "command_register_res.h"
#include "command_res.h"
#include "command_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef command_res_t (*command_handler_t)(command_ctx_t* ctx);

typedef struct {
    bool registered;
    const command_handler_t* handlers;
    size_t count;
} command_t;

command_register_res_t command_register(command_t command, command_type_t type);

command_res_t command_execute(command_type_t type, command_action_t action, command_ctx_t* ctx);

#ifdef __cplusplus
}
#endif