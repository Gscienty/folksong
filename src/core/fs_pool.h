/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_POOL_H_
#define _FOLK_SONG_POOL_H_

#include <sys/types.h>

typedef struct fs_pool_s fs_pool_t;
struct fs_pool_s {

};

void *fs_pool_alloc(fs_pool_t *pool, size_t size);
int fs_pool_release(fs_pool_t *pool, void *ptr);

#endif
