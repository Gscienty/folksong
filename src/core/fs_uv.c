/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_uv.h"


int fs_uv_init(fs_uv_t *uv) {
    uv->loop = uv_loop_new();

    uv_loop_init(uv->loop);

    return FS_UV_OK;
}

int fs_uv_run(fs_uv_t *uv) {
    return uv_run(uv->loop, UV_RUN_DEFAULT);
}
