/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_mod.h"
#include "fs_conf.h"
#include <stdio.h>

typedef struct fs_mod_version_s fs_mod_version_t;
struct fs_mod_version_s {
    char *welcome;
};

static int fs_mod_verison_init(fs_run_t *run, void *ctx) {
    (void) run;

    ((fs_mod_version_t *) ctx)->welcome = "folk song";

    return FS_CONF_OK;
}

static int fs_mod_version_echo(fs_run_t *run, void *ctx) {
    (void) run;

    ((fs_mod_version_t *) ctx)->welcome = 
         "_____ ____  _     _  __   ____  ____  _      _____\n"
         "/    //  _ \\/ \\   / |/ /  / ___\\/  _ \\/ \\  /|/  __/\n"
         "|  __\\| / \\|| |   |   /   |    \\| / \\|| |\\ ||| |  _\n"
         "| |   | \\_/|| |_/\\|   \\   \\___ || \\_/|| | \\||| |_//\n"
         "\\_/   \\____/\\____/\\_|\\_\\  \\____/\\____/\\_/  \\|\\____\\ \n\n";

    return FS_CONF_OK;
}

static int fs_mod_version_init_mod(fs_conf_t *conf, fs_cmd_t *cmd, void **ctx) {
    (void) conf;
    (void) cmd;

    *ctx = fs_pool_alloc(&conf->pool, sizeof(fs_mod_version_t));

    return FS_CONF_OK;
}

static int fs_mod_version_init_mod_completed(fs_run_t *run, void *ctx) {
    (void) run;

    printf("%s\n", ((fs_mod_version_t *) ctx)->welcome);

    return FS_CONF_OK;
}

static fs_mod_method_t method = {
    .init_mod = fs_mod_version_init_mod,
    .init_mod_completed = fs_mod_version_init_mod_completed
};

fs_mod(1, fs_mod_version, &method,
       fs_cmd("version",        fs_mod_verison_init, true,  false),
       fs_cmd("version_echo",   fs_mod_version_echo, false, true));
