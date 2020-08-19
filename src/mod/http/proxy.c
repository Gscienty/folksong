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
#include <pcre.h>

typedef struct fs_mod_http_proxy_s fs_mod_http_proxy_t;
struct fs_mod_http_proxy_s {
    fs_str_t    path;
    pcre        *path_re;
    int         path_captures;

    union {
        struct sockaddr_in  ipv4;
        struct sockaddr_in6 ipv6;
    } target;

    fs_log_t    *log;
};

extern fs_mod_t fs_mod_http;

static fs_mod_method_t method = {
    .init_mod           = NULL,
    .init_mod_completed = NULL,
    .inited             = NULL
};

static int fs_mod_http_proxy_block(fs_run_t *run, void *ctx);

static bool fs_mod_http_proxy_match_cb(void *conf, fs_mod_http_req_t *req);
static int fs_mod_http_proxy_init_cb(fs_mod_http_req_t *req);
static int fs_mod_http_proxy_body_cb(fs_mod_http_req_t *req, const char *at, size_t len);
static int fs_mod_http_proxy_done_cb(fs_mod_http_req_t *req);
static int fs_mod_http_proxy_release_cb(fs_mod_http_req_t *req);

fs_mod(1, fs_mod_http_proxy, &method,
       fs_subblock_cmd  (fs_str("proxy"), fs_mod_http_proxy_block));
       

static int fs_mod_http_proxy_block(fs_run_t *run, void *ctx) {
    const char errstr[128] = { 0 };
    int erroff = 0;
    int n = 0;
    fs_mod_http_proxy_t *proxy;

    if (fs_run_st_subtop_mod(run) != &fs_mod_http) {
        return FS_CONF_PASS;
    }

    if (fs_run_tokens(run)->ele_count != 2) {
        return FS_CONF_ERROR;
    }

    fs_mod_http_t *http         = ctx;
    fs_mod_http_route_t *route  = fs_arr_push(http->routes);

    route->conf         = fs_pool_alloc(run->pool, sizeof(fs_mod_http_proxy_t));
    route->match_cb     = fs_mod_http_proxy_match_cb;
    route->init_cb      = fs_mod_http_proxy_init_cb;
    route->body_cb      = fs_mod_http_proxy_body_cb;
    route->done_cb      = fs_mod_http_proxy_done_cb;
    route->release_cb   = fs_mod_http_proxy_release_cb;

    proxy                   = route->conf;
    proxy->path.buf.buf     = NULL;
    proxy->path_re          = NULL;
    proxy->path_captures    = 0;
    proxy->log              = http->log;

    proxy->path             = *fs_arr_nth(fs_str_t, fs_run_tokens(run), 1);
    proxy->path_re          = pcre_compile(fs_str_get(&proxy->path), PCRE_CASELESS, (const char **) errstr, &erroff, NULL);

    if (proxy->path_re == NULL) {
        if ((size_t) erroff == fs_str_size(&proxy->path)) {
            fs_log_err(run->log, "fs_mod_http_proxy: pcre_compile() failed: %s in %s", errstr, fs_str_get(&proxy->path));
        }
        else {
            fs_log_err(run->log, "fs_mod_http_proxy: pcre_compile() failed: %s in %s at %s",
                       errstr, fs_str_get(&proxy->path), fs_str_get(&proxy->path) + erroff);
        }
    }

    if ((n = pcre_fullinfo(proxy->path_re, NULL, PCRE_INFO_CAPTURECOUNT, &proxy->path_captures)) < 0) {
        fs_log_err(run->log, "fs_mod_http_proxy: pcre_fullinfo() failed, %d", n);
        return FS_CONF_ERROR;
    }

    return FS_CONF_OK;
}

static bool fs_mod_http_proxy_match_cb(void *conf, fs_mod_http_req_t *req) {
    fs_mod_http_proxy_t *proxy = conf;

    if (proxy->path_re
        && pcre_exec(proxy->path_re, NULL, fs_str_get(&req->url), fs_str_size(&req->url), 0, 0, NULL, 0) == PCRE_ERROR_NOMATCH) {

        return false;
    }

    return true;
}

static int fs_mod_http_proxy_init_cb(fs_mod_http_req_t *req) {
    (void) req;

    return 0;
}

static int fs_mod_http_proxy_body_cb(fs_mod_http_req_t *req, const char *at, size_t len) {
    (void) req;
    (void) at;
    (void) len;

    return 0;
}

static int fs_mod_http_proxy_done_cb(fs_mod_http_req_t *req) {
    (void) req;

    fs_mod_http_response((uv_stream_t *) &req->conn, req, 200, "OK");
    return 0;
}

static int fs_mod_http_proxy_release_cb(fs_mod_http_req_t *req) {
    (void) req;

    return 0;
}
