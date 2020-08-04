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
struct fs_mod_http_s {
    uv_tcp_t                conn;

    uint16_t                port;
    fs_str_t                host;

    fs_arr_t                *routes;

    uv_loop_t               *loop;
    fs_pool_t               *pool;
    fs_log_t                *log;
};

typedef struct fs_mod_http_route_s fs_mod_http_route_t;
struct fs_mod_http_route_s {

};

typedef struct fs_mod_http_req_s fs_mod_http_req_t;
struct fs_mod_http_req_s {
    uv_tcp_t                conn;
    http_parser_settings    settings;
    http_parser             parser;
    fs_log_t                *log;

    fs_arr_t                *routes;
    fs_mod_http_route_t     *route;

    uv_buf_t                read_buf;
    size_t                  readed_len;
    size_t                  parsed_len;

    fs_str_t                url;
};

#endif

