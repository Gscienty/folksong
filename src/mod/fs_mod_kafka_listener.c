/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_mod.h"
#include "fs_conf.h"
#include "fs_arr.h"
#include "fs_util.h"
#include <librdkafka/rdkafka.h>
#include <malloc.h>
#include <unistd.h>

typedef struct fs_mod_kafka_listener_s fs_mod_kafka_listener_t;
struct fs_mod_kafka_listener_s {
    fs_arr_t                *topic;

    rd_kafka_conf_t         *conf;
    rd_kafka_t              *rk;
    rd_kafka_queue_t        *queue;

    int                     fds[2];
    uv_poll_t               handler;

    char                    errstr[512];

    uv_loop_t               *loop;
    fs_log_t                *log;
};

typedef struct fs_mod_kafka_listener_topic_s fs_mod_kafka_listener_topic_t;
struct fs_mod_kafka_listener_topic_s {
    fs_str_t                 topic;

    uv_process_options_t    proc_options;
    bool                    proc_flag;

    uv_loop_t               *loop;
    fs_log_t                *log;
};

typedef struct fs_mod_kafka_listener_proc_handler_s fs_mod_kafka_listener_proc_handler_t;
struct fs_mod_kafka_listener_proc_handler_s {
    uv_stdio_container_t    stdio[3];

    uv_process_t            handler;

    uv_pipe_t               in;
    uv_pipe_t               out;

    uv_process_options_t    options;

    uv_write_t              in_write_req;
    uv_buf_t                read_buf;
    size_t                  readed_len;

    rd_kafka_message_t      *rkm;

    fs_log_t                *log;

    char                    key_param[128];
    char                    *args[64];
};

static int fs_mod_kafka_listener_init_mod(fs_conf_t *conf, fs_cmd_t *cmd, void **ctx);
static int fs_mod_kafka_listener_init_mod_completed(fs_run_t *run, void *ctx);
static int fs_mod_kafka_listener_inited(fs_run_t *run, fs_arr_t *ctxs);

static fs_mod_method_t method = {
    .init_mod           = fs_mod_kafka_listener_init_mod,
    .init_mod_completed = fs_mod_kafka_listener_init_mod_completed,
    .inited             = fs_mod_kafka_listener_inited
};

static int fs_mod_kafka_listener_block(fs_run_t *run, void *ctx);
static int fs_mod_kafka_listener_cmd_topic(fs_run_t *run, void *ctx);
static int fs_mod_kafka_listener_cmd_config(fs_run_t *run, void *ctx);
static int fs_mod_kafka_listener_cmd_exec(fs_run_t *run, void *ctx);

static void fs_mod_kafka_listener_poll_cb(uv_poll_t *handler, int status, int events);
static void fs_mod_kafka_listener_proc_writed_cb(uv_write_t *req, int status);
static void fs_mod_kafka_listener_proc_pipe_closed_cb(uv_handle_t *handle);
static void fs_mod_kafka_listener_proc_exited_cb(uv_process_t *handle, int64_t exit_status, int term_signal);
static void fs_mod_kafka_listener_proc_read_alloc_cb(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
static void fs_mod_kafka_listener_proc_readed_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf);

fs_mod(1, fs_mod_kafka_listener, &method,
       fs_block_cmd     (fs_str("kafka"),    fs_mod_kafka_listener_block),
       fs_param_cmd     (fs_str("config"),   fs_mod_kafka_listener_cmd_config),
       fs_subblock_cmd  (fs_str("topic"),    fs_mod_kafka_listener_cmd_topic),
       fs_param_cmd     (fs_str("exec"),     fs_mod_kafka_listener_cmd_exec));

static int fs_mod_kafka_listener_init_mod(fs_conf_t *conf, fs_cmd_t *cmd, void **ctx) {
    fs_mod_kafka_listener_t *listener;
    if (!fs_cmd_equal(cmd, fs_mod_kafka_listener_block)) {
        return FS_CONF_PASS;
    }
    if (fs_run_st_depth(conf->run) != 1) {
        return FS_CONF_PASS;
    }
    *ctx = fs_pool_alloc(&conf->pool, sizeof(fs_mod_kafka_listener_t));
    listener = *ctx;

    listener->topic = NULL;
    listener->rk = NULL;
    listener->conf = rd_kafka_conf_new();

    listener->log = &conf->log;

    return FS_CONF_OK;
}

