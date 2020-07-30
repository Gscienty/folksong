/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_run.h"
#include "fs_mod.h"
#include "fs_pool.h"
#include "fs_arr.h"

int fs_run_init(fs_run_t *run, fs_pool_t *pool) {
    run->pool = pool;
    run->mods = fs_alloc_arr(pool, 8, sizeof(fs_mod_t *));
    run->ctx = fs_alloc_arr(pool, 8, sizeof(void *));
    fs_queue_init(&run->st_mod);

    return FS_RUN_OK;
}

fs_st_mod_t *fs_alloc_st_mod(fs_run_t *run, fs_mod_t *mod) {
    fs_st_mod_t *st_mod;
    if ((st_mod = fs_pool_alloc(run->pool, sizeof(fs_st_mod_t))) == NULL) {
        return NULL;
    }

    fs_queue_init(&st_mod->link);
    st_mod->mod = mod;

    return st_mod;
}
