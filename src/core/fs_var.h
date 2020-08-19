/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_VAR_H_
#define _FOLK_SONG_VAR_H_

#include "fs_core.h"

#define FS_VAR_OK       0
#define FS_VAR_ERROR    -1

int fs_var_parse(fs_arr_t *arr, fs_str_t *str);

#endif
