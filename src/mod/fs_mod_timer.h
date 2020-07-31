/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_core.h"
#include <uv.h>
#include <sys/types.h>
#include <stdbool.h>

typedef struct fs_mod_timer_s fs_mod_timer_t;
struct fs_mod_timer_s {
    uv_timer_t  handler;

    uint64_t    timeout;
    uint64_t    repeat;

    struct {
        void    *ctx;
        int     (*call) (void *ctx);
    } cb;

    bool        timeout_flag:1;
    bool        repeat_flag:1;
    bool        repeat_always:1;
    bool        cb_flag:1;
};
