/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_conf.h"
#include "fs_mod.h"
#include "fs_cmd.h"
#include "fs_arr.h"
#include "fs_mod_http.h"
#include <malloc.h>
#include <stdlib.h>
#include <netinet/in.h>

#define FS_DEFAULT_BACKLOG 128

static int fs_mod_http_init_mod(fs_conf_t *conf, fs_cmd_t *cmd, void **ctx);
static int fs_mod_http_init_mod_completed(fs_run_t *run, void *ctx);
static int fs_mod_http_inited(fs_run_t *run, fs_arr_t *ctxs);

static fs_mod_method_t method = {
    .init_mod           = fs_mod_http_init_mod,
    .init_mod_completed = fs_mod_http_init_mod_completed,
    .inited             = fs_mod_http_inited
};

static int fs_mod_http_block(fs_run_t *run, void *ctx);
static int fs_mod_http_cmd_host(fs_run_t *run, void *ctx);
static int fs_mod_http_cmd_port(fs_run_t *run, void *ctx);

static void fs_mod_http_connection_cb(uv_stream_t *stream, int status);
static void fs_mod_http_req_read_alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void fs_mod_http_req_readed_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

static int fs_mod_http_on_url(http_parser *parser, const char *at, size_t length);
static int fs_mod_http_on_body(http_parser *parser, const char *at, size_t length);
static int fs_mod_http_on_message_complete(http_parser *parser);

static int fs_mod_http_req_release(fs_mod_http_req_t *req);

fs_mod(1, fs_mod_http, &method,
       fs_block_cmd(fs_str("http"),         fs_mod_http_block),
       fs_param_cmd(fs_str("http_host"),    fs_mod_http_cmd_host),
       fs_param_cmd(fs_str("http_port"),    fs_mod_http_cmd_port));

static int fs_mod_http_init_mod(fs_conf_t *conf, fs_cmd_t *cmd, void **ctx) {
    fs_mod_http_t *http;

    if (!fs_cmd_equal(cmd, fs_mod_http_block)) {
        return FS_CONF_ERROR;
    }
    *ctx = fs_pool_alloc(&conf->pool, sizeof(fs_mod_http_t));
    http = *ctx;

    http->log = &conf->log;
    http->routes = fs_alloc_arr(&conf->pool, 1, sizeof(fs_mod_http_route_t));

    return FS_CONF_OK;
}

static int fs_mod_http_init_mod_completed(fs_run_t *run, void *ctx) {
    int ret;
    struct sockaddr_in addr;
    fs_mod_http_t *http = ctx;

    if ((ret = uv_ip4_addr(fs_str_get(&http->host), http->port, &addr)) != 0) {
        fs_log_err(run->log, "fs_mod_http: transfe ip4_addr failed, %s", uv_strerror(ret));
        return FS_CONF_ERROR;
    }

    if ((ret = uv_tcp_init(fs_run_loop(run), &http->conn)) != 0) {
        fs_log_err(run->log, "fs_mod_http: init tcp failed, %s", uv_strerror(ret));
        return FS_CONF_ERROR;
    }
    if ((ret = uv_tcp_bind(&http->conn, (struct sockaddr *) &addr, sizeof(struct sockaddr_in))) != 0) {
        fs_log_err(run->log, "fs_mod_http: bind addr failed, %s", uv_strerror(ret));
        return FS_CONF_ERROR;
    }

    return FS_CONF_OK;
}

static int fs_mod_http_inited(fs_run_t *run, fs_arr_t *ctxs) {
    fs_mod_http_t *http;
    int i;

    for (i = 0; i < ctxs->ele_count; i++) {
        http = *fs_arr_nth(void *, ctxs, i);

        uv_listen((uv_stream_t *) &http->conn, FS_DEFAULT_BACKLOG, fs_mod_http_connection_cb);
    }

    http->loop = fs_run_loop(run);

    return FS_CONF_OK;
}

static int fs_mod_http_block(fs_run_t *run, void *ctx) {
    (void) run;
    (void) ctx;

    return FS_CONF_OK;
}

static int fs_mod_http_cmd_host(fs_run_t *run, void *ctx) {
    fs_mod_http_t *http = ctx;

    if (fs_run_tokens(run)->ele_count != 2) {
        return FS_CONF_ERROR;
    }

    http->host = *fs_arr_nth(fs_str_t, fs_run_tokens(run), 1);

    return FS_CONF_OK;
}

