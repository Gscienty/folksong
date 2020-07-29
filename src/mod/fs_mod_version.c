/*
 * Copyright (c) 2020 Gscienty <gaoxiaochuan@hotmail.com>
 *
 * Distributed under the MIT software license, see the accompanying
 * file LICENSE or https://www.opensource.org/licenses/mit-license.php .
 *
 */

#include "fs_mod.h"
#include <stdio.h>

static int fs_mod_verison_init(fs_conf_t *conf, fs_cmd_t *cmd, void **env) {
    (void) conf;
    (void) cmd;

    *env = "folk song";

    return FS_MOD_OK;
}

static int fs_mod_version_echo(fs_conf_t *conf, fs_cmd_t *cmd, void **env) {
    (void) conf;
    (void) cmd;
    (void) env;

    *env = 
         "_____ ____  _     _  __   ____  ____  _      _____\n"
         "/    //  _ \\/ \\   / |/ /  / ___\\/  _ \\/ \\  /|/  __/\n"
         "|  __\\| / \\|| |   |   /   |    \\| / \\|| |\\ ||| |  _\n"
         "| |   | \\_/|| |_/\\|   \\   \\___ || \\_/|| | \\||| |_//\n"
         "\\_/   \\____/\\____/\\_|\\_\\  \\____/\\____/\\_/  \\|\\____\\ \n\n";


    return FS_MOD_OK;
}

static int fs_mod_version_init_mod(fs_conf_t *conf, fs_cmd_t *cmd, void **env) {
    (void) conf;
    (void) cmd;

    printf("%s\n", (char *) *env);

    return FS_CONF_OK;
}

static fs_mod_method_t method = {
    .init_mod = fs_mod_version_init_mod
};

fs_mod(1, fs_mod_version, &method,
       fs_cmd("version",        fs_mod_verison_init, true,  false),
       fs_cmd("version_echo",   fs_mod_version_echo, false, true));
