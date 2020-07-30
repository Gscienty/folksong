/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_UV_H_
#define _FOLK_SONG_UV_H_

#include "fs_core.h"
#include <uv.h>

#define FS_UV_OK    0

struct fs_uv_s {
    uv_loop_t *loop;
};

int fs_uv_init(fs_uv_t *uv);

int fs_uv_run(fs_uv_t *uv);

#endif
