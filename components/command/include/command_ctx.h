#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void* data;
} command_ctx_in_t;

typedef struct {
    void* data;
} command_ctx_out_t;

typedef struct {
    command_ctx_in_t in;
    command_ctx_out_t out;
} command_ctx_t;

#ifdef __cplusplus
}
#endif