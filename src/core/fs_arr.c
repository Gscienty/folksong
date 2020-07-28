/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_arr.h"
#include "fs_str.h"

fs_arr_t *fs_alloc_arr(fs_pool_t *pool, int capa, size_t ele_size) {
    fs_arr_t *arr;

    if ((arr = fs_pool_alloc(pool, sizeof(fs_arr_t))) == NULL) {
        return NULL;
    }

    if (fs_arr_init(arr, pool, capa, ele_size) != FS_ARR_OK) {
        fs_pool_release(pool, arr);
        return NULL;
    }

    return arr;
}

void *fs_arr_push(fs_arr_t *arr) {
    void *new_elems;

    if (fs_arr_is_full(arr)) {

        if ((new_elems = fs_pool_alloc(fs_arr_pool(arr), fs_arr_capa_size(arr) << 1)) == NULL) {
            return NULL;
        }

        fs_memcpy(new_elems, fs_arr_nth(void, arr, 0), fs_arr_capa_size(arr));

        fs_pool_release(fs_arr_pool(arr), arr->elems);

        arr->elems = new_elems;
        arr->ele_capa <<= 1;
    }

    return fs_arr_nth(void, arr, fs_arr_count(arr)++);
}
