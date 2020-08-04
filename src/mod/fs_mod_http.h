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

#define FS_MOD_HTTP_ERROR   -1
#define FS_MOD_HTTP_OK      0

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
    void *conf;

    bool (*match_cb) (void *conf, fs_mod_http_req_t *req);

    int (*process_cb) (fs_mod_http_req_t *req);
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

void fs_mod_http_res_writed_cb(uv_write_t *req, int status);

#define fs_mod_http_response(stream, req, code, status)                                 \
    ({                                                                                  \
        (req)->responsed = true;                                                        \
        const char msg[] =                                                              \
            "HTTP/1.1 " #code " " status "\r\n"                                         \
            "Connection: close\r\n"                                                     \
            "Content-Length: 0\r\n"                                                     \
            "\r\n";                                                                     \
        uv_buf_t not_found_msg[] = {                                                    \
            { .base = (char *) msg, .len = sizeof(msg) }                                \
        };                                                                              \
        uv_write(&req->writer, stream, not_found_msg, 1, fs_mod_http_res_writed_cb);    \
     })

#endif

