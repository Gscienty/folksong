/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_mod.h"
#include <stdio.h>

static int mod_count = 0;

int fs_mod_init() {

    for (mod_count = 0; global_mods[mod_count]; mod_count++) ;

    return FS_MOD_OK;
}
