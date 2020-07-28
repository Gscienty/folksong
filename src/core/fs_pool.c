/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_pool.h"
#include <malloc.h>

void *fs_pool_alloc(fs_pool_t *pool, size_t size) {
    (void) pool;
    return malloc(size);
}

int fs_pool_release(fs_pool_t *pool, void *ptr) {
    (void) pool;
    free(ptr);
    return 0;
}
