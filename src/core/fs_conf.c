/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_conf.h"
#include "fs_arr.h"
#include "fs_file.h"
#include "fs_mod.h"
#include "fs_util.h"
#include <unistd.h>
#include <stdio.h>

#define FS_CONF_PAYLOAD_BUFFER_SIZE 4096

static int fs_conf_next_token(fs_conf_t *conf);
static int fs_conf_handle(fs_conf_t *conf, int taken_status);

int fs_conf_parse_cmdline(fs_conf_t *conf, fs_str_t *cmdline) {
    int ret;
    fs_file_t tmp_conf_file;

    if (fs_str_empty(cmdline)) {
        return FS_CONF_OK;
    }

    fs_memzero(&tmp_conf_file, sizeof(fs_file_t));

    tmp_conf_file.buf = *fs_str_buf(cmdline);
    tmp_conf_file.fd = FS_INVALID_FILE;

    conf->file = &tmp_conf_file;

    ret = fs_conf_parse(conf, NULL);

    conf->file = NULL;

    return FS_CONF_OK;
}

int fs_conf_parse(fs_conf_t *conf, fs_str_t *filename) {
    fs_file_t *stored_file;
    fs_file_t file;
    fs_fd_t fd;
    fs_buf_t buf;
    int ret;
    int status;

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
        buf.pos = buf.last;

        conf->file->line = 1;
        conf->file->buf = buf;
        conf->file->name = *filename;

        status = FS_PARSE_FILE;
    }
    else if (!fs_file_is_invalid(conf->file)) {
        status = FS_PARSE_BLOCK;
    }
    else {
        status = FS_PARSE_PARAM;
    }

    for ( ;; ) {
        ret = fs_conf_next_token(conf);

        if (ret == FS_CONF_ERROR) {
            goto done;
        }

        if (ret == FS_CONF_FILE_DONE) {
            if (status == FS_PARSE_BLOCK) {
                goto failure;
            }
            goto done;
        }

        ret = fs_conf_handle(conf, ret);

        if (ret != FS_CONF_OK) {
            // TODO echo error
            goto failure;
        }
    }

failure:
    ret = FS_CONF_ERROR;

done:
    if (filename) {
        fs_buf_release(fs_file_buf(conf->file));
        fs_file_close(conf->file);
        conf->file = stored_file;
    }

    if (ret == FS_CONF_ERROR) {
        return FS_CONF_ERROR;
    }

    return FS_CONF_OK;
}