static int fs_mod_kafka_listener_init_mod_completed(fs_run_t *run, void *ctx) {
    (void) run;

    fs_mod_kafka_listener_t *listener = ctx;
    fs_mod_kafka_listener_topic_t *topic;

    rd_kafka_topic_partition_list_t *subscriptions;
    int i;

    if (listener->topic == NULL) {
        return FS_CONF_ERROR;
    }

    for (i = 0; i < listener->topic->ele_count; i++) {
        topic = fs_arr_nth(fs_mod_kafka_listener_topic_t, listener->topic, i);

        if (!topic->proc_flag) {
            return FS_CONF_ERROR;
        }
    }

    listener->rk = rd_kafka_new(RD_KAFKA_CONSUMER, listener->conf, listener->errstr, sizeof(listener->errstr));
    if (!listener->rk) {
        return FS_CONF_ERROR;
    }

    subscriptions = rd_kafka_topic_partition_list_new(listener->topic->ele_count);
    for (i = 0; i < listener->topic->ele_count; i++) {
        topic = fs_arr_nth(fs_mod_kafka_listener_topic_t, listener->topic, i);

        fs_log_dbg(listener->log, "fs_mod_kafka: listen topic: %s", fs_str_get(&topic->topic));

        rd_kafka_topic_partition_list_add(subscriptions, fs_str_get(&topic->topic), RD_KAFKA_PARTITION_UA);
    }

    if (rd_kafka_subscribe(listener->rk, subscriptions)) {
        rd_kafka_topic_partition_list_destroy(subscriptions);
        rd_kafka_destroy(listener->rk);

        return FS_CONF_ERROR;
    }

    rd_kafka_topic_partition_list_destroy(subscriptions);

    return FS_CONF_OK;
}

static int fs_mod_kafka_listener_inited(fs_run_t *run, fs_arr_t *ctxs) {

    fs_mod_kafka_listener_t *listener;
    int i = 0;

    for (i = 0; i < ctxs->ele_count; i++) {
        listener = *fs_arr_nth(void *, ctxs, i);

        fs_log_dbg(listener->log, "fs_mod_kafka: listening, nth: %d", i + 1);

        listener->queue = rd_kafka_queue_get_consumer(listener->rk);

        pipe(listener->fds);
        rd_kafka_queue_io_event_enable(listener->queue, listener->fds[1], "1", 1);
        uv_poll_init(fs_run_loop(run), &listener->handler, listener->fds[0]);
        uv_poll_start(&listener->handler, UV_READABLE, fs_mod_kafka_listener_poll_cb);
    }

    listener->loop = fs_run_loop(run);

    return FS_CONF_OK;
}

static int fs_mod_kafka_listener_block(fs_run_t *run, void *ctx) {
    (void) ctx;

    if (fs_run_st_top_mod(run) != &fs_mod_kafka_listener) {
        return FS_CONF_PASS;
    }

    return FS_CONF_OK;
}

static int fs_mod_kafka_listener_cmd_topic(fs_run_t *run, void *ctx) {
    if (fs_run_st_top_mod(run) != &fs_mod_kafka_listener) {
        return FS_CONF_PASS;
    }
    if (!fs_cmd_equal(fs_run_st_subtop_cmd(run), fs_mod_kafka_listener_block)) {
        return FS_CONF_ERROR;
    }

    fs_mod_kafka_listener_t *listener = ctx;

    if (fs_run_tokens(run)->ele_count != 2) {
        return FS_CONF_ERROR;
    }

    if (listener->topic == NULL) {
        listener->topic = fs_alloc_arr(run->pool, 1, sizeof(fs_mod_kafka_listener_topic_t));
    }

    fs_mod_kafka_listener_topic_t *topic = fs_arr_push(listener->topic);
    topic->topic        = *fs_arr_nth(fs_str_t, fs_run_tokens(run), 1);
    topic->log          = listener->log;
    topic->loop         = listener->loop;
    topic->proc_flag    = false;

    return FS_CONF_OK;
}

static int fs_mod_kafka_listener_cmd_config(fs_run_t *run, void *ctx) {
    if (fs_run_st_top_mod(run) != &fs_mod_kafka_listener) {
        return FS_CONF_PASS;
    }
    if (!fs_cmd_equal(fs_run_st_top_cmd(run), &fs_mod_kafka_listener_block)) {
        return FS_CONF_PASS;
    }

    fs_mod_kafka_listener_t *listener = ctx;

    if (fs_run_tokens(run)->ele_count != 3) {
        return FS_CONF_ERROR;
    }

    if (rd_kafka_conf_set(listener->conf,
                          fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 1)),
                          fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 2)),
                          listener->errstr, sizeof(listener->errstr)) != RD_KAFKA_CONF_OK) {
        return FS_CONF_ERROR;
    }

    fs_log_dbg(listener->log, "fs_mod_kafka: set config: %s - %s", 
               fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 1)),
               fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 2)));

    return FS_CONF_OK;
}

