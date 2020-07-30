/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_BUF_H_
#define _FOLK_SONG_BUF_H_

#include "fs_core.h"
#include <sys/types.h>
#include <stdbool.h>

#define FS_BUF_ERROR    -1
#define FS_BUF_OK       0

struct fs_buf_s {
    size_t  buf_len;
    void    *buf;
    void    *pos;
    void    *last;

    bool    temp:1;
};

#define fs_buf_size(b)                                      \
    ((size_t) ((b)->last - (b)->pos))

#define fs_buf_capacity(b)                                  \
    ((b)->buf_len)

#define fs_buf_overflow(b)                                  \
    ((b)->pos >= (b)->last)

#define fs_alloc_buf(pool)                                  \
    ((fs_buf_t *) fs_pool_alloc(pool, sizeof(fs_buf_t)))

#define fs_buf_pos(b)                                       \
    ((b)->pos)

#define fs_buf_lshift(type, b)                              \
    ({                                                      \
        type _ret = *(type *) (b)->pos;                     \
        (b)->pos += sizeof(type);                           \
        _ret;                                               \
     })

fs_buf_t *fs_create_temp_buf(fs_pool_t *pool, size_t size);

int fs_buf_alloc(fs_buf_t *buf, size_t size);
int fs_buf_release(fs_buf_t *buf);

#endif
