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
#include "fs_buf.h"
#include "fs_var.h"
#include "fs_util.h"
#include <pcre.h>

typedef struct fs_mod_http_proxy_s fs_mod_http_proxy_t;
struct fs_mod_http_proxy_s {
    fs_str_t    path;
    pcre        *path_re;
    int         path_captures;

    fs_arr_t    *pass_path;

    union {
        struct sockaddr_in  ipv4;
        struct sockaddr_in6 ipv6;
    } target;

    fs_log_t    *log;
};

typedef struct fs_mod_http_proxy_next_s fs_mod_http_proxy_next_t;
struct fs_mod_http_proxy_next_s {
    fs_str_t    url;

    uv_tcp_t    proxied_ser;
};

extern fs_mod_t fs_mod_http;

static fs_mod_method_t method = {
    .init_mod           = NULL,
    .init_mod_completed = NULL,
    .inited             = NULL
};

static int fs_mod_http_proxy_block(fs_run_t *run, void *ctx);
static int fs_mod_http_proxy_cmd_pass_path(fs_run_t *run, void *ctx);

static bool fs_mod_http_proxy_match_cb(void *conf, fs_mod_http_req_t *req);
static int fs_mod_http_proxy_init_cb(fs_mod_http_req_t *req);
static int fs_mod_http_proxy_body_cb(fs_mod_http_req_t *req, const char *at, size_t len);
static int fs_mod_http_proxy_done_cb(fs_mod_http_req_t *req);
static int fs_mod_http_proxy_release_cb(fs_mod_http_req_t *req);

static int fs_mod_http_proxy_set_next_path(fs_mod_http_req_t *req);
static int fs_mod_http_variable_compile(uv_buf_t *buf, fs_str_t *var, fs_mod_http_req_t *req);

fs_mod(1, fs_mod_http_proxy, &method,
       fs_subblock_cmd  (fs_str("proxy"),       fs_mod_http_proxy_block),
       fs_param_cmd     (fs_str("pass_path"),   fs_mod_http_proxy_cmd_pass_path));
       

static int fs_mod_http_proxy_block(fs_run_t *run, void *ctx) {
    const char *errstr = NULL;
    int erroff = 0;
    int n = 0;
    fs_mod_http_proxy_t *proxy;

    if (fs_run_st_subtop_mod(run) != &fs_mod_http) {
        return FS_CONF_PASS;
    }

    if (fs_arr_count(fs_run_tokens(run)) != 2) {
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

    proxy                       = route->conf;
    proxy->path.buf.buf         = NULL;
    proxy->path_re              = NULL;
    proxy->path_captures        = 0;
    proxy->log                  = http->log;

    proxy->pass_path            = fs_alloc_arr(run->pool, 2, sizeof(fs_str_t));

    proxy->path                 = *fs_arr_nth(fs_str_t, fs_run_tokens(run), 1);
    proxy->path_re              = pcre_compile(fs_str_get(&proxy->path), PCRE_CASELESS, &errstr, &erroff, NULL);
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

static int fs_mod_http_proxy_cmd_pass_path(fs_run_t *run, void *ctx) {
    if (fs_run_st_subtop_mod(run) != &fs_mod_http) {
        return FS_CONF_PASS;
    }

    if (fs_arr_count(fs_run_tokens(run)) != 2) {
        return FS_CONF_ERROR;
    }

    fs_mod_http_t *http         = ctx;
    fs_mod_http_route_t *route  = fs_arr_last(fs_mod_http_route_t, http->routes);
    fs_mod_http_proxy_t *proxy  = route->conf;

    if (fs_arr_count(proxy->pass_path) != 0) {
        return FS_CONF_ERROR;
    }

    if (fs_var_parse(proxy->pass_path, fs_arr_nth(fs_str_t, fs_run_tokens(run), 1)) != FS_VAR_OK) {
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
    req->self = malloc(sizeof(fs_mod_http_proxy_next_t));

    fs_mod_http_proxy_set_next_path(req);

    // TODO modify header parameters / establish conn / send request header

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
    fs_mod_http_proxy_next_t *next = req->self;

    if (next->url.buf.buf) {
        free(next->url.buf.buf);
    }
    free(next);

    return 0;
}

static int fs_mod_http_proxy_set_next_path(fs_mod_http_req_t *req) {
    fs_mod_http_proxy_t *proxy      = req->route->conf;
    fs_mod_http_proxy_next_t *next  = req->self;
    int i;
    fs_str_t *token;

    next->url.buf.buf               = NULL;

    if (fs_arr_count(proxy->pass_path) != 0) {
        uv_buf_t buf = { NULL, 0 };
        for (i = 0; i < fs_arr_count(proxy->pass_path); i++) {
            token = fs_arr_nth(fs_str_t, proxy->pass_path, i);

            if (*fs_str_get(token) != fs_chr_DOLLAR) {
                if (buf.base == NULL) {
                    buf.base = malloc(fs_str_size(token) + 1);
                }
                else {
                    buf.base = realloc(buf.base, buf.len + fs_str_size(token));
                }
                memcpy(buf.base + buf.len, fs_str_get(token), fs_str_size(token));
                buf.len += fs_str_size(token);
                *(char *) (buf.base + buf.len) = 0;
            }
            else {
                fs_mod_http_variable_compile(&buf, token, req);
            }
        }

        fs_str_from_uv(&next->url, &buf, buf.len);
    }

    return FS_MOD_HTTP_OK;
}

static int fs_mod_http_variable_compile(uv_buf_t *buf, fs_str_t *var, fs_mod_http_req_t *req) {
    static const fs_str_t path_prefix = fs_str("${path_");
    char token[128] = { 0 };
    fs_mod_http_proxy_t *proxy = req->route->conf;
    int n;
    size_t len;
    pcre_extra ext = { .flags = 0 };
    int match_vector[128] = { 0 };
    const char *var_val = NULL;
    const char **var_list = NULL;

    if (fs_str_cmp(var, &path_prefix) == 0) {
        do {
            memcpy(token, fs_str_get(var) + fs_str_size(&path_prefix), fs_str_size(var) - fs_str_size(&path_prefix) - 1);

            n = pcre_exec(proxy->path_re, &ext, fs_str_get(&req->url), fs_str_size(&req->url), 0, 0, match_vector, 128);
            if (strspn(token, "0123456789") == strlen(token)) {
                if (pcre_get_substring_list(fs_str_get(&req->url), match_vector, n, &var_list) != 0) {
                    break;
                }
                n = atoi(token);
                if (n > proxy->path_captures) {
                    break;
                }

                var_val = var_list[n];
            }
            else {
                pcre_get_named_substring(proxy->path_re, fs_str_get(&req->url), match_vector, n, token, &var_val);
            }

            if (var_val == NULL || strlen(var_val) == 0) {
                break;
            }

            len = strlen(var_val);
            if (buf->base == NULL) {
                buf->base = malloc(len + 1);
            }
            else {
                buf->base = realloc(buf->base, buf->len + len);
            }
            memcpy(buf->base + buf->len, var_val, len);
            buf->len += len;
        } while (0);

        if (var_list == NULL && var_val != NULL) {
            pcre_free_substring(var_val);
        }
        else if (var_list != NULL) {
            pcre_free_substring_list(var_list);
        }

        return 0;
    }


    return 0;
}