static int fs_conf_next_token(fs_conf_t *conf) {
    char chr;
    char *src;
    char *dst;

    bool sharp = false;
    bool quoted = false;
    bool d_quote = false;
    bool s_quote = false;
    bool vacancy = true;
    bool need_space = false;
    bool variable = false;
    bool found_token = false;

    fs_str_t *token;
    // TODO release co->tokens's str
    fs_arr_count(conf->tokens) = 0;

    void *token_start = NULL;

    for ( ;; ) {
        if (fs_file_buf_overflow(conf->file)) {
            if (fs_file_done(conf->file)) {
                return FS_CONF_FILE_DONE;
            }
            if (fs_file_read(conf->file, fs_file_remain_size(conf->file)) == FS_FILE_ERROR) {
                return FS_CONF_ERROR;
            }
        }

        chr = fs_file_buf_lshift(char, conf->file);

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
                token_start = fs_file_buf_pos(conf->file);
                vacancy = false;
                quoted = true;
                continue;

            case fs_chr_DQUO:
                token_start = fs_file_buf_pos(conf->file);
                d_quote = true;
                vacancy = false;
                continue;

            case fs_chr_SQUO:
                token_start = fs_file_buf_pos(conf->file);
                s_quote = true;
                vacancy = false;
                continue;

            case fs_chr_DOLLAR:
                token_start = fs_file_buf_pos(conf->file) - 1;
                variable = true;
                vacancy = false;
                continue;

            default:
                token_start = fs_file_buf_pos(conf->file) - 1;
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

                if (fs_buf_alloc(fs_str_buf(token), fs_buf_pos(fs_file_buf(conf->file)) - 1 - token_start + 1) != FS_BUF_OK) {
                    return FS_CONF_ERROR;
                }

                for (dst = fs_str_get(token), src = token_start; (void *) src < fs_file_buf_pos(conf->file) - 1;) {
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
}

static int fs_conf_handle(fs_conf_t *conf, int taken_status) {
    int i;
    int ret;
    fs_cmd_t *cmd;
    fs_str_t *name;

    if (taken_status == FS_CONF_BLOCK_END) {
        if (fs_run_st_empty(conf->run)) {
            return FS_CONF_ERROR;
        }

        if (fs_mod_init_mod_completed(fs_run_st_top_mod(conf->run))) {
            if (fs_mod_init_mod_completed(fs_run_st_top_mod(conf->run))(conf->run, fs_run_st_top_ctx(conf->run)) != FS_CONF_OK) {
                return FS_CONF_ERROR;
            }
        }

        fs_run_st_pop(conf->run);

        return FS_CONF_OK;
    }

    name = fs_arr_nth(fs_str_t, conf->tokens, 0);

    for (i = 0; !fs_gmod_is_end(i); i++) {
        cmd = fs_gmod_cmd(i);
        if (!cmd) {
            continue;
        }

        for ( ; !fs_cmd_is_end(cmd); cmd++) {
            if (fs_str_cmp(fs_cmd_token(cmd), name) != 0) {
                continue;
            }

            if (!fs_cmd_is_block(cmd) && taken_status != FS_CONF_OK) {
                continue;
            }

            if (fs_cmd_is_block(cmd) && taken_status != FS_CONF_BLOCK_START) {
                continue;
            }

            if (fs_cmd_is_child(cmd) && !fs_cmd_is_block(cmd)) {
                // someone parameter

                if (fs_run_st_empty(conf->run)) {
                    return FS_CONF_ERROR;
                }

                if (fs_cmd_call(cmd)(conf->run, *fs_run_st_top_ctx(conf->run)) != FS_CONF_OK) {
                    return FS_CONF_ERROR;
                }
            }
            else {
                if (fs_run_st_push(conf->run, fs_gmod_nth(i)) == NULL) {
                    return FS_CONF_ERROR;
                }

                if (fs_gmod_nth_init_mod(i)) {
                    ret = fs_gmod_nth_init_mod(i)(conf, cmd, fs_run_st_top_ctx(conf->run));

                    if (ret == FS_CONF_PASS) {
                        conf->run->ctx->ele_count--;
                        fs_run_st_pop(conf->run);
                        continue;
                    }
                    if (ret != FS_CONF_OK) {
                        return FS_CONF_ERROR;
                    }
                }

                if (!fs_gmod_nth_used(i)) {
                    fs_gmod_nth_ctxs(i) = fs_alloc_arr(&conf->pool, 1, sizeof(void *));
                }
                fs_gmod_nth_used(i) = true;
                fs_gmod_stored_ctx(i, *fs_run_st_top_ctx(conf->run));

                if (fs_cmd_call(cmd)(conf->run, *fs_run_st_top_ctx(conf->run)) != FS_CONF_OK) {
                    return FS_CONF_ERROR;
                }

                if (taken_status == FS_CONF_OK) {

                    if (fs_gmod_nth_init_mod_completed(i)) {
                        if (fs_gmod_nth_init_mod_completed(i)(conf->run, *fs_run_st_top_ctx(conf->run)) != FS_CONF_OK) {
                            return FS_CONF_ERROR;
                        }
                    }

                    fs_run_st_pop(conf->run);

                }

            }

            return FS_CONF_OK;
        }
    }

    return FS_CONF_ERROR;
}

