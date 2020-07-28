/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_file.h"


ssize_t fs_file_read(fs_file_t *file, fs_buf_t *buf, size_t size, off_t off) {
    if (lseek(file->fd, off, SEEK_SET) == -1) {
        return FS_FILE_ERROR;
    }

    file->off = off;

    ssize_t n = read(file->fd, buf->buf, size);

    if (n == -1) {
        return FS_FILE_ERROR;
    }

    buf->pos = buf->buf;
    file->off += n;
    buf->last = buf->pos + n;

    return n;
}
