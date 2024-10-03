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

    if (!strcmp(argv[1], "run") || !strcmp(argv[1], "r")) {
        return subcmd_run(argc, argv);
    } else if (!strcmp(argv[1], "time") || !strcmp(argv[1], "t")) {
        return subcmd_time(argc, argv);
    } else if (!strcmp(argv[1], "queue") || !strcmp(argv[1], "q")) {
        return subcmd_queue(argc, argv);
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

    sa_server.sun_family = AF_UNIX;
    strcpy(sa_server.sun_path, addr);
    sa_len = strlen(sa_server.sun_path) + sizeof(sa_server.sun_family);

    if (connect(soc, (struct sockaddr *)&sa_server, sa_len) < 0) {
        return -1;
    }

    return soc;
} /*}}}*/
