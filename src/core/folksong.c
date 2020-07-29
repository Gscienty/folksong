#include "fs_str.h"
#include "fs_conf.h"
#include "fs_mod.h"
#include <stdio.h>

int main() {
    fs_str_t cmdline = fs_str("version { version_echo; }");
    fs_conf_t conf;
    conf.tokens = fs_alloc_arr(NULL, 3, sizeof(fs_str_t));
    conf.ctx = fs_alloc_arr(NULL, 32, sizeof(void *));

    fs_mod_init();

    fs_conf_parse_cmdline(&conf, &cmdline);

    return 0;
}
