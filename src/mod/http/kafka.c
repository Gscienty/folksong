/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_core.h"
#include "fs_conf.h"
#include "fs_arr.h"
#include "fs_mod_http.h"
#include <librdkafka/rdkafka.h>

typedef struct fs_mod_http_kafka_publisher_s fs_mod_http_kafka_publisher_t;
struct fs_mod_http_kafka_publisher_s {
    fs_str_t            prefix;
    rd_kafka_conf_t     *conf;

    char                errstr[256];
    fs_log_t            *log;
};

extern fs_mod_t fs_mod_http;

static fs_mod_method_t method = {
    .init_mod           = NULL,
    .init_mod_completed = NULL,
    .inited             = NULL
};

static int fs_mod_http_kafka_block(fs_run_t *run, void *ctx);
static int fs_mod_http_kafka_cmd_config(fs_run_t *run, void *ctx);

static bool fs_mod_http_kafka_match_cb(void *conf, fs_mod_http_req_t *req);
static int fs_mod_http_kafka_process_cb(fs_mod_http_req_t *req);

fs_mod(1, fs_mod_http_kafka, &method,
       fs_subblock_cmd  (fs_str("kafka"), fs_mod_http_kafka_block),
       fs_param_cmd     (fs_str("config"), fs_mod_http_kafka_cmd_config));

static int fs_mod_http_kafka_block(fs_run_t *run, void *ctx) {
    fs_mod_http_kafka_publisher_t *publisher;
    if (fs_run_st_subtop_mod(run) != &fs_mod_http) {
        return FS_CONF_PASS;
    }

    if (fs_run_tokens(run)->ele_count != 2) {
        return FS_CONF_ERROR;
    }

    fs_mod_http_t *http = ctx;
    fs_mod_http_route_t *route = fs_arr_push(http->routes);

    route->conf         = fs_pool_alloc(run->pool, sizeof(fs_mod_http_kafka_publisher_t));
    route->match_cb     = fs_mod_http_kafka_match_cb;
    route->process_cb   = fs_mod_http_kafka_process_cb;
    publisher           = route->conf;
    publisher->conf     = rd_kafka_conf_new();
    publisher->prefix   = *fs_arr_nth(fs_str_t, fs_run_tokens(run), 1);
    publisher->log      = http->log;

    return FS_CONF_OK;
}

static int fs_mod_http_kafka_cmd_config(fs_run_t *run, void *ctx) {
    if (!fs_cmd_equal(fs_run_st_top_cmd(run), fs_mod_http_kafka_block)) {
        return FS_CONF_PASS;
    }

    if (fs_run_tokens(run)->ele_count != 3) {
        return FS_CONF_ERROR;
    }

    fs_mod_http_t *http                         = ctx;
    fs_mod_http_route_t *route                  = fs_arr_last(fs_mod_http_route_t, http->routes);
    fs_mod_http_kafka_publisher_t *publisher    = route->conf;


    if (rd_kafka_conf_set(publisher->conf,
                          fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 1)),
                          fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 2)),
                          publisher->errstr, sizeof(publisher->errstr)) != RD_KAFKA_CONF_OK) {
        return FS_CONF_ERROR;
    }

    fs_log_dbg(listener->log, "fs_mod_http_kafka: set config: %s - %s", 
               fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 1)),
               fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 2)));

    return FS_CONF_OK;
}

static bool fs_mod_http_kafka_match_cb(void *conf, fs_mod_http_req_t *req) {

}

static int fs_mod_http_kafka_process_cb(fs_mod_http_req_t *req) {

}
