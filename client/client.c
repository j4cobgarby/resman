// vim: fdm=marker
#include "client.h"

#include <argp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

int main(int argc, char **argv) { /*{{{*/
    if (argc < 2) {
        print_subcmds(argv[0]);
        return EXIT_FAILURE;
    }

    if (!strcmp(argv[1], "run") || !strcmp(argv[1], "r") ||
        !strcmp(argv[1], "x")) {
        return subcmd_run(argc, argv);
    } else if (!strcmp(argv[1], "time") || !strcmp(argv[1], "t")) {
        return subcmd_time(argc, argv);
    } else if (!strcmp(argv[1], "check") || !strcmp(argv[1], "c")) {
        return subcmd_check(argc, argv);
    } else if (!strcmp(argv[1], "dequeue") || !strcmp(argv[1], "d")) {
        return subcmd_dequeue(argc, argv);
    } else if (!strcmp(argv[1], "version") || !strcmp(argv[1], "v")) {
        printf("Resman client version " CLIENT_VER_STRING ".\n");
    } else {
        print_subcmds(argv[0]);
        return EXIT_FAILURE;
    }

    return 0;
} /*}}}*/

int connect_to_server(const char *addr) { /*{{{*/
    int soc;
    struct sockaddr_un sa_server;
    unsigned int sa_len;

    if ((soc = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    memset(&sa_server, 0, sizeof(sa_server));
    sa_server.sun_family = AF_UNIX;
    sa_server.sun_path[0] = '\0';
    strncpy(sa_server.sun_path + 1, addr, strlen(addr));
    sa_len = offsetof(struct sockaddr_un, sun_path) + 1 + strlen(addr);

    if (connect(soc, (struct sockaddr *)&sa_server, sa_len) < 0) {
        return -1;
    }

    return soc;
} /*}}}*/
