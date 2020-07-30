/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_buf.h"
#include "fs_pool.h"
#include <stddef.h>
#include <malloc.h>

fs_buf_t *fs_create_temp_buf(fs_pool_t *pool, size_t size) {
    fs_buf_t *buf = NULL;

    if ((buf = fs_alloc_buf(pool)) == NULL) {
        return NULL;
    }

    buf->buf = fs_pool_alloc(pool, size);
    buf->pos = buf->buf;
    buf->last = buf->buf + size;
    buf->temp = true;

    return buf;
}

int fs_buf_alloc(fs_buf_t *buf, size_t size) {

    buf->buf = malloc(size);
    buf->buf_len = size;
    buf->pos = buf->buf;
    buf->last = buf->pos + size;
    buf->temp = true;

    return FS_BUF_OK;
}

int fs_buf_release(fs_buf_t *buf) {
    free(buf->buf);

    return FS_BUF_OK;
}
