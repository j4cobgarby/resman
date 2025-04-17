// vim: fdm=marker
#include "server.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <pwd.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <systemd/sd-journal.h>
#include <unistd.h>

#include "resman.h"

/* The currently executing job, or NULL if no job is currently running. */
queued_job *running_job = NULL;

/* Queue for jobs to run, not including the one which is currently running.
 * Only includes actual jobs, not timeslot reservations, since those can't
 * be queued */
queued_job *q = NULL;

/* Mutex for running_job */
pthread_mutex_t mut_rj = PTHREAD_MUTEX_INITIALIZER;

/* Mutex for job queue (q) */
pthread_mutex_t mut_q = PTHREAD_MUTEX_INITIALIZER;

int main(void) { /*{{{*/
    printf(
        "  ____\n"
        " |  _ \\ ___  ___ _ __ ___   __ _ _ __  \n"
        " | |_) / _ \\/ __| '_ ` _ \\ / _` | '_ \\ \n"
        " |  _ <  __/\\__ \\ | | | | | (_| | | | |\n"
        " |_| \\_\\___||___/_| |_| |_|\\__,_|_| |_|\n"
        "Version 0.0\n");
    printf("\0337");  // Save cursor position for persistent status line
    disp_status();
    int soc_listen, soc_client;
    struct sockaddr_un sa_client = {0};
    unsigned int soc_len = sizeof(sa_client);
    pthread_t thr_dispatcher;

    RESMAND_INFO("== Resman daemon ==");

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
        if ((soc_client = accept(soc_listen, (struct sockaddr *)&sa_client,
                                 &soc_len)) < 0) {
            perror("accept");
            continue;
        }

        if (handle_client(soc_client) < 0) {
            RESMAND_ERROR("Failed while handling a new client.");
        }
    }
} /*}}}*/

void disp_status(void) {
    // Go back to saved pos, clear line, and go to start
    printf("\0338\033[2K\r");

    if (running_job) {
        struct passwd *pw = getpwuid(running_job->job.uid);
        const char *username = pw ? pw->pw_name : "<unknown>";
        if (running_job->job.job_type == JOB_CMD) {
            printf("\0337> running job of %s, %d jobs queued", username,
                   queue_len(q));
        } else {
            printf("\0337> running timeslot for %s, %d jobs queued", username,
                   queue_len(q));
        }
    } else {
        printf("\0337> idle, %d jobs queued", queue_len(q));
    }
    fflush(stdout);
}

/* The dispatcher is responsible for polling the currently running job (if one
 * exists) to check when it ends. When there is no job (the server is free,)
 * then this function begins a new one.
 * It's meant to be run as a thread. */
void *dispatcher(void *args UNUSED) { /*{{{*/
    pid_t pid;
    queued_job *next_job;
    int is_running;

    while (true) {
        sleep(POLL_DELAY);

        pthread_mutex_lock(&mut_rj);
        is_running = !!running_job;
        pthread_mutex_unlock(&mut_rj);

        if (is_running) {
            /* There is a currently running job. */

            assert(running_job->job.job_type == JOB_CMD);
            pid = running_job->job.cmd.pid;

            if (kill(pid, 0) == 0) {
                /* Job is still alive, just wait. */
                continue;
            } else if (errno == ESRCH) {
                /* The job has ended */
                RESMAND_INFO("Job finished, uuid=%d",
                             running_job->job.job_uuid);
                disp_status();

                /* TODO: Here we could add the finished job to a persistent
                 * database */
                pthread_mutex_lock(&mut_rj);
                free_queued_job(running_job);
                running_job = NULL;
                pthread_mutex_unlock(&mut_rj);

                goto try_start;  // Skip the timeout to try to start a new job.
            } else {
                perror("kill");
                exit(EXIT_FAILURE);
            }
        } else {
            /* No job is running, so let's try to start a new one. */
        try_start:
            pthread_mutex_lock(&mut_q);
            /* Get the next job in the queue. This can be NULL, but that's
             * okay; just means the queue was empty. */
            next_job = deq_job(&q);
            pthread_mutex_unlock(&mut_q);

            pthread_mutex_lock(&mut_rj);
            running_job = next_job;
            pthread_mutex_unlock(&mut_rj);

            disp_status();

            if (running_job) {
                switch (running_job->job.job_type) {
                    case JOB_CMD:
                        RESMAND_INFO(
                            "Sending start signal to job(pid=%d, job_uuid=%d).",
                            running_job->job.cmd.pid,
                            running_job->job.job_uuid);
                        /* Tell the waiting job stub to start the desired
                         * process */
                        kill(running_job->job.cmd.pid, SIGUSR1);
                        break;
                    case JOB_TIMESLOT:
                        RESMAND_INFO(
                            "Serving time slot request. Sleeping for %d secs",
                            running_job->job.timeslot.secs);
                        sleep(running_job->job.timeslot.secs);
                        RESMAND_INFO("Sleep over.\n");
                        running_job = NULL;
                        break;
                    default:
                        RESMAND_ERROR("Got malformed job type: %d",
                                      running_job->job.job_type);
                }
            }
        }
    }
} /*}}}*/

int send_queue_info(int soc_client, unsigned int count) {
    pthread_mutex_lock(&mut_q);
    pthread_mutex_lock(&mut_rj);

    int qlen = 0;
    queued_job *qjob;
    if (q) {
        for (qlen = 1, qjob = q; qjob->next; qjob = qjob->next, qlen++)
            ;
    }

    queue_info_response_header header;
    header.total_count = qlen + (running_job ? 1 : 0);
    header.resp_count = header.total_count > count ? count : header.total_count;
    header.currently_running = !!running_job;

    const unsigned long buf_len =
        sizeof(header) + header.resp_count * sizeof(job_descriptor);
    char *ser_buf = malloc(buf_len);
    if (!ser_buf) goto fail;

    memcpy(ser_buf, &header, sizeof(header));
    int pos = sizeof(header);
    unsigned int i = 0;
    if (running_job) {
        memcpy(ser_buf + pos, &running_job->job, sizeof(job_descriptor));
        pos += sizeof(job_descriptor);
        i += 1;
    }
    for (qjob = q; i < header.resp_count && qjob; qjob = qjob->next, i++) {
        memcpy(ser_buf + pos, &qjob->job, sizeof(job_descriptor));
        pos += sizeof(job_descriptor);
    }
    pthread_mutex_unlock(&mut_q);
    pthread_mutex_unlock(&mut_rj);

    if (send(soc_client, ser_buf, buf_len, 0) < 0) {
        RESMAND_ERROR("Failed sending queue response to client.");
        free(ser_buf);
        return -1;
    }

    free(ser_buf);
    return 0;
fail:
    pthread_mutex_unlock(&mut_q);
    pthread_mutex_unlock(&mut_rj);
    return -1;
}

void sigint_handler(int sig UNUSED) { /*{{{*/
    printf("Caught SIGINT: exiting.\n");
    exit(EXIT_SUCCESS);
} /*}}}*/

void free_queued_job(queued_job *qjob) { free(qjob); }
