// vim: fdm=marker
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "resman.h"
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

/* Get the next job UUID. This will wrap around, but we assume that there won't
 * be so many jobs in the queue that there are ID collisions. */
static uuid_t next_uuid(void) { /*{{{*/
    static uuid_t next = 0;
    if (next == UUID_MAX) next = 0;
    return ++next;
} /*}}}*/

static int send_status(int soc, status_response *resp) {/*{{{*/
    if (!resp) return -1;
    return send(soc, resp, sizeof(status_response), 0);
}/*}}}*/

/* Handle a new client connection. This will wait for the client to send a
 * request, at which point -- based on the type of request -- it will perform
 * the necessary action.
 * Returns -1 on failure, or 0 on success. */
int handle_client(int soc_client) { /*{{{*/
    ipc_request req;
    status_response resp;
    int n_jobs;

    printf("[main] Client connected.\n");

    int bytes_read;
    if ((bytes_read = recv(soc_client, &req, sizeof(req), 0)) == -1) {
        perror("recv");
    }

    if (bytes_read != sizeof(req)) {
        printf("Something went wrong, we read %d bytes but expected %lu\n",
               bytes_read, sizeof(req));
        close(soc_client);
        return -1;
    }

    if (req.req_type == IPCREQ_JOB) {
        job_descriptor job = req.job;
        job.job_uuid = next_uuid();
        printf("== Job ==\n");
        printf(
            " * User ID=%d\n"
            " * Message=%s\n"
            " * Submit time=%ld\n"
            " * Generated UUID=%d\n",
            job.uid, job.msg, job.t_submitted, job.job_uuid);
        if (job.job_type == JOB_CMD) {
            printf(" * PID=%d\n", job.cmd.pid);
        } else {
            printf(" * Seconds=%d\n", job.timeslot.secs);
        }
        printf("=========\n");

        switch (job.job_type) {
            case JOB_CMD:
                pthread_mutex_lock(&mut_q);
                n_jobs = enq_job(&q, job);
                pthread_mutex_unlock(&mut_q);
                printf("[main] Enqueued job. Now %d jobs in queue\n", n_jobs);
                resp.status = STATUS_OK;
                break;
            case JOB_TIMESLOT:
                if (running_job || peek_job(q, 0)) {
                    printf(
                        "[main] User %d wanted a timeslot, but server is "
                        "already "
                        "reserved.\n",
                        job.uid);
                    resp.status = STATUS_FAIL;
                } else {
                    pthread_mutex_lock(&mut_q);
                    n_jobs = enq_job(&q, job);
                    pthread_mutex_unlock(&mut_q);
                    resp.status = STATUS_OK;
                }
                break;
        }

        send_status(soc_client, &resp);
    } else if (req.req_type == IPCREQ_VIEW_QUEUE) {
        info_request info = req.info;
        printf("== Info Request ==\n");
        printf(" * Num to view=%d\n", info.n_view);
        printf("=========\n");

        send_queue_info(soc_client, info.n_view);
    } else if (req.req_type == IPCREQ_DEQUEUE) {
        dequeue_request deq = req.deq;
        printf("== Dequeue Request ==\n");
        printf(" * Job ID=%d\n", deq.job_uuid);
        printf("=========\n");

        pthread_mutex_lock(&mut_q);
        queued_job *deq_job = remove_job(&q, deq.job_uuid);
        pthread_mutex_unlock(&mut_q);

        printf("Dequeued job = %lu\n", (unsigned long)deq_job);

        free_queued_job(deq_job);
        resp.status = deq_job ? STATUS_OK : STATUS_FAIL;
        send_status(soc_client, &resp);
    } else {
        fprintf(stderr, "Incorrect request type: %d\n", req.req_type);
        close(soc_client);
        return -1;
    }

    close(soc_client);

    return 0;
} /*}}}*/
