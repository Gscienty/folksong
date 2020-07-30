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
#include <stddef.h>

typedef struct fs_mod_method_s fs_mod_method_t;
struct fs_mod_method_s {
    int (*init_mod) (fs_conf_t *conf, fs_cmd_t *cmd, void **ctx);
    int (*init_mod_completed) (fs_run_t *run, void *ctx);
};

struct fs_mod_s {
    unsigned int    version;
    fs_str_t        name;

    fs_mod_method_t *methods;
    fs_cmd_t        commands[];
};

extern fs_mod_t *global_mods[];

int fs_mod_init();

#define fs_mod_init_mod(mod)                    \
    ((mod)->methods->init_mod)

#define fs_mod_init_mod_completed(mod)          \
    ((mod)->methods->init_mod_completed)

#define fs_gmod_nth(i)                          \
    (global_mods[i])

#define fs_gmod_is_end(i)                       \
    (fs_gmod_nth(i) == NULL)

#define fs_gmod_nth_init_mod(i)                 \
    (fs_mod_init_mod(fs_gmod_nth(i)))

#define fs_gmod_nth_init_mod_completed(i)       \
    (fs_mod_init_mod_completed(fs_gmod_nth(i))) \

#define fs_gmod_cmd(i)                          \
    (global_mods[i]->commands)

#define fs_mod(_version, _name, _methods, ...)  \
    fs_mod_t _name = {                          \
        .version = _version,                    \
        .name = fs_str(#_name),                 \
        .methods = _methods,                    \
        .commands = {                           \
            __VA_ARGS__                         \
            ,fs_cmd_end                         \
        }                                       \
    }

#endif
