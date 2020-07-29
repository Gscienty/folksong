/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_STR_H_
#define _FOLK_SONG_STR_H_

#include "fs_buf.h"
#include "fs_util.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

typedef struct fs_str_s fs_str_t;
struct fs_str_s {
    fs_buf_t buf;
};

#define fs_str(str)                                 \
    {                                               \
        .buf = {                                    \
            .buf = (void *) str,                    \
            .pos = (void *) str,                    \
            .last = (void *) str + sizeof(str) - 1, \
            .temp = false                           \
        }                                           \
    }

#define fs_null_str         \
    {                       \
        .buf = {            \
            .buf = NULL,    \
            .pos = NULL,    \
            .last = NULL,   \
            .temp = false   \
        }                   \
    }

#define fs_str_size(str)    \
    fs_buf_size(&(str)->buf)

#define fs_str_set(str, text)                                   \
    ({                                                          \
        (str)->buf.buf = (void *) (text);                       \
        (str)->buf.pos = (void *) (text);                       \
        (str)->buf.last = (void *) (text) + sizeof(text) - 1;   \
        (str)->buf.temp = false;                                \
     })

#define fs_str_get(str)                     \
    ((char *) ((str)->buf.pos))

#define fs_str_buf(str)                     \
    (&(str)->buf)

#define fs_memcpy(dst, src, size)           \
    memcpy((dst), (src), (size))

#define fs_memzero(ptr, size)               \
    memset(ptr, 0, size)

#define fs_str_empty(str)                   \
    (fs_str_size(str) == 0)

#define fs_str_cmp(a, b)                                                                    \
    memcmp(fs_str_get(a), fs_str_get(b), fs_min(size_t, fs_str_size(a), fs_str_size(b)))    \

#define fs_chr_LF       '\n'
#define fs_chr_CR       '\r'
#define fs_chr_SHARP    '#'
#define fs_chr_ESCAPE   '\\'
#define fs_chr_SPACE    ' '
#define fs_chr_TAB      '\t'
#define fs_chr_SEM      ';'
#define fs_chr_LCBRACK  '{'
#define fs_chr_RCBRACK  '}'
#define fs_chr_DQUO     '"'
#define fs_chr_SQUO     '\''
#define fs_chr_DOLLAR   '$'
#define fs_chr_LBRACK   '('
#define fs_chr_RBRACK   ')'

#endif
