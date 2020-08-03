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
    fs_str_t cmdline = fs_str("kafka_listener {\n kafka_config bootstrap.servers 127.0.0.1:9092; \nkafka_config group.id rand; \nkafka_topic test; \nexec /bin/python3 /root/proj/folksong/scripts/recv.py; \n}");
    fs_conf_t conf;

    // init conf
    conf.tokens = fs_alloc_arr(NULL, 3, sizeof(fs_str_t));

    fs_run_t run;
    fs_run_init(&run, &conf);

    fs_conf(&conf, &cmdline);

    if (!conf.valid) {
        printf("ERR\n");
        return -1;
    }

    printf("SUCC\n");
    fs_run(&run);

    return 0;
}
