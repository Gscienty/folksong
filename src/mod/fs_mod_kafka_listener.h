/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_MOD_KAFKA_LISTENER_H_
#define _FOLK_SONG_MOD_KAFKA_LISTENER_H_

#include "fs_conf.h"
#include <uv.h>
#include <librdkafka/rdkafka.h>
#include <stdbool.h>

typedef struct fs_mod_kafka_listener_s fs_mod_kafka_listener_t;
struct fs_mod_kafka_listener_s {
    fs_arr_t                *topic;

    rd_kafka_conf_t         *conf;
    rd_kafka_t              *rk;

    int                     fds[2];
    uv_poll_t               handler;

    rd_kafka_queue_t        *queue;
    char                    errstr[512];

    uv_process_options_t    proc_options;
    bool                    proc_flag;

    uv_loop_t               *loop;

    fs_log_t                *log;
};

#endif
