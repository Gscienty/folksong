/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_mod.h"
#include "fs_conf.h"
#include "fs_mod_timer.h"
#include "fs_arr.h"
#include "fs_util.h"
#include <stdlib.h>

static int fs_mod_timer_init_mod(fs_conf_t *conf, fs_cmd_t *cmd, void **ctx);
static int fs_mod_timer_init_mod_completed(fs_run_t *run, void *ctx);
static int fs_mod_timer_inited(fs_run_t *run, fs_arr_t *ctxs);

static fs_mod_method_t method = {
    .init_mod           = fs_mod_timer_init_mod,
    .init_mod_completed = fs_mod_timer_init_mod_completed,
    .inited             = fs_mod_timer_inited
};

static int fs_mod_timer_cmd(fs_run_t *run, void *ctx);
static int fs_mod_timer_cmd_timeout(fs_run_t *run, void *ctx);
static int fs_mod_timer_cmd_repeat(fs_run_t *run, void *ctx);
static int fs_mod_timer_parse_time(uint64_t *ret, fs_str_t *time);

static void fs_mod_timer_cb(uv_timer_t *handler);

fs_mod(1, fs_mod_timer, &method,
       fs_block_cmd(fs_str("timer"),    fs_mod_timer_cmd),
       fs_param_cmd(fs_str("timeout"),  fs_mod_timer_cmd_timeout),
       fs_param_cmd(fs_str("repeat"),   fs_mod_timer_cmd_repeat));

static int fs_mod_timer_init(fs_mod_timer_t *timer);

static int fs_mod_timer_init_mod(fs_conf_t *conf, fs_cmd_t *cmd, void **ctx) {
    (void) conf;

    if (fs_cmd_equal(cmd, fs_mod_timer_cmd)) {
        *ctx = fs_pool_alloc(&conf->pool, sizeof(fs_mod_timer_t));

        fs_mod_timer_init(*ctx);
    }
    else {
        return FS_CONF_ERROR;
    }

    return FS_CONF_OK;
}

static int fs_mod_timer_init_mod_completed(fs_run_t *run, void *ctx) {
    (void) run;
    fs_mod_timer_t *timer = ctx;

    if ((!timer->repeat_flag && !timer->timeout_flag) || !timer->cb_flag) {
        return FS_CONF_ERROR;
    }

    return FS_CONF_OK;
}

static int fs_mod_timer_inited(fs_run_t *run, fs_arr_t *ctxs) {
    (void) run;
    fs_mod_timer_t *timer;

    int i;
    for (i = 0; i < ctxs->ele_count; i++) {
        timer = fs_mod_inited_nth_ctx(fs_mod_timer_t, ctxs, i);

        uv_timer_init(fs_run_loop(run), &timer->handler);
        uv_timer_start(&timer->handler, fs_mod_timer_cb, timer->timeout, timer->repeat);
    }

    return FS_CONF_OK;
}

static int fs_mod_timer_init(fs_mod_timer_t *timer) {
    static const fs_mod_timer_t inited_timer  = {
        .timeout        = 0,
        .repeat         = 0,
        .timeout_flag   = false,
        .repeat_flag    = false,
        .repeat_always  = false,
        .cb_flag        = false,
    };

    *timer = inited_timer;

    return FS_CONF_OK;
}

static int fs_mod_timer_cmd(fs_run_t *run, void *ctx) {
    (void) ctx;

    if (fs_run_tokens(run)->ele_count == 1) {
        return FS_CONF_OK;
    }

    return FS_CONF_OK;
}

static int fs_mod_timer_cmd_timeout(fs_run_t *run, void *ctx) {
    fs_mod_timer_t *timer = ctx;

    if (fs_run_st_top_mod(run) != &fs_mod_timer) {
        return FS_CONF_ERROR;
    }

    if (timer->timeout_flag) {
        return FS_CONF_ERROR;
    }

    if (fs_run_tokens(run)->ele_count != 2) {
        return FS_CONF_ERROR;
    }

    if (fs_mod_timer_parse_time(&timer->timeout, fs_arr_nth(fs_str_t, fs_run_tokens(run), 1)) != FS_CONF_OK) {
        return FS_CONF_ERROR;
    }
    timer->timeout_flag = true;

    return FS_CONF_OK;
}

static int fs_mod_timer_cmd_repeat(fs_run_t *run, void *ctx) {
    fs_mod_timer_t *timer = ctx;

    if (fs_run_st_top_mod(run) != &fs_mod_timer) {
        return FS_CONF_ERROR;
    }

    if (timer->repeat_flag) {
        return FS_CONF_ERROR;
    }

    if (fs_run_tokens(run)->ele_count != 2) {
        return FS_CONF_ERROR;
    }

    if (fs_mod_timer_parse_time(&timer->repeat, fs_arr_nth(fs_str_t, fs_run_tokens(run), 1)) != FS_CONF_OK) {
        return FS_CONF_ERROR;
    }
    timer->repeat_flag = true;

    return FS_CONF_OK;
}

static int fs_mod_timer_parse_time(uint64_t *ret, fs_str_t *time) {
    char *sp;

    for (sp = fs_str_get(time); *sp; sp++) {
        if (*sp < '0' || '9' < *sp) {
            break;
        }
    }

    if (!*sp) {
        *ret = atoll(fs_str_get(time));
    }
    else if (strcmp(sp, "ms") == 0) {
        *sp = 0;
        *ret = atoll(fs_str_get(time));
        *sp = 'm';
    }
    else if (strcmp(sp, "s") == 0) {
        *sp = 0;
        *ret = atoll(fs_str_get(time)) * 1000;
        *sp = 's';
    }
    else if (strcmp(sp, "min") == 0) {
        *sp = 0;
        *ret = atoll(fs_str_get(time)) * 1000 * 60;
        *sp = 'm';
    }
    else if (strcmp(sp, "hour") == 0) {
        *sp = 0;
        *ret = atoll(fs_str_get(time)) * 1000 * 60 * 60;
        *sp = 'h';
    }
    else if (strcmp(sp, "day") == 0) {
        *sp = 0;
        *ret = atoll(fs_str_get(time)) * 1000 * 60 * 60 * 24;
        *sp = 'd';
    }
    else {
        return FS_CONF_ERROR;
    }

    return FS_CONF_OK;
}

#define fs_mod_timer_reflect(_handler)                                                              \
    ((fs_mod_timer_t *) (((void *) (_handler)) - ((void *) &((fs_mod_timer_t *) 0)->handler)))

uv_process_t proc;
static void fs_mod_timer_cb(uv_timer_t *handler) {
    fs_mod_timer_t *timer = fs_mod_timer_reflect(handler);

    uv_stdio_container_t stdio[3];
    stdio[0].flags = UV_IGNORE;
    stdio[1].flags = UV_INHERIT_FD;
    stdio[1].data.fd = 1;
    stdio[2].flags = UV_IGNORE;

    timer->proc_options.stdio_count = 3;
    timer->proc_options.stdio = stdio;

    timer->proc_options.exit_cb = NULL;

    uv_spawn(handler->loop, &proc, &timer->proc_options);

    uv_unref((uv_handle_t *) &proc);
}
