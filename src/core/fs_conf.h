/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_CONF_H_
#define _FOLK_SONG_CONF_H_

#include "fs_str.h"
#include "fs_file.h"
#include "fs_arr.h"
#include "fs_queue.h"

#define FS_CONF_FILE_DONE   -10001
#define FS_CONF_ERROR       -10002

#define FS_CONF_BLOCK_START 1
#define FS_CONF_BLOCK_END   2
#define FS_CONF_OK          0

#define FS_PARSE_FILE       0
#define FS_PARSE_BLOCK      1
#define FS_PARSE_PARAM      2

typedef struct fs_conf_s fs_conf_t;
struct fs_conf_s {
    fs_arr_t    *tokens;
    fs_file_t   *file;

    fs_arr_t    *ctx;

    fs_queue_t  st_mod;

    fs_pool_t   pool;
};

int fs_conf_parse_cmdline(fs_conf_t *conf, fs_str_t *cmdline);

int fs_conf_parse(fs_conf_t *conf, fs_str_t *filename);

#endif
