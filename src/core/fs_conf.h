/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_CONF_H_
#define _FOLK_SONG_CONF_H_

#include "fs_core.h"
#include "fs_run.h"
#include "fs_mod.h"
#include "fs_pool.h"

#define FS_CONF_FILE_DONE   -10001
#define FS_CONF_ERROR       -10002

#define FS_CONF_BLOCK_START 1
#define FS_CONF_BLOCK_END   2
#define FS_CONF_OK          0
#define FS_CONF_PASS        -10003

#define FS_PARSE_FILE       0
#define FS_PARSE_BLOCK      1
#define FS_PARSE_PARAM      2

struct fs_conf_s {
    fs_arr_t    *tokens;
    fs_file_t   *file;

    fs_run_t    *run;

    fs_pool_t   pool;
};

int fs_conf_parse_cmdline(fs_conf_t *conf, fs_str_t *cmdline);

int fs_conf_parse(fs_conf_t *conf, fs_str_t *filename);

#define fs_conf(conf, cmdline)                  \
    ({                                          \
        fs_gmod_init();                         \
        fs_conf_parse_cmdline(conf, cmdline);   \
        fs_gmod_inited((conf)->run);            \
     })

#define fs_conf_file(conf, cmdline)             \
    ({                                          \
        fs_gmod_init();                         \
        fs_conf_parse(conf, cmdline);           \
        fs_gmod_inited((conf)->run);            \
     })

#endif
