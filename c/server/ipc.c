// vim: fdm=marker
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "server.h"

/* Create and return a new UNIX domain socket which listens on a given address.
 * This address is a filesystem path, since this type of socket uses a "file"
 * to communicate over. */
int make_soc_listen(const char *addr) { /*{{{*/
    int soc_listen;
    struct sockaddr_un sa_local = {0};

    soc_listen = socket(AF_UNIX, SOCK_STREAM, 0);
    if (soc_listen < 0) {
        perror("socket");
        return -1;
    }

    remove(addr);

    sa_local.sun_family = AF_UNIX;
    strcpy(sa_local.sun_path, addr);

    if (bind(soc_listen, (struct sockaddr *)&sa_local,
             sizeof(struct sockaddr_un)) < 0) {
        perror("bind");
        return -1;
    }

    if (listen(soc_listen, LISTEN_QUEUE) < 0) {
        perror("listen");
        return -1;
    }

    return soc_listen;
} /*}}}*/

/* Handle a new client connection. This will wait for the client to send a
 * request, at which point -- based on the type of request -- it will perform
 * the necessary action.
 * If the client wants to enqueue a job, then it does that. If the client wants
 * to reserve the server for some time, this will check if the server is free
 * right now and, if so, allow the reservation.
 * Returns -1 on failure, or 0 on success. */
int handle_client(int soc_client) { /*{{{*/
    char read_buf[JOB_SER_MAXLEN] = {0};
    job_descriptor *job;
    int n_jobs;

    printf("[main] Client connected.\n");

    int bytes_read;
    if ((bytes_read = recv(soc_client, read_buf, JOB_SER_MAXLEN, 0)) == -1) {
        perror("recv");
    }

    if ((job = (job_descriptor *)malloc(sizeof(job_descriptor))) == NULL) {
        perror("malloc");
    }

    if (deserialise_job(read_buf, bytes_read, job) < 0) {
        printf("Failed to deserialise job.\n");
        close(soc_client);
        return -1;
    }

    printf("== Job ==\n");
    printf(
        " * UID=%d\n"
        " * Message=%s\n"
        " * Submit time=%ld\n"
        " * Run time=%ld\n"
        " * End time=%ld\n",
        job->uid, job->msg, job->t_submitted, job->t_started, job->t_ended);
    if (job->req_type == JOB_CMD) {
        printf(" * PID=%d\n", job->cmd.pid);
    } else {
        printf(" * Seconds=%d\n", job->timeslot.secs);
    }
    printf("=========\n");

    switch (job->req_type) {
        case JOB_CMD:
            n_jobs = enq_job(&q, job);
            printf("[main] Enqueued job. Now %d jobs in queue\n", n_jobs);
            // TODO: Send confirmation to client
            break;
        case JOB_TIMESLOT:
            if (peek_job(q, 0)) {
                printf(
                    "[main] User %d wanted a timeslot, but server is already "
                    "reserved.\n",
                    job->uid);
                // TODO: Send message back to client
            } else {
                n_jobs = enq_job(&q, job);
                printf("[main] Requested timeslot reservation allowed.\n");
                // TODO: Send message back to client
            }
            break;
    }

    close(soc_client);

    return 0;
} /*}}}*/
