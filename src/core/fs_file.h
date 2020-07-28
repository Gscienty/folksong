/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#ifndef _FOLK_SONG_FILE_H_
#define _FOLK_SONG_FILE_H_

#include "fs_str.h"
#include "fs_buf.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define FS_FILE_ERROR -20001

typedef int fs_fd_t;

typedef struct fs_file_s fs_file_t;
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



ssize_t fs_file_read(fs_file_t *file, fs_buf_t *buf, size_t size, off_t off);

#endif
