// vim: fdm=marker
#include "server.h"

#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "resman.h"

void sigint_handler(int sig UNUSED) { /*{{{*/
    printf("Caught SIGINT: exiting.\n");
    exit(EXIT_SUCCESS);
} /*}}}*/

int make_soc_listen(const char *addr) { /*{{{*/
    int soc_listen;
    struct sockaddr_un sa_local = {0};

    soc_listen = socket(AF_UNIX, SOCK_STREAM, 0);
    if (soc_listen < 0) {
        fprintf(stderr, "Failed to create UNIX socket.\n");
        return -1;
    }

    remove(socket_addr);

    sa_local.sun_family = AF_UNIX;
    strcpy(sa_local.sun_path, addr);

    if (bind(soc_listen, (struct sockaddr *)&sa_local,
             sizeof(struct sockaddr_un)) < 0) {
        fprintf(stderr, "Failed to bind socket.\n");
        return -1;
    }

    if (listen(soc_listen, LISTEN_QUEUE) < 0) {
        fprintf(stderr, "Failed to listen on socket.\n");
        return -1;
    }

    return soc_listen;
} /*}}}*/

int handle_client(int soc_client) { /*{{{*/
    char read_buf[JOB_SER_MAXLEN];
    printf("[info] Client connected.\n");

    int bytes_read;
    if ((bytes_read = recv(soc_client, read_buf, JOB_SER_MAXLEN, 0)) == -1) {
        fprintf(stderr, "Failed to read from client.\n");
    }

    job_descriptor job;
    if (deserialise_job(read_buf, bytes_read, &job) < 0) {
        printf("Failed to deserialise job.\n");
        close(soc_client);
        return -1;
    }

    printf("== Job ==\n");
    printf(
        "\tUID=%d\n"
        "\tMessage=%s\n"
        "\tSubmit time=%ld\n"
        "\tRun time=%ld\n"
        "\tEnd time=%ld\n",
        job.uid, job.msg, job.t_submitted, job.t_started, job.t_ended);
    if (job.req_type == JOB_CMD) {
        printf("\tPID=%d\n", job.cmd.pid);
    } else {
        printf("\tSeconds=%d\n", job.timeslot.secs);
    }
    printf("=========\n");
    free_job_descriptor(&job);

    close(soc_client);

    return 0;
} /*}}}*/

int main(void) { /*{{{*/
    int soc_listen, soc_client;
    unsigned int soc_len; /* Used to get length of sa_client in accept() */
    struct sockaddr_un sa_client = {0};

    if ((soc_listen = make_soc_listen(socket_addr)) < 0) {
        return EXIT_FAILURE;
    }

    if (signal(SIGINT, &sigint_handler) == SIG_ERR) {
        perror("signal");
        return EXIT_FAILURE;
    }

    while (true) {
        printf("[info] Waiting for connection.\n");
        if ((soc_client = accept(soc_listen, (struct sockaddr *)&sa_client,
                                 &soc_len)) < 0) {
            fprintf(stderr, "Failed to accept client.\n");
            continue;
        }

        if (handle_client(soc_client) < 0) {
            fprintf(stderr, "Failed while handling a client.\n");
        }
    }
} /*}}}*/
