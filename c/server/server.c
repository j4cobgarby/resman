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

/* Mutex for running_job */
pthread_mutex_t mut_rj = PTHREAD_MUTEX_INITIALIZER;

/* Mutex for job queue (q) */
pthread_mutex_t mut_q = PTHREAD_MUTEX_INITIALIZER;

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

/* The dispatcher is responsible for polling the currently running job (if one
 * exists) to check when it ends. When there is no job (the server is free,)
 * then this function begins a new one.
 * It's meant to be run as a thread. */
void *dispatcher(void *args UNUSED) { /*{{{*/
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
} /*}}}*/

void sigint_handler(int sig UNUSED) { /*{{{*/
    printf("Caught SIGINT: exiting.\n");
    exit(EXIT_SUCCESS);
} /*}}}*/
