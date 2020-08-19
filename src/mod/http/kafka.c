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
#include <malloc.h>
#include <stddef.h>
#include <pcre.h>

typedef struct fs_mod_http_kafka_conf_item_s fs_mod_http_kafka_conf_item_t;
struct fs_mod_http_kafka_conf_item_s {
    char *key;
    char *value;
};

typedef struct fs_mod_http_kafka_publisher_s fs_mod_http_kafka_publisher_t;
struct fs_mod_http_kafka_publisher_s {
    fs_str_t            path;
    pcre                *path_re;

    fs_arr_t            *conf;

    char                errstr[256];
    fs_log_t            *log;
};

typedef struct fs_mod_http_kafka_value_s fs_mod_http_kafka_value_t;
struct fs_mod_http_kafka_value_s {
    uv_buf_t buf;
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
static int fs_mod_http_kafka_init_cb(fs_mod_http_req_t *req);
static int fs_mod_http_kafka_body_cb(fs_mod_http_req_t *req, const char *at, size_t length);
static int fs_mod_http_kafka_done_cb(fs_mod_http_req_t *req);
static int fs_mod_http_kafka_release_cb(fs_mod_http_req_t *req);

fs_mod(1, fs_mod_http_kafka, &method,
       fs_subblock_cmd  (fs_str("kafka"),   fs_mod_http_kafka_block),
       fs_param_cmd     (fs_str("config"),  fs_mod_http_kafka_cmd_config));

static int fs_mod_http_kafka_block(fs_run_t *run, void *ctx) {
    const char *errstr = NULL;
    int erroff = 0;
    fs_mod_http_kafka_publisher_t *publisher;
    if (fs_run_st_subtop_mod(run) != &fs_mod_http) {
        return FS_CONF_PASS;
    }

    if (fs_run_tokens(run)->ele_count != 2) {
        return FS_CONF_ERROR;
    }

    fs_mod_http_t *http         = ctx;
    fs_mod_http_route_t *route  = fs_arr_push(http->routes);

    route->conf         = fs_pool_alloc(run->pool, sizeof(fs_mod_http_kafka_publisher_t));
    route->match_cb     = fs_mod_http_kafka_match_cb;
    route->init_cb      = fs_mod_http_kafka_init_cb;
    route->body_cb      = fs_mod_http_kafka_body_cb;
    route->done_cb      = fs_mod_http_kafka_done_cb;
    route->release_cb   = fs_mod_http_kafka_release_cb;

    publisher           = route->conf;
    publisher->conf     = fs_alloc_arr(http->pool, 2, sizeof(fs_mod_http_kafka_conf_item_t));
    publisher->log      = http->log;

    publisher->path     = *fs_arr_nth(fs_str_t, fs_run_tokens(run), 1);
    publisher->path_re  = pcre_compile(fs_str_get(&publisher->path), PCRE_CASELESS, &errstr, &erroff, NULL);
    if (publisher->path_re == NULL) {
        if ((size_t) erroff == fs_str_size(&publisher->path)) {
            fs_log_err(run->log, "fs_mod_http_kafka: pcre_compile() failed: %s in %s", errstr, fs_str_get(&publisher->path));
        }
        else {
            fs_log_err(run->log, "fs_mod_http_kafka: pcre_compile() failed: %s in %s at %s",
                       errstr, fs_str_get(&publisher->path), fs_str_get(&publisher->path) + erroff);
        }
    }

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

    if (publisher->path_re
        && pcre_exec(publisher->path_re, NULL, fs_str_get(&req->url), fs_str_size(&req->url), 0, 0, NULL, 0) == PCRE_ERROR_NOMATCH) {

        return false;
    }

    return true;
}

static int fs_mod_http_kafka_init_cb(fs_mod_http_req_t *req) {
    fs_mod_http_kafka_value_t *value = fs_pool_alloc(req->pool, sizeof(fs_mod_http_kafka_value_t));

    value->buf.base = NULL;
    value->buf.len = 0;

    req->self = value;

    return 0;
}

static int fs_mod_http_kafka_body_cb(fs_mod_http_req_t *req, const char *at, size_t length) {
    fs_mod_http_kafka_value_t *value = req->self;

    if (value->buf.base == NULL) {
        value->buf.base = malloc(length);
    }
    else {
        value->buf.base = realloc(value->buf.base, value->buf.len + length);
    }

    memcpy(value->buf.base + value->buf.len, at, length);
    value->buf.len += length;
}

static int fs_mod_http_kafka_done_cb(fs_mod_http_req_t *req) {
    int n = 0;
    int match_vector[128] = { 0 };
    fs_mod_http_kafka_publisher_t *publisher = req->route->conf;
    fs_mod_http_kafka_value_t *value = req->self;

    const char *topic = NULL;
    const char *key = NULL;

    pcre_extra ext = { .flags = 0 };
    n = pcre_exec(publisher->path_re, &ext, fs_str_get(&req->url), fs_str_size(&req->url), 0, 0, match_vector, 128);
    pcre_get_named_substring(publisher->path_re, fs_str_get(&req->url), match_vector, n, "topic", &topic);
    if (topic == NULL) {
        fs_mod_http_response((uv_stream_t *) &req->conn, req, 500, "Internal Server Error");

        return FS_MOD_HTTP_ERROR;
    }
    if (strlen(topic) == 0) {
        fs_mod_http_response((uv_stream_t *) &req->conn, req, 400, "Bad Request");

        return FS_MOD_HTTP_OK;
    }

    pcre_get_named_substring(publisher->path_re, fs_str_get(&req->url), match_vector, n, "key", &key);

    rd_kafka_t *rk = NULL;
    rd_kafka_conf_t *rkconf = NULL;
    rd_kafka_topic_t *rkt = NULL;
    rd_kafka_topic_conf_t *rktconf = NULL;
    char errstr[256];
    int ret;
    int i;

    if (value->buf.base == NULL) {
        fs_mod_http_response((uv_stream_t *) &req->conn, req, 400, "Bad Request");

        goto failed;
    }

    rkconf = rd_kafka_conf_new();

    for (i = 0; i < publisher->conf->ele_count; i++) {
        fs_mod_http_kafka_conf_item_t *item = fs_arr_nth(fs_mod_http_kafka_conf_item_t, publisher->conf, i);

        if (rd_kafka_conf_set(rkconf, item->key, item->value, errstr, sizeof(errstr)) != RD_KAFKA_CONF_OK) {
            fs_log_err(req->log, "fs_mod_http_kafka: kafka_conf_set failed, %s", errstr);
            fs_mod_http_response((uv_stream_t *) &req->conn, req, 500, "Internal Server Error");

            goto failed;
        }
    }

    rk = rd_kafka_new(RD_KAFKA_PRODUCER, rkconf, errstr, sizeof(errstr));
    if (!rk) {
        fs_log_err(req->log, "fs_mod_http_kafka: rd_kafka_new failed, %s", errstr);
        fs_mod_http_response((uv_stream_t *) &req->conn, req, 500, "Internal Server Error");

        goto failed;
    }
    rktconf = rd_kafka_topic_conf_new();
    if (!rktconf) {
        fs_log_err(req->log, "fs_mod_http_kafka: rd_kafka_topic_conf_new failed, %d", 0);
        fs_mod_http_response((uv_stream_t *) &req->conn, req, 500, "Internal Server Error");

        goto failed;
    }
    rkt = rd_kafka_topic_new(rk, topic, rktconf);
    if (!rkt) {
        fs_log_err(req->log, "fs_mod_http_kafka: rd_kafka_topic_new failed, %d", 0);
        fs_mod_http_response((uv_stream_t *) &req->conn, req, 500, "Internal Server Error");

        goto failed;
    }

    ret = rd_kafka_produce(rkt, RD_KAFKA_PARTITION_UA,  RD_KAFKA_MSG_F_COPY,
                           value->buf.base, value->buf.len,
                           key, key ? strlen(key) : 0,
                           NULL);
    if (ret) {
        fs_log_err(req->log, "fs_mod_http_kafka: rd_kafka_producev failed, %s", rd_kafka_err2str(ret));
        fs_mod_http_response((uv_stream_t *) &req->conn, req, 500, "Internal Server Error");

        goto failed;
    }

    rd_kafka_poll(rk, 0);
    fs_log_dbg(req->log, "fs_mod_http_kafka: flushing messages, wait for max %dms", 10 * 1000);
    rd_kafka_flush(rk, 10 * 1000);
    fs_log_info(req->log, "fs_mod_http_kafka: send message, topic: %s, key: %s", topic, key);

    fs_mod_http_response((uv_stream_t *) &req->conn, req, 200, "OK");

    rd_kafka_destroy(rk);
    rd_kafka_topic_destroy(rkt);

    pcre_free_substring(topic);
    if (key) {
        pcre_free_substring(key);
    }
    return FS_MOD_HTTP_OK;

failed:
    if (rk) {
        rd_kafka_destroy(rk);
    }
    if (rkt) {
        rd_kafka_topic_destroy(rkt);
    }

    pcre_free_substring(topic);
    if (key) {
        pcre_free_substring(key);
    }

    return FS_MOD_HTTP_ERROR;
}

static int fs_mod_http_kafka_release_cb(fs_mod_http_req_t *req) {
    fs_mod_http_kafka_value_t *value = req->self;

    if (value->buf.base) {
        free(value->buf.base);
    }

    free(value);

    return 0;
}
