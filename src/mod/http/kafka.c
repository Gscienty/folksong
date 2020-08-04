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

typedef struct fs_mod_http_kafka_conf_item_s fs_mod_http_kafka_conf_item_t;
struct fs_mod_http_kafka_conf_item_s {
    char *key;
    char *value;
};

typedef struct fs_mod_http_kafka_publisher_s fs_mod_http_kafka_publisher_t;
struct fs_mod_http_kafka_publisher_s {
    fs_str_t            prefix;

    fs_arr_t            *conf;

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
static int fs_mod_http_kafka_process_cb(void *conf, fs_mod_http_req_t *req);

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
    publisher->prefix   = *fs_arr_nth(fs_str_t, fs_run_tokens(run), 1);
    publisher->conf     = fs_alloc_arr(http->pool, 2, sizeof(fs_mod_http_kafka_conf_item_t));
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

    fs_mod_http_kafka_conf_item_t *item = fs_arr_push(publisher->conf);
    item->key   = fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 1));
    item->value = fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 2));

    fs_log_dbg(listener->log, "fs_mod_http_kafka: set config: %s - %s", 
               fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 1)),
               fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 2)));

    return FS_CONF_OK;
}

static bool fs_mod_http_kafka_match_cb(void *conf, fs_mod_http_req_t *req) {
    fs_mod_http_kafka_publisher_t *publisher = conf;

    if (fs_str_size(&publisher->prefix) - 1 > fs_str_size(&req->url)) {
        return false;
    }

    return fs_str_get(&req->url) == strstr(fs_str_get(&req->url), fs_str_get(&publisher->prefix));
}

static int fs_mod_http_kafka_process_cb(void *conf, fs_mod_http_req_t *req) {
    fs_mod_http_kafka_publisher_t *publisher = conf;
    char *topic = fs_str_get(&req->url) + fs_str_size(&publisher->prefix) - 1;
    bool only_topic = true;
    char *key = strchr(topic, '/');
    rd_kafka_t *rk;
    rd_kafka_conf_t *rkconf;
    char errstr[256];
    int ret;
    int i;

    if (key) {
        only_topic = false;
        *key++ = 0;
    }

    if (req->body.pos == 0) {
        fs_mod_http_response((uv_stream_t *) &req->conn, req, 400, "Bad Request");

        return FS_MOD_HTTP_ERROR;
    }

    rkconf = rd_kafka_conf_new();

    for (i = 0; i < publisher->conf->ele_count; i++) {
        fs_mod_http_kafka_conf_item_t *item = fs_arr_nth(fs_mod_http_kafka_conf_item_t, publisher->conf, i);

        if (rd_kafka_conf_set(rkconf, item->key, item->value, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
            fs_log_err(req->log, "fs_mod_http_kafka: kafka_conf_set failed, %s", errstr);
            fs_mod_http_response((uv_stream_t *) &req->conn, req, 500, "Internal Server Error");

            return FS_MOD_HTTP_ERROR;
        }
    }

    rk = rd_kafka_new(RD_KAFKA_PRODUCER, rkconf, errstr, sizeof(errstr));
    if (!rk) {
        fs_log_err(req->log, "fs_mod_http_kafka: rd_kafka_new failed, %s", errstr);
        fs_mod_http_response((uv_stream_t *) &req->conn, req, 500, "Internal Server Error");

        return FS_MOD_HTTP_ERROR;
    }

    ret = rd_kafka_producev(rk,
                            RD_KAFKA_V_TOPIC(topic),
                            RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
                            RD_KAFKA_V_VALUE((char *) req->body.pos, fs_buf_size(&req->body) - 1),
                            RD_KAFKA_V_OPAQUE(NULL),
                            RD_KAFKA_V_END);

    if (ret) {
        fs_log_err(req->log, "fs_mod_http_kafka: rd_kafka_producev failed, %s", rd_kafka_err2str(ret));
        fs_mod_http_response((uv_stream_t *) &req->conn, req, 500, "Internal Server Error");

        return FS_MOD_HTTP_ERROR;
    }

    rd_kafka_poll(rk, 0);
    rd_kafka_flush(rk, 10 * 1000);

    rd_kafka_destroy(rk);

    fs_log_info(req->log, "fs_mod_http_kafka: send message, topic: %s, key: %s", topic, key);

    fs_mod_http_response((uv_stream_t *) &req->conn, req, 200, "OK");
    return FS_MOD_HTTP_OK;
}
