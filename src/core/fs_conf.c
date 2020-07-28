/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_conf.h"
#include "fs_file.h"
#include <unistd.h>

#define FS_CONF_PAYLOAD_BUFFER_SIZE 4096

static int fs_conf_next_token(fs_conf_t *conf);

int fs_conf_parse(fs_conf_t *conf, fs_str_t *filename) {
    fs_file_t *stored_file;
    fs_file_t file;
    fs_fd_t fd;
    fs_buf_t buf;

    if (filename) {
        if((fd = fs_file_open(fs_str_get(filename), FS_FILE_RDONLY, FS_FILE_OPEN, 0)) == FS_INVALID_FILE) {
            // log
            return FS_CONF_ERROR;
        }

        stored_file = conf->file;
        conf->file = &file;

        conf->file->fd = fd;
        fs_file_off(conf->file) = 0;
        if (fs_file_info(fd, fs_file_info_get(conf->file)) == FS_INVALID_FILE) {
            // log
        }

        if (fs_buf_alloc(&buf, FS_CONF_PAYLOAD_BUFFER_SIZE) != 0) {
            // log
            goto failure;
        }

        conf->file->line = 1;
        conf->file->buf = buf;
        conf->file->name = *filename;
    }

    fs_conf_next_token(conf);

failure:
    return 0;
}

static int fs_conf_next_token(fs_conf_t *conf) {
    char chr;
    ssize_t n;
    ssize_t size;
    fs_buf_t buf;

    bool sharp = false;
    bool quoted = false;
    bool d_quote = false;
    bool s_quote = false;
    bool vacancy = true;
    bool need_space = false;
    bool variable = false;
    bool found_token = false;

    fs_str_t *token;

    buf = conf->file->buf;

    buf.pos = buf.buf;
    buf.last = buf.buf;

    void *token_start = NULL;

    for ( ;; ) {
        if (fs_buf_overflow(&buf)) {
            if (fs_file_done(conf->file)) {
                return FS_CONF_FILE_DONE;
            }

            size = fs_file_size(conf->file) - fs_file_off(conf->file);

            if ((size_t) size > fs_buf_capacity(&buf)) {
                size = fs_buf_capacity(&buf);
            }

            n = fs_file_read(conf->file, &buf, size, fs_file_off(conf->file));

            if (n == FS_FILE_ERROR) {
                return FS_CONF_ERROR;
            }

            if (n != size) {
                return FS_CONF_ERROR;
            }
        }

        chr = fs_buf_lshift(char, &buf);

        if (chr == fs_chr_LF) {
            fs_file_line(conf->file)++;

            if (sharp) {
                sharp = false;
            }
        }

        if (sharp) {
            continue;
        }

        if (quoted) {
            quoted = false;
            continue;
        }

        if (need_space) {
            switch (chr) {
            case fs_chr_CR:
            case fs_chr_SPACE:
            case fs_chr_LF:
            case fs_chr_TAB:
                vacancy = true;
                need_space = false;
                continue;

            case fs_chr_SEM:
                return FS_CONF_OK;

            case fs_chr_LCBRACK:
                return FS_CONF_BLOCK_START;
            }
        }

        if (vacancy) {
            switch (chr) {
            case fs_chr_CR:
            case fs_chr_SPACE:
            case fs_chr_LF:
            case fs_chr_TAB:
                continue;

            case fs_chr_SEM:
                return FS_CONF_OK;

            case fs_chr_LCBRACK:
                return FS_CONF_BLOCK_START;

            case fs_chr_RCBRACK:
                return FS_CONF_BLOCK_END;

            case fs_chr_SHARP:
                sharp = true;
                continue;

            case fs_chr_ESCAPE:
                token_start = fs_buf_pos(&buf);
                vacancy = false;
                quoted = true;
                continue;

            case fs_chr_DQUO:
                token_start = fs_buf_pos(&buf);
                d_quote = true;
                vacancy = false;
                continue;

            case fs_chr_SQUO:
                token_start = fs_buf_pos(&buf);
                s_quote = true;
                vacancy = false;
                continue;

            case fs_chr_DOLLAR:
                token_start = fs_buf_pos(&buf) - 1;
                variable = true;
                vacancy = false;
                continue;

            default:
                token_start = fs_buf_pos(&buf) - 1;
                vacancy = false;
            }
        }
        else {
            if (chr == fs_chr_LCBRACK && variable) {
                continue;
            }

            variable = false;

            if (chr == fs_chr_ESCAPE) {
                quoted = true;
                continue;
            }

            if (chr == fs_chr_DOLLAR) {
                variable = true;
                continue;
            }

            if (d_quote) {
                if (chr == fs_chr_DQUO) {
                    d_quote = false;
                    need_space = true;
                    found_token = true;
                }
            }
            else if (s_quote) {
                if (chr == fs_chr_SQUO) {
                    s_quote = false;
                    need_space = true;
                    found_token = true;
                }
            }
            else if (chr == fs_chr_SPACE || chr == fs_chr_TAB || chr == fs_chr_CR || chr == fs_chr_LF || chr == fs_chr_SEM || chr == fs_chr_LCBRACK) {
                vacancy = true;
                found_token = true;
            }

            if (found_token) {

                token = fs_arr_push(conf->tokens);

                if (fs_buf_alloc(fs_str_buf(token), fs_buf_pos(&buf) - 1 - token_start + 1) != FS_BUF_OK) {
                    return FS_CONF_ERROR;
                }

                char *src;
                char *dst;

                for (dst = fs_buf_pos(fs_str_buf(token)), src = token_start; (void *) src < fs_buf_pos(&buf) - 1;) {
                    if (*src == fs_chr_ESCAPE) {
                        switch (*(src + 1)) {
                        case fs_chr_DQUO:
                        case fs_chr_SQUO:
                        case fs_chr_ESCAPE:
                            src++;
                            break;

                        case 't':
                            *dst++ = fs_chr_TAB;
                            src += 2;
                            continue;

                        case 'r':
                            *dst++ = fs_chr_CR;
                            src += 2;
                            continue;

                        case 'n':
                            *dst++ = fs_chr_LF;
                            src += 2;
                            continue;
                        }
                    }

                    *dst++ = *src++;
                }

                *dst = 0;

                if (chr == fs_chr_SEM) {
                    return FS_CONF_OK;
                }

                if (chr == fs_chr_LCBRACK) {
                    return FS_CONF_BLOCK_START;
                }

                found_token = false;
            }
        }
    }


    return FS_CONF_OK;
}
