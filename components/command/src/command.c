#include "command.h"

#include "esp_log.h"

static command_t commands[COMMAND_TYPE_COUNT];

command_register_res_t command_register(command_t command, command_type_t type) {
    if (type < 0 || type >= COMMAND_TYPE_COUNT) {
        return COMMAND_REGISTER_RES_INVALID_TYPE;
    }

    if (commands[type].registered) {
        return COMMAND_REGISTER_RES_ALREADY_REGISTERED;
    }

    commands[type] = command;
    commands[type].registered = true;

    return COMMAND_REGISTER_RES_OK;
}

command_res_t command_execute(command_type_t type, command_action_t action, command_ctx_t* ctx) {
    if (!commands[type].registered) {
        return (command_res_t){
            .status = COMMAND_STATUS_INVALID_TYPE,
            .reply = COMMAND_REPLY_SELF,
        };
    }

    const command_handler_t command_handler = commands[type].handlers[action];
    if (!command_handler) {
        return (command_res_t){
            .status = COMMAND_STATUS_INVALID_ACTION,
            .reply = COMMAND_REPLY_SELF,
        };
    }

    return command_handler(ctx);
}