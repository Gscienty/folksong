/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_ARR_H_
#define _FOLK_SONG_ARR_H_

#include "fs_core.h"
#include "fs_pool.h"
#include <stddef.h>

#define FS_ARR_ERROR    -1
#define FS_ARR_OK       0

struct fs_arr_s {
    void        *elems;
    int         ele_capa;
    int         ele_count;
    size_t      ele_size;
    fs_pool_t   *pool;
};

#define fs_arr_is_full(arr)                             \
    ((arr)->ele_count == (arr)->ele_capa)

#define fs_arr_capa_size(arr)                           \
    ((arr)->ele_capa * (arr)->ele_size)

#define fs_arr_nth(type, arr, n)                        \
    (type *) ((arr)->elems + ((n) * (arr)->ele_size))

#define fs_arr_count(arr)                               \
    ((arr)->ele_count)

#define fs_arr_pool(arr)                                \
    ((arr)->pool)

#define fs_arr_last(type, arr)                          \
    (fs_arr_nth(type, arr, fs_arr_count(arr) - 1))

fs_arr_t *fs_alloc_arr(fs_pool_t *pool, int capa, size_t ele_size);

void *fs_arr_push(fs_arr_t *arr);

static inline int fs_arr_init(fs_arr_t *arr, fs_pool_t *pool, int capa, size_t ele_size) {
    arr->ele_count = 0;
    arr->ele_size = ele_size;
    arr->ele_capa = capa;
    arr->pool = pool;

    if ((arr->elems = fs_pool_alloc(pool, capa * ele_size)) == NULL) {
        return FS_ARR_ERROR;
    }

    return FS_ARR_OK;
}

#endif
