/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_var.h"
#include "fs_str.h"
#include "fs_buf.h"
#include "fs_arr.h"

#include <stdio.h>

static fs_str_t *fs_var_new_token(fs_arr_t *arr, size_t len);

int fs_var_parse(fs_arr_t *arr, fs_str_t *str) {
    char chr;
    bool variable = false;
    fs_buf_t buf = str->buf;
    fs_str_t *token = NULL;

    while (!fs_buf_overflow(&buf)) {
        chr = fs_buf_lshift(char, &buf);

        switch (chr) {
        case fs_chr_DOLLAR:
            variable = true;
            token = fs_var_new_token(arr, fs_str_size(str) + 1);
            break;

        case fs_chr_LCBRACK:
            if (!variable) {
                return FS_VAR_ERROR;
            }
            break;

        case fs_chr_RCBRACK:
            if (!variable) {
                return FS_VAR_ERROR;
            }

            variable = false;
            *((char *) token->buf.last++) = chr;
            *(char *) token->buf.last = 0;

            token = NULL;
            continue;
        }

        if (token == NULL) {
            token = fs_var_new_token(arr, fs_str_size(str) + 1);
        }
        *((char *) token->buf.last++) = chr;
        *(char *) token->buf.last = 0;
    }

    return FS_VAR_OK;
}

static fs_str_t *fs_var_new_token(fs_arr_t *arr, size_t len) {
    fs_str_t *ret       = fs_arr_push(arr);

    ret->buf.buf        = fs_pool_alloc(arr->pool, len);
    ret->buf.buf_len    = len;
    ret->buf.pos        = ret->buf.buf;
    ret->buf.last       = ret->buf.buf;
    ret->buf.temp       = false;

    return ret;
}