static int fs_mod_kafka_listener_cmd_exec(fs_run_t *run, void *ctx) {
    if (fs_run_st_top_mod(run) != &fs_mod_kafka_listener) {
        return FS_CONF_PASS;
    }
    if (!fs_cmd_equal(fs_run_st_top_cmd(run), fs_mod_kafka_listener_cmd_topic)) {
        return FS_CONF_ERROR;
    }

    fs_mod_kafka_listener_t *listener = ctx;
    fs_mod_kafka_listener_topic_t *topic;
    int i;
    char **args = fs_pool_alloc(run->pool, sizeof(char *) * fs_arr_count(fs_run_tokens(run)));

    topic = fs_arr_last(fs_mod_kafka_listener_topic_t, listener->topic);
    topic->proc_options.file    = fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), 1));
    topic->proc_options.args    = args;
    topic->proc_options.flags   = UV_PROCESS_DETACHED;
    topic->proc_options.env     = NULL;

    for (i = 1; i < fs_arr_count(fs_run_tokens(run)); i++) {
        args[i - 1] = fs_str_get(fs_arr_nth(fs_str_t, fs_run_tokens(run), i));
    }
    args[i] = NULL;

    topic->proc_flag = true;

    return FS_CONF_OK;
}

#define fs_mod_kafka_reflect(_handler)                                                                              \
    ((fs_mod_kafka_listener_t *) (((void *) (_handler)) - ((void *) &((fs_mod_kafka_listener_t *) 0)->handler)))


static void fs_mod_kafka_listener_poll_cb(uv_poll_t *handler, int status, int events) {
    fs_mod_kafka_listener_t *listener = fs_mod_kafka_reflect(handler);
    fs_mod_kafka_listener_proc_handler_t *proc;
    fs_mod_kafka_listener_topic_t *topic;
    fs_mod_kafka_listener_topic_t *tmp_topic;
    fs_str_t rkm_topic;
    char unuse;
    int ret;
    int i;

    (void) status;
    (void) events;

    read(listener->fds[0], &unuse, 1);

    rd_kafka_message_t *rkm;
    while ((rkm = rd_kafka_consumer_poll(listener->rk, 0))) {
        if (rkm->err != 0) {
            rd_kafka_message_destroy(rkm);
            break;
        }

        topic = NULL;
        fs_str_set(&rkm_topic, (const char *) rd_kafka_topic_name(rkm->rkt));

        fs_log_dbg(listener->log, "fs_mod_kafka: received message, topic: %s", fs_str_get(&rkm_topic));

        for (i = 0; i < listener->topic->ele_count; i++) {
            tmp_topic = fs_arr_nth(fs_mod_kafka_listener_topic_t, listener->topic, i);

            if (fs_str_cmp(&tmp_topic->topic, &rkm_topic) == 0) {
                topic = tmp_topic;
                break;
            }
        }

        if (topic == NULL) {
            rd_kafka_message_destroy(rkm);
            continue;
        }

        proc = malloc(sizeof(fs_mod_kafka_listener_proc_handler_t));
        if (proc == NULL) {
            rd_kafka_message_destroy(rkm);
            return;
        }

        proc->read_buf.base = NULL;
        proc->read_buf.len = 0;
        proc->readed_len = 0;
        proc->log = listener->log;
        proc->rkm = rkm;

        proc->options = topic->proc_options;
        proc->options.args = proc->args;
        proc->options.stdio_count = 3;
        proc->options.stdio = proc->stdio;
        proc->options.exit_cb = fs_mod_kafka_listener_proc_exited_cb;

        proc->stdio[0].flags = UV_CREATE_PIPE | UV_READABLE_PIPE;
        proc->stdio[1].flags = UV_CREATE_PIPE | UV_WRITABLE_PIPE;
        proc->stdio[2].flags = UV_INHERIT_FD;

        uv_pipe_init(listener->loop, &proc->in, true);
        uv_pipe_init(listener->loop, &proc->out, true);

        proc->stdio[0].data.stream = (uv_stream_t *) &proc->in;
        proc->stdio[1].data.stream = (uv_stream_t *) &proc->out;
        proc->stdio[2].data.fd = 2;

        for (i = 0; topic->proc_options.args[i]; i++) {
            if (strcmp("$kafka_topic", topic->proc_options.args[i]) == 0) {
                proc->options.args[i] = (char *) rd_kafka_topic_name(rkm->rkt);
            }
            else if (strcmp("$kafka_key", topic->proc_options.args[i]) == 0) {
                sprintf(proc->key_param, "%.*s", (int) rkm->key_len, (char *) rkm->key);
                proc->options.args[i] = proc->key_param;
            }
            else {
                proc->options.args[i] = topic->proc_options.args[i];
            }
        }
        proc->options.args[i] = NULL;

        uv_buf_t writed[] = {
            { .base = rkm->payload, .len = rkm->len }
        };

        fs_log_info(proc->log, "fs_mod_kafka: received message, len: %ld", rkm->len);

        if ((ret = uv_spawn(listener->loop, &proc->handler, &proc->options)) != 0) {
            fs_log_err(proc->log, "fs_mod_kafka: %s", uv_strerror(ret));
            return;
        }
        uv_unref((uv_handle_t *) &proc->handler);

        if ((ret = uv_write(&proc->in_write_req, (uv_stream_t *) &proc->in, writed, 1, fs_mod_kafka_listener_proc_writed_cb)) != 0) {
            fs_log_err(proc->log, "fs_mod_kafka: %s", uv_strerror(ret));
            return;
        }

        uv_read_start((uv_stream_t *) &proc->out, fs_mod_kafka_listener_proc_read_alloc_cb, fs_mod_kafka_listener_proc_readed_cb);
    }
}

