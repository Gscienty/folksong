/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_UTIL_H_
#define _FOLK_SONG_UTIL_H_

#define fs_min(type, a, b)      \
    ({                          \
        type _a = a;            \
        type _b = b;            \
        _a < _b ? _a : _b;      \
     })

#define fs_max(type, a, b)      \
    ({                          \
        type _a = a;            \
        type _b = b;            \
        _a > _b ? _a : _b;      \
     })

#endif
