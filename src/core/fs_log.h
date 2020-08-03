/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_LOG_H_
#define _FOLK_SONG_LOG_H_

#include <stdio.h>

typedef struct fs_log_s fs_log_t;
struct fs_log_s {

};

#define fs_log_info(log, fmt, ...)       \
    printf("[INFO] " fmt "\n", __VA_ARGS__)

#define fs_log_err(log, fmt, ...)        \
    printf("[ERROR]" fmt "\n", __VA_ARGS__)

#define fs_log_dbg(log, fmt, ...)        \
    printf("[DEBUG]" fmt "\n", __VA_ARGS__)

#endif