#define fs_mod_kafka_listener_proc_handler_write_req_reflect(_handler)                                                                          \
    ((fs_mod_kafka_listener_proc_handler_t *) (((void *) (_handler)) - ((void *) &((fs_mod_kafka_listener_proc_handler_t *) 0)->in_write_req)))

static void fs_mod_kafka_listener_proc_writed_cb(uv_write_t *req, int status) {
    fs_mod_kafka_listener_proc_handler_t *proc = fs_mod_kafka_listener_proc_handler_write_req_reflect(req);

    fs_log_dbg(proc->log, "fs_mod_kafka: writed, status: %d", status);

    uv_close((uv_handle_t *) &proc->in, fs_mod_kafka_listener_proc_pipe_closed_cb);
    rd_kafka_message_destroy(proc->rkm);
}

#define fs_mod_kafka_listener_proc_handler_proc_reflect(_handler)                                                                               \
    ((fs_mod_kafka_listener_proc_handler_t *) (((void *) (_handler)) - ((void *) &((fs_mod_kafka_listener_proc_handler_t *) 0)->handler)))

static void fs_mod_kafka_listener_proc_exited_cb(uv_process_t *handle, int64_t exit_status, int term_signal) {
    fs_mod_kafka_listener_proc_handler_t *proc = fs_mod_kafka_listener_proc_handler_proc_reflect(handle);

    fs_log_info(proc->log, "fs_mod_kafka: proc exited, status: %ld, term_signal: %d", exit_status, term_signal);
    fs_log_dbg(proc->log, "fs_mod_kafka: proc output: size: %ld", proc->readed_len);

    if (proc->read_buf.base) {
        free(proc->read_buf.base);
    }
    free(proc);
}

static void fs_mod_kafka_listener_proc_pipe_closed_cb(uv_handle_t *handle) {
    (void) handle;
}

#define fs_mod_kafka_listener_proc_handler_out_reflect(_handler)                                                                                \
    ((fs_mod_kafka_listener_proc_handler_t *) (((void *) (_handler)) - ((void *) &((fs_mod_kafka_listener_proc_handler_t *) 0)->out)))
static void fs_mod_kafka_listener_proc_read_alloc_cb(uv_handle_t *_handler, size_t suggested_size, uv_buf_t *buf) {
    fs_mod_kafka_listener_proc_handler_t *proc = fs_mod_kafka_listener_proc_handler_out_reflect(_handler);

    fs_log_dbg(proc->log, "fs_mod_kakfa: read proc output, suggest_size: %ld", suggested_size);

    if (proc->read_buf.base == NULL) {
        proc->read_buf.base = malloc(suggested_size);
        proc->read_buf.len  = suggested_size;

        *buf = proc->read_buf;
    }
    else {
        size_t remain_size = proc->read_buf.len - proc->readed_len;
        if (remain_size < suggested_size) {
            proc->read_buf.base = realloc(proc->read_buf.base, proc->read_buf.len << 1);
            proc->read_buf.len <<= 1;

            remain_size = proc->read_buf.len - proc->readed_len;
        }

        buf->base = proc->read_buf.base + proc->readed_len;
        buf->len = suggested_size;
    }

}

static void fs_mod_kafka_listener_proc_readed_cb(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    (void) buf;

    fs_mod_kafka_listener_proc_handler_t *proc = fs_mod_kafka_listener_proc_handler_out_reflect(stream);
    if (nread < 0) {
        if (nread == UV_EOF) {
            uv_close((uv_handle_t *) &proc->out, fs_mod_kafka_listener_proc_pipe_closed_cb);
        }
    }
    else {
        proc->readed_len += nread;
    }
}
