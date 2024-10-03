// vim: fdm=marker
#include "resman.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include <signal.h> /* signal() */

#define LISTEN_QUEUE 8

void sigint_handler(int sig UNUSED) {/*{{{*/
    printf("Caught SIGINT: exiting.\n");
    exit(EXIT_SUCCESS);
}/*}}}*/

int main(void) {/*{{{*/
    int soc_listen, soc_client;
    struct sockaddr_un sa_local = {0};
    struct sockaddr_un sa_client = {0};
    unsigned int soc_len; /* Used to get length of sa_client in accept() */

    char read_buf[JOB_SER_MAXLEN];

    soc_listen = socket(AF_UNIX, SOCK_STREAM, 0);
    if (soc_listen < 0) {
        fprintf(stderr, "Failed to create UNIX socket.\n");
        return EXIT_FAILURE;
    }

    remove(socket_addr);

    sa_local.sun_family = AF_UNIX;
    strcpy(sa_local.sun_path, socket_addr);

    if (bind(soc_listen, (struct sockaddr *)&sa_local,
             sizeof(struct sockaddr_un)) < 0) {
        fprintf(stderr, "Failed to bind socket.\n");
        return EXIT_FAILURE;
    }

    if (listen(soc_listen, LISTEN_QUEUE) < 0) {
        fprintf(stderr, "Failed to listen on socket.\n");
        return EXIT_FAILURE;
    }

    signal(SIGINT, &sigint_handler);

    while (true) {
        printf("[info] Waiting for connection.\n");
        if ((soc_client = accept(soc_listen, (struct sockaddr *)&sa_client,
                                 &soc_len)) < 0) {
            fprintf(stderr, "Failed to accept client. Restarting loop.\n");
            continue;
        }

        printf("[info] Client connected.\n");

        int bytes_read;
        if ((bytes_read = recv(soc_client, read_buf,
                               JOB_SER_MAXLEN, 0)) == -1) {
            fprintf(stderr, "Failed to read from client.\n");
        }

        printf("Read %d bytes: %s", bytes_read, read_buf);

        close(soc_client);
    }
}/*}}}*/