static int fs_mod_http_cmd_port(fs_run_t *run, void *ctx) {
    fs_mod_http_t *http = ctx;

    if (fs_run_tokens(run)->ele_count != 2) {
        return FS_CONF_ERROR;
    }

    http->port = atoi(fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 1)));

    return FS_CONF_OK;
}

#define fs_mod_http_reflect(_handler)                                                       \
    ((fs_mod_http_t *) (((void *) (_handler)) - ((void *) &((fs_mod_http_t *) 0)->conn)))

static void fs_mod_http_connection_cb(uv_stream_t *stream, int status) {
    fs_mod_http_t *http = fs_mod_http_reflect(stream);
    fs_mod_http_req_t *req;

    if (status != 0) {
        fs_log_err(http->log, "fs_mod_http: connection failed, status: %d", status);
        return;
    }

    req = fs_pool_alloc(http->pool, sizeof(fs_mod_http_req_t));
    http_parser_settings_init(&req->settings);
    http_parser_init(&req->parser, HTTP_REQUEST);

    req->log                = http->log;
    req->read_buf.base      = NULL;
    req->read_buf.len       = 0;
    req->readed_len         = 0;
    req->parsed_len         = 0;
    req->routes             = http->routes;

    req->settings.on_url    = fs_mod_http_on_url;
    req->settings.on_body   = fs_mod_http_on_body;

    if (uv_tcp_init(http->loop, &req->conn) != 0) {
        fs_log_err(http->log, "fs_mod_http: init conn failed, status: %d", status);
        return;
    }

    if (uv_accept(stream, (uv_stream_t *) &req->conn) != 0) {
        fs_log_err(http->log, "fs_mod_http: accept stream failed, status: %d", status);
        return;
    }

    uv_read_start((uv_stream_t *) &req->conn, fs_mod_http_req_read_alloc_cb, fs_mod_http_req_readed_cb);
}

#define fs_mod_http_req_conn_reflect(_handler)                                                      \
    ((fs_mod_http_req_t *) (((void *) (_handler)) - ((void *) &((fs_mod_http_req_t *) 0)->conn)))
static void fs_mod_http_req_read_alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    fs_mod_http_req_t *req = fs_mod_http_req_conn_reflect(handle);

    fs_log_dbg(req->log, "fs_mod_http: read http req, suggested_size: %ld", suggested_size);

    if (req->read_buf.base == NULL) {
        req->read_buf.base  = malloc(suggested_size);
        req->read_buf.len   = suggested_size;

        *buf = req->read_buf;
    }
    else {
        size_t remain_size = req->read_buf.len - req->readed_len;
        if (remain_size < suggested_size) {
            req->read_buf.base = realloc(req->read_buf.base, req->read_buf.len << 1);
            req->read_buf.len <<= 1;

            remain_size = req->read_buf.len - req->readed_len;
        }

        buf->base = req->read_buf.base + req->readed_len;
        buf->len = suggested_size;
    }
}

static void fs_mod_http_req_readed_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    fs_mod_http_req_t *req = fs_mod_http_req_conn_reflect(stream);

    if (nread < 0) {
        if (nread == UV_EOF) {
            fs_log_info(req->log, "fs_mod_http_req: readed EOF, %ld\n", nread);

            // TODO close
        }
    }
    else {
        do {
            http_parser_execute(&req->parser, &req->settings, buf->base, nread);
            // note: one connection may have multi http req
        } while (req->parser.content_length != 0);
    }
}

#define fs_mod_http_req_parser_reflect(_handler)                                                      \
    ((fs_mod_http_req_t *) (((void *) (_handler)) - ((void *) &((fs_mod_http_req_t *) 0)->parser)))
static int fs_mod_http_on_url(http_parser *parser, const char *at, size_t length) {
    fs_mod_http_req_t *req = fs_mod_http_req_parser_reflect(parser);
    fs_str_set_with_len(&req->url, at, length);

    // TODO find route

    return 0;
}

static int fs_mod_http_on_body(http_parser *parser, const char *at, size_t length) {
    // TODO start mark body
}

static int fs_mod_http_on_message_complete(http_parser *parser) {
    // TODO do route
}

static int fs_mod_http_req_release(fs_mod_http_req_t *req) {
    // TODO release req
}
