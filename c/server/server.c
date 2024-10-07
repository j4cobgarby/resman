// vim: fdm=marker
#include "server.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "resman.h"

/* The currently executing job, or NULL if no job is currently running. */
job_descriptor *running_job = NULL;

/* Queue for jobs to run, not including the one which is currently running.
 * Only includes actual jobs, not timeslot reservations, since those can't
 * be queued */
job_descriptor *q = NULL;

pthread_mutex_t mut_rj = PTHREAD_MUTEX_INITIALIZER;  // Mtx for running_job
pthread_mutex_t mut_q = PTHREAD_MUTEX_INITIALIZER;   // Mtx for job queue

void *dispatcher(void *args UNUSED) {
    pid_t pid;
    job_descriptor *next_job;
    int is_running;

    while (true) {
        sleep(POLL_DELAY);

        pthread_mutex_lock(&mut_rj);
        is_running = !!running_job;
        pthread_mutex_unlock(&mut_rj);

        if (is_running) {
            /* There is a currently running job. */

            pthread_mutex_lock(&mut_rj);

            if (running_job->req_type == JOB_CMD) {
                pid = running_job->cmd.pid;
                pthread_mutex_unlock(&mut_rj);
            } else {
                /* Current job is not a command, but a timeslot */
                pthread_mutex_unlock(&mut_rj);
                continue;
            }

            if (kill(pid, 0) == 0) {
                /* Job still lives */
                continue;
            } else if (errno == ESRCH) {
                /* The job has ended */
                printf("[dispatcher] Job has ended!\n");
                pthread_mutex_lock(&mut_rj);
                free_job_descriptor(running_job);
                running_job = NULL;
                pthread_mutex_unlock(&mut_rj);

                goto try_start;
            } else {
                perror("kill");
                exit(EXIT_FAILURE);
            }
        } else {
            /* No job is running, so let's try to start a new one. */
        try_start:
            pthread_mutex_lock(&mut_q);
            /* Get the next job in the queue. This can be NULL, but that's
             * okay. */
            next_job = deq_job(&q);
            pthread_mutex_unlock(&mut_q);

            pthread_mutex_lock(&mut_rj);
            running_job = next_job;
            if (running_job) {
                /* This should be the case, because only CMD jobs should be in
                 * the queue. If not, there's a bug. */
                assert(running_job->req_type == JOB_CMD);

                printf("[dispatcher] Got job %d, sending signal.\n",
                       running_job->cmd.pid);

                /* Tell the waiting job stub to start the desired process */
                kill(running_job->cmd.pid, SIGUSR1);
            }
            pthread_mutex_unlock(&mut_rj);
        }
    }
}

int main(void) { /*{{{*/
    int soc_listen, soc_client;
    struct sockaddr_un sa_client = {0};
    unsigned int soc_len = sizeof(sa_client);
    pthread_t thr_dispatcher;

    if ((soc_listen = make_soc_listen(socket_addr)) < 0) {
        return EXIT_FAILURE;
    }

    if (signal(SIGINT, &sigint_handler) == SIG_ERR) {
        perror("signal");
        return EXIT_FAILURE;
    }

    if (pthread_create(&thr_dispatcher, NULL, &dispatcher, NULL) != 0) {
        fprintf(stderr, "pthread_create failed!.\n");
        return EXIT_FAILURE;
    }

    while (true) {
        printf("[main] Waiting for connection.\n");
        if ((soc_client = accept(soc_listen, (struct sockaddr *)&sa_client,
                                 &soc_len)) < 0) {
            perror("accept");
            continue;
        }

        if (handle_client(soc_client) < 0) {
            fprintf(stderr, "Failed while handling a client.\n");
        }
    }
} /*}}}*/

void sigint_handler(int sig UNUSED) { /*{{{*/
    printf("Caught SIGINT: exiting.\n");
    exit(EXIT_SUCCESS);
} /*}}}*/

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
                printf("[main] Requested timeslot reservation allowed.\n");
                // TODO: Send message back to client
            }
            break;
    }

    close(soc_client);

    return 0;
} /*}}}*/

const job_descriptor *peek_job(job_descriptor *q, int off) { /*{{{*/
    for (int i = 0; i < off && q; i++, q = q->next)
        ;

    return q;
} /*}}}*/

job_descriptor *deq_job(job_descriptor **q) { /*{{{*/
    job_descriptor *ret = *q;

    *q = (ret ? ret->next : NULL);

    return ret;
} /*}}}*/

int enq_job(job_descriptor **q, job_descriptor *job) { /*{{{*/
    int i;

    if (!*q) {
        *q = job;
        job->next = NULL;
        return 1;
    }

    for (i = 1; (*q)->next; q = &(*q)->next, i++)
        ;

    (*q)->next = job;
    job->next = NULL;
    return i;
} /*}}}*/
