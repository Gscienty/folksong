/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_FILE_H_
#define _FOLK_SONG_FILE_H_

#include "fs_core.h"
#include "fs_buf.h"
#include "fs_str.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FS_FILE_ERROR -20001

struct fs_file_s {
    fs_str_t    name;
    fs_fd_t     fd;
    struct stat info;
    fs_buf_t    buf;
    size_t      off;
    int         line;
};

#define FS_FILE_RDONLY  O_RDONLY
#define FS_FILE_OPEN    0

#define FS_INVALID_FILE -1

#define fs_file_open(name, mode, create, access)    \
    open((const char *) name, mode | create, access)

#define fs_file_info_get(file)                      \
    (&(file)->info)

#define fs_file_info(fd, info)                      \
    fstat(fd, info)

#define fs_file_size(file)                          \
    (fs_file_info_get(file)->st_size)

#define fs_file_off(file)                           \
    ((file)->off)

#define fs_file_line(file)                          \
    ((file)->line)

#define fs_file_done(file)                          \
    ((long) (file)->off >= fs_file_size(file))

#define fs_file_is_invalid(file)                    \
    ((file)->fd == FS_INVALID_FILE)

#define fs_file_buf(file)                           \
    (&(file)->buf)

#define fs_file_close(file)                         \
    close((file)->fd)

#define fs_file_buf_overflow(file)                  \
    fs_buf_overflow(fs_file_buf(file))

#define fs_file_buf_capacity(file)                  \
    fs_buf_capacity(fs_file_buf(file))

#define fs_file_remain_size(file)                   \
    (fs_file_size(file) - fs_file_off(file))

#define fs_file_read(file, size)                                                                    \
    ({                                                                                              \
        ssize_t _target = (size);                                                                   \
        if ((size_t) _target > fs_file_buf_capacity(file)) {                                        \
            _target = fs_file_buf_capacity(file);                                                   \
        }                                                                                           \
        ssize_t _n = fs_file_read_in_buf(file, fs_file_buf(file), _target, fs_file_off(file));      \
        int err;                                                                                    \
        if (_n == FS_FILE_ERROR) {                                                                  \
            err = FS_FILE_ERROR;                                                                    \
        }                                                                                           \
        if (_n != _target) {                                                                        \
            err = FS_FILE_ERROR;                                                                    \
        }                                                                                           \
        err;                                                                                        \
     })

#define fs_file_buf_lshift(type, file)              \
    (fs_buf_lshift(type, fs_file_buf(file)))

#define fs_file_buf_pos(file)                       \
    (fs_buf_pos(fs_file_buf(file)))

ssize_t fs_file_read_in_buf(fs_file_t *file, fs_buf_t *buf, size_t size, off_t off);

#endif
