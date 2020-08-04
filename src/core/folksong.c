#include "fs_str.h"
#include "fs_conf.h"
#include "fs_mod.h"
#include "fs_arr.h"
#include "fs_run.h"
#include <stdio.h>

#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void usage() {
    printf("Folksong v0.1. 使用方式:\n\n");
    printf("-h\t\t帮助文档\n");
    printf("-c\t\t命令行设定Folksong配置\n");
    printf("-f\t\t指定Folksong配置文件\n");
}

int main(int argc, char **argv) {
    int i;
    char *arg;
    /*fs_str_t cmdline = fs_str("kafka_listener { kafka_config bootstrap.servers 127.0.0.1:9092; kafka_config group.id rand; kafka_topic test; exec /bin/python3 /root/proj/folksong/scripts/recv.py $kafka_key; }");*/
    fs_conf_t conf;
    fs_str_t param;
    // init conf
    conf.tokens = fs_alloc_arr(NULL, 3, sizeof(fs_str_t));

    fs_run_t run;
    fs_run_init(&run, &conf);

    if (argc == 1) {
        fs_log_err(&conf->log, "fs_main: invalid argument: %s\n", argv[0]);
        return -1;
    }

    for (i = 1; i < argc; i++) {
        arg = argv[i];

        if (arg[0] == '-') {
            switch (arg[1]) {
            case 'h':
                usage();
                return 0;

            case 'c':
                fs_str_set(&param, argv[i + 1]);
                fs_conf(&conf, &param);

                if (!conf.valid) {
                    return -1;
                }
                goto run;

            case 'f':
                fs_str_set(&param, argv[i + 1]);
                fs_conf_file(&conf, &param);

                if (!conf.valid) {
                    return -1;
                }
                goto run;

            default:
                fs_log_err(&conf->log, "fs_main: invalid argument: %s\n", arg);
            }
        }
        else {
            fs_log_err(&conf->log, "fs_main: invalid argument: %s\n", arg);
        }
    }

run:
    fs_run(&run);

    return 0;
}
