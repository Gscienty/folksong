/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_MOD_H_
#define _FOLK_SONG_MOD_H_

#include "fs_cmd.h"
#include "fs_str.h"
#include <stddef.h>

#define FS_MOD_OK 0

typedef struct fs_mod_s fs_mod_t;
struct fs_mod_s {
    unsigned int version;
    fs_str_t name;

    fs_cmd_t commands[];
};

extern fs_mod_t *global_mods[];

int fs_mod_init();

#define fs_gmod_is_end(i)       \
    (global_mods[i] == NULL)

#define fs_gmod_cmd(i)          \
    (global_mods[i]->commands)

#define fs_mod(_version, _name, ...)            \
    fs_mod_t _name = {                          \
        .version = _version,                    \
        .name = fs_str(#_name),                 \
        .commands = {                           \
            __VA_ARGS__                         \
            ,fs_cmd_end                         \
        }                                       \
    }

#endif
