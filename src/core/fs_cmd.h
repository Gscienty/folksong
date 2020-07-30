/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_CMD_H_
#define _FOLK_SONG_CMD_H_

#include "fs_core.h"
#include "fs_str.h"
#include <stdbool.h>

struct fs_cmd_s {
    fs_str_t    token;
    int         (*call) (fs_run_t *run, void *ctx);
    bool        block:1;
    bool        child:1;
};

#define fs_cmd_end                  \
    {                               \
        .token  = fs_null_str,      \
        .call   = NULL,             \
        .block  = false,            \
        .child  = false,            \
    }

#define fs_cmd_token(cmd)               \
    (&(cmd)->token)

#define fs_cmd_is_end(cmd)              \
    (fs_str_empty(fs_cmd_token(cmd)))

#define fs_cmd_call(cmd)                \
    ((cmd)->call)

#define fs_cmd_is_block(cmd)            \
    ((cmd)->block)

#define fs_is_child(cmd)                \
    ((cmd)->child)

#define fs_cmd(_token, _call, _block, _child)       \
    {                                               \
        .token  = fs_str(_token),                   \
        .call   = _call,                            \
        .block  = _block,                           \
        .child  = _child                            \
    }

#endif
