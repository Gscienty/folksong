/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_MOD_H_
#define _FOLK_SONG_MOD_H_

#include "fs_core.h"
#include "fs_str.h"
#include "fs_cmd.h"
#include "fs_queue.h"
#include <stddef.h>
#include <stdbool.h>

#define FS_MOD_OK 0

typedef struct fs_mod_method_s fs_mod_method_t;
struct fs_mod_method_s {
    int (*init_mod) (fs_conf_t *conf, fs_cmd_t *cmd, void **ctx);
    int (*init_mod_completed) (fs_run_t *run, void *ctx);

    int (*inited) (fs_run_t *run, fs_arr_t *ctxs);
};

struct fs_mod_s {
    unsigned int    version;
    fs_str_t        name;

    bool            used:1;
    fs_arr_t        *ctxs;

    fs_mod_method_t *methods;
    fs_cmd_t        commands[];
};

extern fs_mod_t *global_mods[];

int fs_gmod_init();

int fs_gmod_inited(fs_run_t *run);

#define fs_mod_init_mod(mod)                    \
    ((mod)->methods->init_mod)

#define fs_mod_init_mod_completed(mod)          \
    ((mod)->methods->init_mod_completed)

#define fs_mod_inited(mod)                      \
    ((mod)->methods->inited)

#define fs_mod_used(mod)                        \
    ((mod)->used)

#define fs_mod_ctxs(mod)                        \
    ((mod)->ctxs)

#define fs_gmod_nth(i)                          \
    (global_mods[i])

#define fs_gmod_is_end(i)                       \
    (fs_gmod_nth(i) == NULL)

#define fs_gmod_nth_init_mod(i)                 \
    (fs_mod_init_mod(fs_gmod_nth(i)))

#define fs_gmod_nth_init_mod_completed(i)       \
    (fs_mod_init_mod_completed(fs_gmod_nth(i))) \

#define fs_gmod_nth_inited(i)                   \
    (fs_mod_inited(fs_gmod_nth(i)))

#define fs_gmod_nth_used(i)                     \
    (fs_mod_used(fs_gmod_nth(i)))

#define fs_gmod_nth_ctxs(i)                     \
    (fs_mod_ctxs(fs_gmod_nth(i)))

#define fs_gmod_cmd(i)                          \
    (global_mods[i]->commands)

#define fs_mod_inited_nth_ctx(type, ctxs, i)    \
    ((type *) * fs_arr_nth(void *, ctxs, i))

#define fs_gmod_stored_ctx(i, ctx)                              \
    ({                                                          \
        void **_stored_ctx = fs_arr_push(fs_gmod_nth_ctxs(i));  \
        *_stored_ctx = ctx;                                     \
     })

#define fs_mod(_version, _name, _methods, ...)  \
    fs_mod_t _name = {                          \
        .version    = _version,                 \
        .name       = fs_str(#_name),           \
        .methods    = _methods,                 \
        .used       = false,                    \
        .ctxs       = NULL,                     \
        .commands   = {                         \
            __VA_ARGS__                         \
            ,fs_cmd_end                         \
        }                                       \
    }

#endif
