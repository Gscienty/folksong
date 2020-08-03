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
#include "fs_mod_kafka_listener.h"
#include <uv.h>
#include <stdlib.h>

extern fs_mod_t fs_mod_timer;
extern fs_mod_t fs_mod_kafka_listener;

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
static int fs_mod_kafka_listener_cmd(fs_run_t *run, fs_mod_kafka_listener_t *listener);

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
    if (fs_run_tokens(run)->ele_count == 1) {
        return FS_CONF_ERROR;
    }

    if (fs_run_st_top_mod(run) == &fs_mod_timer) {
        return fs_mod_timer_exec_cmd(run, ctx);
    }
    if (fs_run_st_top_mod(run) == &fs_mod_kafka_listener) {
        return fs_mod_kafka_listener_cmd(run, ctx);
    }

    return FS_CONF_OK;
}

static int fs_mod_timer_exec_cmd(fs_run_t *run, fs_mod_timer_t *timer) {
    int i;
    char **args = fs_pool_alloc(run->pool, sizeof(char *) * fs_arr_count(fs_run_tokens(run)));

    timer->proc_options.file = fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 1));
    timer->proc_options.args = args;
    timer->proc_options.flags = UV_PROCESS_DETACHED;

    for (i = 1; i < fs_arr_count(fs_run_tokens(run)); i++) {
        args[i - 1] = fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), i));
    }
    args[i] = NULL;

    return FS_CONF_OK;
}

static int fs_mod_kafka_listener_cmd(fs_run_t *run, fs_mod_kafka_listener_t *listener) {
    int i;
    char **args = fs_pool_alloc(run->pool, sizeof(char *) * fs_arr_count(fs_run_tokens(run)));

    listener->proc_options.file = fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 1));
    listener->proc_options.args = args;
    listener->proc_options.flags = UV_PROCESS_DETACHED;

    for (i = 1; i < fs_arr_count(fs_run_tokens(run)); i++) {
        args[i - 1] = fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), i));
    }
    args[i] = NULL;

    listener->proc_flag = true;

    return FS_CONF_OK;
}
