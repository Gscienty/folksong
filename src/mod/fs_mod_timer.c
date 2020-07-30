/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_mod.h"
#include "fs_conf.h"

static int fs_mod_timer_init_mod(fs_conf_t *conf, fs_cmd_t  *cmd, void **ctx) {
    (void) conf;
    (void) cmd;
    (void) ctx;

    return FS_CONF_OK;
}
