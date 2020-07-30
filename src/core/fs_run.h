/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_RUN_H_
#define _FOLK_SONG_RUN_H_

#include "fs_core.h"
#include "fs_queue.h"
#include "fs_uv.h"

#define FS_RUN_OK   0

struct fs_st_mod_s {
    fs_queue_t  link;
    fs_mod_t    *mod;
    void        **ctx;
};

struct fs_run_s {
    fs_arr_t    *mods;
    fs_arr_t    *ctx;
    fs_queue_t  st_mod;
    fs_queue_t  inited;
    fs_uv_t     *uv;
    fs_pool_t   *pool;

    fs_conf_t   *conf;
};

int fs_run_init(fs_run_t *run, fs_conf_t *conf);

fs_st_mod_t *fs_alloc_st_mod(fs_run_t *run, fs_mod_t *mod);

fs_qe_inited_t *fs_alloc_qe_inited(fs_run_t *run, int (*inited) (fs_run_t *run, void *ctx), void *ctx);

int fs_run(fs_run_t *run);

#define fs_run_st_empty(run)                                \
    (fs_queue_empty(&(run)->st_mod))

#define fs_run_st_top(run)                                  \
    (fs_queue_head(fs_st_mod_t, &(run)->st_mod, link))

#define fs_run_st_top_ctx(run)                              \
    (fs_run_st_top(run)->ctx)

#define fs_run_st_top_mod(run)                              \
    (fs_run_st_top(run)->mod)

#define fs_run_st_pop(run)                                  \
    ({                                                      \
        fs_st_mod_t *_st_mod = fs_run_st_top(run);          \
        fs_queue_remove(&_st_mod->link);                    \
        fs_pool_release((run)->pool, _st_mod);              \
     })

#define fs_run_ctx_push(run)                                \
    (fs_arr_push((run)->ctx))

#define fs_run_tokens(run)                                  \
    ((run)->conf->tokens)

#define fs_run_st_push(run, mod)                                        \
    ({                                                                  \
        fs_st_mod_t *_st_mod = fs_alloc_st_mod(run, mod);               \
        _st_mod->ctx = fs_run_ctx_push(run);                            \
        _st_mod ?                                                       \
            fs_queue_insert_before(&(run)->st_mod, &_st_mod->link)      \
            : NULL;                                                     \
     })

#define fs_run_loop(run)                                                \
    ((run)->uv->loop)

#endif
