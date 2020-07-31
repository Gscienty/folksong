/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_mod.h"
#include "fs_conf.h"
#include "fs_arr.h"
#include "fs_mod_timer.h"
#include <stdlib.h>

extern fs_mod_t fs_mod_timer;

static int fs_mod_exec_init_mod(fs_conf_t *conf, fs_cmd_t *cmd, void **ctx);
static int fs_mod_exec_init_mod_completed(fs_run_t *run, void *ctx);
static int fs_mod_exec_inited(fs_run_t *run, fs_arr_t *ctxs);

static fs_mod_method_t method = {
    .init_mod           = fs_mod_exec_init_mod,
    .init_mod_completed = fs_mod_exec_init_mod_completed,
    .inited             = fs_mod_exec_inited
};

static int fs_mod_exec_cmd(fs_run_t *run, void *ctx);
static int fs_mod_timer_exec_cmd(fs_run_t *run, fs_mod_timer_t *timer);
static int fs_mod_exec_call(void *ctx);

fs_mod(1, fs_mod_exec, &method,
       fs_param_cmd(fs_str("exec"), fs_mod_exec_cmd));

static int fs_mod_exec_init_mod(fs_conf_t *conf, fs_cmd_t *cmd, void **ctx) {
    (void) conf;
    (void) cmd;
    (void) ctx;

    return FS_CONF_OK;
}

static int fs_mod_exec_init_mod_completed(fs_run_t *run, void *ctx) {
    (void) run;
    (void) ctx;

    return FS_CONF_OK;
}

static int fs_mod_exec_inited(fs_run_t *run, fs_arr_t *ctxs) {
    (void) run;
    (void) ctxs;

    return FS_CONF_OK;
}

static int fs_mod_exec_cmd(fs_run_t *run, void *ctx) {
    (void) ctx;

    if (fs_run_tokens(run)->ele_count != 2) {
        return FS_CONF_ERROR;
    }

    if (fs_run_st_top_mod(run) == &fs_mod_timer) {
        return fs_mod_timer_exec_cmd(run, *fs_run_st_top_ctx(run));
    }

    return FS_CONF_OK;
}

static int fs_mod_timer_exec_cmd(fs_run_t *run, fs_mod_timer_t *timer) {

    timer->cb.ctx = fs_arr_nth(fs_str_t, fs_run_tokens(run), 1);
    timer->cb.call = fs_mod_exec_call;

    timer->cb_flag = true;

    return FS_CONF_OK;
}

static int fs_mod_exec_call(void *ctx) {
    system(fs_str_get((fs_str_t *) ctx));
}
