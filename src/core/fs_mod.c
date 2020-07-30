/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_mod.h"
#include "fs_arr.h"
#include <stdio.h>

int fs_gmod_init() {

    int i;

    for (i = 0; fs_gmod_nth(i); i++) {
        fs_gmod_nth(i)->used = false;
    }

    return FS_MOD_OK;
}

int fs_gmod_inited(fs_run_t *run) {

    int i = 0;

    for (i = 0; fs_gmod_nth(i); i++) {
        if (fs_gmod_nth_used(i)) {
            fs_gmod_nth_inited(i)(run, fs_gmod_nth_ctxs(i));
        }
    }

    return FS_MOD_OK;
}
