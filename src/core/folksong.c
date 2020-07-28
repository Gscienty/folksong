#include "fs_str.h"
#include "fs_conf.h"
#include <stdio.h>

int main() {
    fs_str_t conf_filename = fs_str("./conf.debug");

    fs_conf_t conf;

    conf.tokens = fs_alloc_arr(NULL, 3, sizeof(fs_str_t));

    fs_conf_parse(&conf, &conf_filename);

    printf("%d\n", fs_arr_count(conf.tokens));

    int i;
    for (i = 0; i < fs_arr_count(conf.tokens); i++) {
        printf("%s\n", fs_str_get(fs_arr_nth(fs_str_t, conf.tokens, i)));
    }

    return 0;
}
