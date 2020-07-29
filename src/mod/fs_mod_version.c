#include "fs_mod.h"
#include <stdio.h>

static int fs_mod_verison_init(fs_conf_t *conf, fs_cmd_t *cmd, void **env) {
    (void) conf;
    (void) cmd;

    *env = 
         "_____ ____  _     _  __   ____  ____  _      _____\n"
         "/    //  _ \\/ \\   / |/ /  / ___\\/  _ \\/ \\  /|/  __/\n"
         "|  __\\| / \\|| |   |   /   |    \\| / \\|| |\\ ||| |  _\n"
         "| |   | \\_/|| |_/\\|   \\   \\___ || \\_/|| | \\||| |_//\n"
         "\\_/   \\____/\\____/\\_|\\_\\  \\____/\\____/\\_/  \\|\\____\\ \n\n";

    return FS_MOD_OK;
}

static int fs_mod_version_echo(fs_conf_t *conf, fs_cmd_t *cmd, void **env) {
    (void) conf;
    (void) cmd;

    printf("%s\n", (char *) *env);

    return FS_MOD_OK;
}

fs_mod(1, fs_mod_version,
       fs_cmd("version", fs_mod_verison_init, true, false),
       fs_cmd("version_echo", fs_mod_version_echo, false, true));
