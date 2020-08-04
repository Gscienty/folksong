/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_HTTP_H_
#define _FOLK_SONG_HTTP_H_

#include "fs_core.h"
#include "fs_str.h"
#include "http-parser/http_parser.h"
#include <uv.h>

typedef struct fs_mod_http_s fs_mod_http_t;
typedef struct fs_mod_http_route_s fs_mod_http_route_t;
typedef struct fs_mod_http_req_s fs_mod_http_req_t;

struct fs_mod_http_s {
    uv_tcp_t                conn;

    uint16_t                port;
    fs_str_t                host;

    fs_arr_t                *routes;

    uv_loop_t               *loop;
    fs_pool_t               *pool;
    fs_log_t                *log;
};

struct fs_mod_http_route_s {
    bool (*match_cb) (fs_mod_http_req_t *req);

    int (*init_ctx_cb) (fs_mod_http_req_t *req, void **ctx);
    int (*release_ctx_cb) (void *ctx);

    int (*process_cb) (fs_mod_http_req_t *req, void *ctx);
    int (*response_cb)  (uv_stream_t *stream, fs_mod_http_req_t *req, void *ctx);
};

struct fs_mod_http_req_s {
    uv_tcp_t                conn;
    http_parser_settings    settings;
    http_parser             parser;
    fs_log_t                *log;

    fs_pool_t               *pool;

    fs_arr_t                *routes;
    fs_mod_http_route_t     *route;

    uv_buf_t                read_buf;
    size_t                  readed_len;
    size_t                  parsed_len;

    fs_str_t                url;
    bool                    parsed;
    bool                    responsed;

    uv_write_t              writer;

    fs_buf_t                body;
};

int fs_mod_http_response_404(uv_stream_t *stream, fs_mod_http_req_t *req);
int fs_mod_http_response_200(uv_stream_t *stream, fs_mod_http_req_t *req);

#endif

