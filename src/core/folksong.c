#include "fs_str.h"
#include "fs_conf.h"
#include "fs_mod.h"
#include "fs_arr.h"
#include "fs_run.h"
#include <stdio.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main() {
    fs_str_t cmdline = fs_str("timer { timeout 1s; repeat 2s; exec 'echo \"Hello World\"'; }");
    fs_conf_t conf;

    // init conf
    conf.tokens = fs_alloc_arr(NULL, 3, sizeof(fs_str_t));

    fs_run_t run;
    fs_run_init(&run, &conf);

    fs_conf(&conf, &cmdline);

    if (!conf.valid) {
        return -1;
    }

    fs_run(&run);

    return 0;
}
