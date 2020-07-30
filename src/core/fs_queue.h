/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_QUEUE_H_
#define _FOLK_SONG_QUEUE_H_

#include "fs_core.h"

struct fs_queue_s {
    fs_queue_t *prev;
    fs_queue_t *next;
};

#define fs_queue_init(q)            \
    ({                              \
        (q)->prev = (q);            \
        (q)->next = (q);            \
     })

#define fs_queue_empty(q)           \
    ((q)->next == (q))

#define fs_queue_insert_after(h, q) \
    ({                              \
        (q)->next = (h)->next;      \
        (q)->next->prev = (q);      \
        (q)->prev = (h);            \
        (h)->next = (q);            \
     })

#define fs_queue_insert_before(h, q)    \
    ({                                  \
        (q)->prev = (h)->prev;          \
        (q)->prev->next = (q);          \
        (q)->next = (h);                \
        (h)->prev = (q);                \
     })

#define fs_queue_remove(q)              \
    ({                                  \
        (q)->next->prev = (q)->prev;    \
        (q)->prev->next = (q)->next;    \
     })

#define fs_queue_reflict(type, q, link)                         \
    ((type *) ((void *) (q) - ((void *) &((type *) 0)->link)))

#define fs_queue_payload fs_queue_reflict

#define fs_queue_head(type, q, link)                            \
    (fs_queue_payload(type, (q)->next, link))

#endif
