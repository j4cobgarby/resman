// vim: fdm=marker
#include "server.h"

#include <assert.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/pidfd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "resman.h"

/* The currently executing job, or NULL if no job is currently running. */
queued_job* running_job = NULL;

/* Queue for jobs to run, not including the one which is currently running.
 * Only includes actual jobs, not timeslot reservations, since those can't
 * be queued */
queued_job* q = NULL;

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
    fflush(stdout);

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
        fprintf(stderr, "pthread_create failed\n");
        return EXIT_FAILURE;
    }

    while (true) {
        if ((soc_client = accept(soc_listen, (struct sockaddr*)&sa_client,
                                 &soc_len)) < 0) {
            perror("accept");
            continue;
        }

        if (handle_client(soc_client) < 0) {
            RESMAND_ERROR("Failed while handling a new client\n");
        }
    }
} /*}}}*/

/* The dispatcher is responsible for polling the currently running job (if one
 * exists) to check when it ends. When there is no job (the server is free),
 * then this function begins a new one.
 * It's meant to be run as a thread. */
void* dispatcher(void* args UNUSED) { /*{{{*/
    queued_job* next_job;

    while (true) {
        sleep(POLL_DELAY);

        pthread_mutex_lock(&mut_rj);
        if (running_job) {
            /* There is a currently running job. Time or Cmd.
             * This code checks if the current job is ready to end. */
            switch (running_job->job.job_type) {
                case JOB_TIMESLOT: {
                    if (running_job->manually_released ||
                        time(NULL) >= running_job->job.timeslot.t_end) {
                        RESMAND_INFO("Timeslot job %d finished\n",
                                     running_job->job.job_uuid);
                        free_queued_job(running_job);
                        goto try_start;
                    }
                    break;
                }
                case JOB_CMD: {
                    /* The pidfd becomes readable when the task that it refers
                     * to has terminated (see open(2)), use poll() to check. */
                    bool job_exited = false;
                    struct pollfd pollfd = {
                        .fd = running_job->job.cmd.pidfd,
                        .events = POLLIN,
                    };
                    const int r = poll(&pollfd, 1, 0);
                    if (r == -1) {
                        perror("poll");
                    } else if (pollfd.revents & POLLIN) {
                        // The job has ended, pidfd became readable
                        RESMAND_INFO("Command job %d finished\n",
                                     running_job->job.job_uuid);
                        job_exited = true;
                    }

                    if (job_exited || running_job->manually_released) {
                        close(running_job->job.cmd.pidfd);
                        free_queued_job(running_job);
                        goto try_start;
                    }

                    break;
                }
            }
        } else {
            /* No job is running, so let's try to start a new one. */
        try_start:
            pthread_mutex_lock(&mut_q);
            /* Get the next job in the queue. This can be NULL, but that's
             * okay; just means the queue was empty. */
            next_job = deq_job(&q);
            pthread_mutex_unlock(&mut_q);

            running_job = next_job;

            if (running_job) {
                switch (running_job->job.job_type) {
                    case JOB_CMD:
                        const pid_t target_pid =
                            pidfd_getpid(running_job->job.cmd.pidfd);
                        if (target_pid == -1) {
                            RESMAND_ERROR(
                                "Failed to get PID for next command job %d in "
                                "queue, maybe the process exited?",
                                running_job->job.job_uuid)
                            // Try next job in queue without delay
                            close(running_job->job.cmd.pidfd);
                            free_queued_job(running_job);
                            goto try_start;
                        }

                        RESMAND_INFO(
                            "Starting command job %d (pid: %d) for user %d: "
                            "'%s'\n",
                            running_job->job.job_uuid, target_pid,
                            running_job->job.uid, running_job->job.msg);
                        running_job->job.t_started = time(NULL);

                        // Signal the waiting job stub to start executing
                        if (pidfd_send_signal(running_job->job.cmd.pidfd,
                                              SIGUSR1, NULL, 0) == -1) {
                            perror("pidfd_send_signal");
                            RESMAND_ERROR(
                                "Failed to signal next command job %d in "
                                "queue, trying the next one",
                                running_job->job.job_uuid);
                            // Try next job in queue instead
                            close(running_job->job.cmd.pidfd);
                            free_queued_job(running_job);
                            goto try_start;
                        }
                        break;
                    case JOB_TIMESLOT:
                        RESMAND_INFO(
                            "Starting timeslot job %d. Sleeping for %ds\n",
                            running_job->job.job_uuid,
                            running_job->job.timeslot.secs);
                        running_job->job.t_started = time(NULL);
                        running_job->job.timeslot.t_end =
                            running_job->job.t_started +
                            running_job->job.timeslot.secs;
                        break;
                    default:
                        RESMAND_ERROR("Received invalid job type: %d\n",
                                      running_job->job.job_type);
                }
            }
        }
        pthread_mutex_unlock(&mut_rj);
    }
} /*}}}*/

int send_queue_info(const int soc_client, const unsigned int count) { /* {{{ */
    pthread_mutex_lock(&mut_q);
    pthread_mutex_lock(&mut_rj);

    int qlen = 0;
    queued_job* qjob;
    if (q) {
        for (qlen = 1, qjob = q; qjob->next; qjob = qjob->next, qlen++);
    }

    queue_info_response_header header;
    header.total_count = qlen + (running_job ? 1 : 0);
    header.resp_count = header.total_count > count ? count : header.total_count;
    header.currently_running = !!running_job;

    const unsigned long buf_len =
        sizeof(header) + header.resp_count * sizeof(job_descriptor);
    char* ser_buf = malloc(buf_len);
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
        RESMAND_ERROR("Failed sending queue response to client\n");
        free(ser_buf);
        return -1;
    }

    free(ser_buf);
    return 0;
fail:
    pthread_mutex_unlock(&mut_q);
    pthread_mutex_unlock(&mut_rj);
    return -1;
} /* }}} */

void sigint_handler(int sig UNUSED) { /*{{{*/
    printf("Caught SIGINT, exiting\n");
    exit(EXIT_SUCCESS);
} /*}}}*/

void free_queued_job(queued_job* qjob) { free(qjob); }
